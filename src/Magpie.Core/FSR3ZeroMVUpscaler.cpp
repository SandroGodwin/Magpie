#include "pch.h"
#include "FSR3ZeroMVUpscaler.h"
#include "DeviceResources.h"
#include "HalfResOpticalFlow.h"
#include "Logger.h"

#ifdef MP_ENABLE_FSR3_ZEROMV
#include <d3d12.h>
#include <ffx_api.h>
#include <dx12/ffx_api_dx12.h>
#include <ffx_upscale.h>

namespace Magpie {

struct FSR3ZeroMVUpscaler::Impl {
	~Impl();

	ID3D11Device5* device11 = nullptr;
	ID3D11DeviceContext4* context11 = nullptr;
	winrt::com_ptr<ID3D12Device> device12;
	winrt::com_ptr<ID3D12CommandQueue> queue12;
	winrt::com_ptr<ID3D12CommandAllocator> allocator12;
	winrt::com_ptr<ID3D12GraphicsCommandList> commandList12;
	winrt::com_ptr<ID3D11Texture2D> sharedInput11;
	winrt::com_ptr<ID3D11Texture2D> sharedOutput11;
	winrt::com_ptr<ID3D11Texture2D> sharedMotion11;
	winrt::com_ptr<ID3D12Resource> sharedInput12;
	winrt::com_ptr<ID3D12Resource> sharedOutput12;
	winrt::com_ptr<ID3D12Resource> sharedMotion12;
	winrt::com_ptr<ID3D12Resource> zeroMotion12;
	winrt::com_ptr<ID3D12Resource> flatDepth12;
	winrt::com_ptr<ID3D12Resource> exposure12;
	winrt::com_ptr<ID3D12Resource> reactive12;
	winrt::com_ptr<ID3D12Resource> transparency12;
	winrt::com_ptr<ID3D12DescriptorHeap> descriptorHeap12;
	winrt::com_ptr<ID3D11Fence> fence11;
	winrt::com_ptr<ID3D12Fence> fence12;
	std::unique_ptr<HalfResOpticalFlow> opticalFlow;
	HMODULE loaderModule = nullptr;
	decltype(&ffxCreateContext) createContext = nullptr;
	decltype(&ffxDestroyContext) destroyContext = nullptr;
	decltype(&ffxDispatch) dispatch = nullptr;
	decltype(&ffxQuery) query = nullptr;
	ffxContext context = nullptr;
	ffxCreateContextDescUpscale createDesc{};
	ffxCreateBackendDX12Desc backendDesc{};
	ffxCreateContextDescUpscaleVersion apiVersion{};
	ffxOverrideVersion overrideVersion{};
	uint64_t fenceValue = 0;
	uint64_t lastSubmittedValue = 0;
	uint32_t inputWidth = 0;
	uint32_t inputHeight = 0;
	uint32_t outputWidth = 0;
	uint32_t outputHeight = 0;
	bool enableOpticalFlow = false;
	bool resetHistory = true;
};

static bool WaitForFence(FSR3ZeroMVUpscaler::Impl& impl, uint64_t value) noexcept {
	if (!value || impl.fence12->GetCompletedValue() >= value) return true;
	wil::unique_event_nothrow event;
	HRESULT hr = event.create();
	if (FAILED(hr)) return false;
	hr = impl.fence12->SetEventOnCompletion(value, event.get());
	if (FAILED(hr)) return false;
	WaitForSingleObject(event.get(), INFINITE);
	return true;
}

static bool WaitForQueue(FSR3ZeroMVUpscaler::Impl& impl) noexcept {
	const uint64_t value = ++impl.fenceValue;
	HRESULT hr = impl.queue12->Signal(impl.fence12.get(), value);
	if (FAILED(hr)) return false;
	return WaitForFence(impl, value);
}

FSR3ZeroMVUpscaler::Impl::~Impl() {
	if (queue12 && fence12) WaitForQueue(*this);
	if (context && destroyContext) destroyContext(&context, nullptr);
	if (loaderModule) FreeLibrary(loaderModule);
}

static bool CreateSharedTexture(
	FSR3ZeroMVUpscaler::Impl& impl,
	const D3D11_TEXTURE2D_DESC& sourceDesc,
	winrt::com_ptr<ID3D11Texture2D>& texture11,
	winrt::com_ptr<ID3D12Resource>& texture12
) noexcept {
	D3D11_TEXTURE2D_DESC desc = sourceDesc;
	desc.Usage = D3D11_USAGE_DEFAULT;
	desc.CPUAccessFlags = 0;
	desc.MiscFlags = D3D11_RESOURCE_MISC_SHARED | D3D11_RESOURCE_MISC_SHARED_NTHANDLE;
	HRESULT hr = impl.device11->CreateTexture2D(&desc, nullptr, texture11.put());
	if (FAILED(hr)) {
		Logger::Get().ComError("Create FSR3 shared D3D11 texture failed", hr);
		return false;
	}
	winrt::com_ptr<IDXGIResource1> dxgiResource;
	hr = texture11->QueryInterface(IID_PPV_ARGS(dxgiResource.put()));
	if (FAILED(hr)) return false;
	HANDLE rawHandle = nullptr;
	hr = dxgiResource->CreateSharedHandle(nullptr, GENERIC_ALL, nullptr, &rawHandle);
	if (FAILED(hr)) return false;
	wil::unique_handle handle(rawHandle);
	hr = impl.device12->OpenSharedHandle(handle.get(), IID_PPV_ARGS(texture12.put()));
	if (FAILED(hr)) {
		Logger::Get().ComError("Open FSR3 shared texture in D3D12 failed", hr);
		return false;
	}
	return true;
}

static bool CreateAuxTexture(
	FSR3ZeroMVUpscaler::Impl& impl,
	DXGI_FORMAT format,
	uint32_t width,
	uint32_t height,
	winrt::com_ptr<ID3D12Resource>& resource,
	D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle,
	D3D12_GPU_DESCRIPTOR_HANDLE gpuHandle,
	const float clearValue[4],
	D3D12_RESOURCE_BARRIER& barrier
) noexcept {
	D3D12_RESOURCE_DESC desc{};
	desc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	desc.Width = width;
	desc.Height = height;
	desc.DepthOrArraySize = 1;
	desc.MipLevels = 1;
	desc.Format = format;
	desc.SampleDesc.Count = 1;
	desc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
	desc.Flags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
	D3D12_HEAP_PROPERTIES heap{};
	heap.Type = D3D12_HEAP_TYPE_DEFAULT;
	HRESULT hr = impl.device12->CreateCommittedResource(&heap, D3D12_HEAP_FLAG_NONE, &desc,
		D3D12_RESOURCE_STATE_UNORDERED_ACCESS, nullptr, IID_PPV_ARGS(resource.put()));
	if (FAILED(hr)) {
		Logger::Get().ComError("Create FSR3 virtual input texture failed", hr);
		return false;
	}
	D3D12_UNORDERED_ACCESS_VIEW_DESC uav{};
	uav.Format = format;
	uav.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;
	impl.device12->CreateUnorderedAccessView(resource.get(), nullptr, &uav, cpuHandle);
	impl.commandList12->ClearUnorderedAccessViewFloat(
		gpuHandle, cpuHandle, resource.get(), clearValue, 0, nullptr);
	barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
	barrier.Transition.pResource = resource.get();
	barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
	barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE;
	barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
	return true;
}

FSR3ZeroMVUpscaler::FSR3ZeroMVUpscaler() = default;

FSR3ZeroMVUpscaler::~FSR3ZeroMVUpscaler() = default;

bool FSR3ZeroMVUpscaler::Initialize(
	DeviceResources& resources,
	ID3D11Texture2D* input,
	ID3D11Texture2D* output,
	bool enableOpticalFlow
) noexcept {
	_enableOpticalFlow = enableOpticalFlow;
	_impl.reset();
	auto impl = std::make_unique<Impl>();
	impl->device11 = resources.GetD3DDevice();
	impl->context11 = resources.GetD3DDC();
	impl->enableOpticalFlow = enableOpticalFlow;

	D3D11_TEXTURE2D_DESC inputDesc{};
	D3D11_TEXTURE2D_DESC outputDesc{};
	input->GetDesc(&inputDesc);
	output->GetDesc(&outputDesc);
	if (inputDesc.Width > outputDesc.Width || inputDesc.Height > outputDesc.Height) {
		Logger::Get().Error("FSR3 experimental backend only supports upscaling");
		return false;
	}
	if ((float)outputDesc.Width / inputDesc.Width > 3.0f ||
		(float)outputDesc.Height / inputDesc.Height > 3.0f) {
		Logger::Get().Error("FSR3 experimental backend supports up to a 3x scale");
		return false;
	}
	impl->inputWidth = inputDesc.Width;
	impl->inputHeight = inputDesc.Height;
	impl->outputWidth = outputDesc.Width;
	impl->outputHeight = outputDesc.Height;

	HRESULT hr = D3D12CreateDevice(resources.GetGraphicsAdapter(), D3D_FEATURE_LEVEL_11_0,
		IID_PPV_ARGS(impl->device12.put()));
	if (FAILED(hr)) {
		Logger::Get().ComError("Create FSR3 D3D12 device failed", hr);
		return false;
	}
	D3D12_COMMAND_QUEUE_DESC queueDesc{};
	queueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
	hr = impl->device12->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(impl->queue12.put()));
	if (SUCCEEDED(hr)) hr = impl->device12->CreateCommandAllocator(
		D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(impl->allocator12.put()));
	if (SUCCEEDED(hr)) hr = impl->device12->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT,
		impl->allocator12.get(), nullptr, IID_PPV_ARGS(impl->commandList12.put()));
	if (FAILED(hr)) {
		Logger::Get().ComError("Create FSR3 D3D12 command objects failed", hr);
		return false;
	}

	if (!CreateSharedTexture(*impl, inputDesc, impl->sharedInput11, impl->sharedInput12) ||
		!CreateSharedTexture(*impl, outputDesc, impl->sharedOutput11, impl->sharedOutput12)) {
		return false;
	}
	if (enableOpticalFlow) {
		impl->opticalFlow = std::make_unique<HalfResOpticalFlow>();
		if (!impl->opticalFlow->Initialize(impl->device11, impl->context11, input)) return false;
		D3D11_TEXTURE2D_DESC motionDesc{};
		motionDesc.Width = inputDesc.Width;
		motionDesc.Height = inputDesc.Height;
		motionDesc.MipLevels = 1;
		motionDesc.ArraySize = 1;
		motionDesc.Format = DXGI_FORMAT_R16G16_FLOAT;
		motionDesc.SampleDesc.Count = 1;
		motionDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
		if (!CreateSharedTexture(*impl, motionDesc, impl->sharedMotion11, impl->sharedMotion12)) return false;
	}

	D3D12_DESCRIPTOR_HEAP_DESC heapDesc{};
	heapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	heapDesc.NumDescriptors = 5;
	heapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	hr = impl->device12->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(impl->descriptorHeap12.put()));
	if (FAILED(hr)) return false;
	ID3D12DescriptorHeap* heaps[]{ impl->descriptorHeap12.get() };
	impl->commandList12->SetDescriptorHeaps(1, heaps);
	const UINT stride = impl->device12->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	D3D12_CPU_DESCRIPTOR_HANDLE cpu = impl->descriptorHeap12->GetCPUDescriptorHandleForHeapStart();
	D3D12_GPU_DESCRIPTOR_HANDLE gpu = impl->descriptorHeap12->GetGPUDescriptorHandleForHeapStart();
	D3D12_RESOURCE_BARRIER auxBarriers[5]{};
	UINT auxCount = 0;
	const float zero[4]{};
	const float one[4]{ 1, 1, 1, 1 };
	const float reactive02[4]{ 0.2f, 0.2f, 0.2f, 0.2f };
	const float reactive08[4]{ 0.8f, 0.8f, 0.8f, 0.8f };
	auto addAux = [&](DXGI_FORMAT format, uint32_t width, uint32_t height,
		winrt::com_ptr<ID3D12Resource>& texture, const float value[4]) -> bool {
		if (!CreateAuxTexture(*impl, format, width, height, texture, cpu, gpu, value,
			auxBarriers[auxCount])) return false;
		++auxCount;
		cpu.ptr += stride;
		gpu.ptr += stride;
		return true;
	};
	if (!addAux(DXGI_FORMAT_R32_FLOAT, inputDesc.Width, inputDesc.Height, impl->flatDepth12, zero) ||
		!addAux(DXGI_FORMAT_R32_FLOAT, 1, 1, impl->exposure12, one) ||
		!addAux(DXGI_FORMAT_R8_UNORM, inputDesc.Width, inputDesc.Height, impl->reactive12,
			enableOpticalFlow ? reactive02 : reactive08) ||
		!addAux(DXGI_FORMAT_R8_UNORM, inputDesc.Width, inputDesc.Height, impl->transparency12, zero)) {
		return false;
	}
	if (!enableOpticalFlow && !addAux(DXGI_FORMAT_R16G16_FLOAT, inputDesc.Width, inputDesc.Height,
		impl->zeroMotion12, zero)) return false;
	impl->commandList12->ResourceBarrier(auxCount, auxBarriers);
	hr = impl->commandList12->Close();
	if (FAILED(hr)) return false;
	ID3D12CommandList* lists[]{ impl->commandList12.get() };
	impl->queue12->ExecuteCommandLists(1, lists);

	hr = impl->device11->CreateFence(0, D3D11_FENCE_FLAG_SHARED, IID_PPV_ARGS(impl->fence11.put()));
	if (FAILED(hr)) return false;
	HANDLE rawFence = nullptr;
	hr = impl->fence11->CreateSharedHandle(nullptr, GENERIC_ALL, nullptr, &rawFence);
	if (FAILED(hr)) return false;
	wil::unique_handle fenceHandle(rawFence);
	hr = impl->device12->OpenSharedHandle(fenceHandle.get(), IID_PPV_ARGS(impl->fence12.put()));
	if (FAILED(hr) || !WaitForQueue(*impl)) return false;

	impl->loaderModule = LoadLibraryW(L"amd_fidelityfx_loader_dx12.dll");
	if (!impl->loaderModule) {
		Logger::Get().Win32Error("Load amd_fidelityfx_loader_dx12.dll failed");
		return false;
	}
	impl->createContext = reinterpret_cast<decltype(impl->createContext)>(
		GetProcAddress(impl->loaderModule, "ffxCreateContext"));
	impl->destroyContext = reinterpret_cast<decltype(impl->destroyContext)>(
		GetProcAddress(impl->loaderModule, "ffxDestroyContext"));
	impl->dispatch = reinterpret_cast<decltype(impl->dispatch)>(
		GetProcAddress(impl->loaderModule, "ffxDispatch"));
	impl->query = reinterpret_cast<decltype(impl->query)>(
		GetProcAddress(impl->loaderModule, "ffxQuery"));
	if (!impl->createContext || !impl->destroyContext || !impl->dispatch || !impl->query) {
		Logger::Get().Error("AMD FidelityFX loader exports are incomplete");
		return false;
	}

	ffxQueryDescGetVersions versionQuery{};
	versionQuery.header.type = FFX_API_QUERY_DESC_TYPE_GET_VERSIONS;
	versionQuery.createDescType = FFX_API_CREATE_CONTEXT_DESC_TYPE_UPSCALE;
	versionQuery.device = impl->device12.get();
	uint64_t versionCount = 0;
	versionQuery.outputCount = &versionCount;
	ffxReturnCode_t rc = impl->query(nullptr, &versionQuery.header);
	if (rc != FFX_API_RETURN_OK || !versionCount) {
		Logger::Get().Error(fmt::format("Query FSR upscaler versions failed ({})", (uint32_t)rc));
		return false;
	}
	std::vector<uint64_t> versionIds(versionCount);
	std::vector<const char*> versionNames(versionCount);
	versionQuery.versionIds = versionIds.data();
	versionQuery.versionNames = versionNames.data();
	rc = impl->query(nullptr, &versionQuery.header);
	if (rc != FFX_API_RETURN_OK) return false;
	uint64_t fsr3VersionId = 0;
	std::string availableVersions;
	for (uint64_t i = 0; i < versionCount; ++i) {
		const char* name = versionNames[i] ? versionNames[i] : "unknown";
		if (!availableVersions.empty()) availableVersions += ", ";
		availableVersions += name;
		if (strstr(name, "3.1.5")) fsr3VersionId = versionIds[i];
	}
	if (!fsr3VersionId) {
		Logger::Get().Error(fmt::format("FSR 3.1.5 provider not found; available: {}", availableVersions));
		return false;
	}

	impl->createDesc.header.type = FFX_API_CREATE_CONTEXT_DESC_TYPE_UPSCALE;
	impl->createDesc.flags = FFX_UPSCALE_ENABLE_DEPTH_INVERTED |
		FFX_UPSCALE_ENABLE_DEPTH_INFINITE | FFX_UPSCALE_ENABLE_NON_LINEAR_COLORSPACE;
	impl->createDesc.maxRenderSize = { inputDesc.Width, inputDesc.Height };
	impl->createDesc.maxUpscaleSize = { outputDesc.Width, outputDesc.Height };
	impl->backendDesc.header.type = FFX_API_CREATE_CONTEXT_DESC_TYPE_BACKEND_DX12;
	impl->backendDesc.device = impl->device12.get();
	impl->apiVersion.header.type = FFX_API_CREATE_CONTEXT_DESC_TYPE_UPSCALE_VERSION;
	impl->apiVersion.version = FFX_UPSCALER_VERSION;
	impl->overrideVersion.header.type = FFX_API_DESC_TYPE_OVERRIDE_VERSION;
	impl->overrideVersion.versionId = fsr3VersionId;
	impl->createDesc.header.pNext = &impl->backendDesc.header;
	impl->backendDesc.header.pNext = &impl->apiVersion.header;
	impl->apiVersion.header.pNext = &impl->overrideVersion.header;
	rc = impl->createContext(&impl->context, &impl->createDesc.header, nullptr);
	if (rc != FFX_API_RETURN_OK || !impl->context) {
		Logger::Get().Error(fmt::format("Create FSR 3.1.5 context failed ({})", (uint32_t)rc));
		return false;
	}
	Logger::Get().Info(fmt::format(
		"FSR 3.1.5 D3D11/D3D12 backend initialized (opticalFlow={}, virtual auxiliary inputs): {}x{} -> {}x{}",
		enableOpticalFlow, inputDesc.Width, inputDesc.Height, outputDesc.Width, outputDesc.Height));
	_impl = std::move(impl);
	return true;
}

bool FSR3ZeroMVUpscaler::Resize(DeviceResources& resources, ID3D11Texture2D* input,
	ID3D11Texture2D* output) noexcept {
	return Initialize(resources, input, output, _enableOpticalFlow);
}

bool FSR3ZeroMVUpscaler::Draw(ID3D11Texture2D* input, ID3D11Texture2D* output) noexcept {
	if (!_impl || !_impl->context) return false;
	Impl& impl = *_impl;
	if (!WaitForFence(impl, impl.lastSubmittedValue)) return false;
	impl.context11->CopyResource(impl.sharedInput11.get(), input);
	if (impl.enableOpticalFlow) {
		if (!impl.opticalFlow->Estimate(input)) return false;
		impl.context11->CopyResource(impl.sharedMotion11.get(), impl.opticalFlow->GetMotionTexture());
	}
	const uint64_t inputReady = ++impl.fenceValue;
	HRESULT hr = impl.context11->Signal(impl.fence11.get(), inputReady);
	if (FAILED(hr)) return false;
	impl.context11->Flush();
	hr = impl.queue12->Wait(impl.fence12.get(), inputReady);
	if (FAILED(hr)) return false;

	hr = impl.allocator12->Reset();
	if (SUCCEEDED(hr)) hr = impl.commandList12->Reset(impl.allocator12.get(), nullptr);
	if (FAILED(hr)) return false;
	D3D12_RESOURCE_BARRIER barriers[3]{};
	barriers[0].Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
	barriers[0].Transition = { impl.sharedInput12.get(), D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES,
		D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE };
	barriers[1].Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
	barriers[1].Transition = { impl.sharedOutput12.get(), D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES,
		D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_UNORDERED_ACCESS };
	const UINT barrierCount = impl.enableOpticalFlow ? 3 : 2;
	if (impl.enableOpticalFlow) {
		barriers[2].Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
		barriers[2].Transition = { impl.sharedMotion12.get(), D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES,
			D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE };
	}
	impl.commandList12->ResourceBarrier(barrierCount, barriers);

	ffxDispatchDescUpscale desc{};
	desc.header.type = FFX_API_DISPATCH_DESC_TYPE_UPSCALE;
	desc.commandList = impl.commandList12.get();
	desc.color = ffxApiGetResourceDX12(impl.sharedInput12.get(), FFX_API_RESOURCE_STATE_COMPUTE_READ);
	desc.depth = ffxApiGetResourceDX12(impl.flatDepth12.get(), FFX_API_RESOURCE_STATE_COMPUTE_READ);
	desc.motionVectors = ffxApiGetResourceDX12(
		impl.enableOpticalFlow ? impl.sharedMotion12.get() : impl.zeroMotion12.get(),
		FFX_API_RESOURCE_STATE_COMPUTE_READ);
	desc.exposure = ffxApiGetResourceDX12(impl.exposure12.get(), FFX_API_RESOURCE_STATE_COMPUTE_READ);
	desc.reactive = ffxApiGetResourceDX12(impl.reactive12.get(), FFX_API_RESOURCE_STATE_COMPUTE_READ);
	desc.transparencyAndComposition = ffxApiGetResourceDX12(
		impl.transparency12.get(), FFX_API_RESOURCE_STATE_COMPUTE_READ);
	desc.output = ffxApiGetResourceDX12(impl.sharedOutput12.get(), FFX_API_RESOURCE_STATE_UNORDERED_ACCESS);
	desc.jitterOffset = { 0.0f, 0.0f };
	desc.motionVectorScale = { 1.0f, 1.0f };
	desc.renderSize = { impl.inputWidth, impl.inputHeight };
	desc.upscaleSize = { impl.outputWidth, impl.outputHeight };
	desc.enableSharpening = true;
	desc.sharpness = 0.2f;
	desc.frameTimeDelta = 16.6667f;
	desc.preExposure = 1.0f;
	desc.reset = impl.resetHistory;
	desc.cameraNear = 1.0f;
	desc.cameraFar = FLT_MAX;
	desc.cameraFovAngleVertical = 1.04719755f;
	desc.viewSpaceToMetersFactor = 1.0f;
	desc.flags = FFX_UPSCALE_FLAG_NON_LINEAR_COLOR_SRGB;
	const ffxReturnCode_t rc = impl.dispatch(&impl.context, &desc.header);
	if (rc != FFX_API_RETURN_OK) {
		Logger::Get().Error(fmt::format("Dispatch FSR 3.1.5 failed ({})", (uint32_t)rc));
		return false;
	}
	for (UINT i = 0; i < barrierCount; ++i) {
		std::swap(barriers[i].Transition.StateBefore, barriers[i].Transition.StateAfter);
	}
	impl.commandList12->ResourceBarrier(barrierCount, barriers);
	hr = impl.commandList12->Close();
	if (FAILED(hr)) return false;
	ID3D12CommandList* lists[]{ impl.commandList12.get() };
	impl.queue12->ExecuteCommandLists(1, lists);
	const uint64_t outputReady = ++impl.fenceValue;
	hr = impl.queue12->Signal(impl.fence12.get(), outputReady);
	impl.lastSubmittedValue = outputReady;
	if (SUCCEEDED(hr)) hr = impl.context11->Wait(impl.fence11.get(), outputReady);
	if (FAILED(hr)) return false;
	impl.context11->CopyResource(output, impl.sharedOutput11.get());
	impl.resetHistory = false;
	return true;
}

}

#else

namespace Magpie {
struct FSR3ZeroMVUpscaler::Impl {};
FSR3ZeroMVUpscaler::FSR3ZeroMVUpscaler() = default;
FSR3ZeroMVUpscaler::~FSR3ZeroMVUpscaler() = default;
bool FSR3ZeroMVUpscaler::Initialize(DeviceResources&, ID3D11Texture2D*, ID3D11Texture2D*, bool) noexcept {
	Logger::Get().Error("FSR3 support is not enabled in this build");
	return false;
}
bool FSR3ZeroMVUpscaler::Resize(DeviceResources&, ID3D11Texture2D*, ID3D11Texture2D*) noexcept { return false; }
bool FSR3ZeroMVUpscaler::Draw(ID3D11Texture2D*, ID3D11Texture2D*) noexcept { return false; }
}

#endif
