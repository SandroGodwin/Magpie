#include "pch.h"
#include "XeSSZeroMVUpscaler.h"
#include "DeviceResources.h"
#include "DirectXHelper.h"
#include "HalfResOpticalFlow.h"
#include "Logger.h"

#ifdef MP_ENABLE_XESS_ZEROMV

#include <d3d12.h>
#include <xess/xess.h>
#include <xess/xess_d3d12.h>

namespace Magpie {

struct XeSSZeroMVUpscaler::Impl {
	ID3D11Device5* device11 = nullptr;
	ID3D11DeviceContext4* context11 = nullptr;

	winrt::com_ptr<ID3D12Device> device12;
	winrt::com_ptr<ID3D12CommandQueue> queue12;
	winrt::com_ptr<ID3D12CommandAllocator> allocator12;
	winrt::com_ptr<ID3D12GraphicsCommandList> commandList12;

	winrt::com_ptr<ID3D11Texture2D> sharedInput11;
	winrt::com_ptr<ID3D11Texture2D> sharedOutput11;
	winrt::com_ptr<ID3D11Texture2D> sharedMotion11;

	winrt::com_ptr<ID3D11ShaderResourceView> inputSrv11;
	winrt::com_ptr<ID3D11UnorderedAccessView> sharedInputUav11;
	winrt::com_ptr<ID3D11ComputeShader> colorConvertShader11;

	winrt::com_ptr<ID3D12Resource> sharedInput12;
	winrt::com_ptr<ID3D12Resource> sharedOutput12;
	winrt::com_ptr<ID3D12Resource> sharedMotion12;

	winrt::com_ptr<ID3D12Resource> zeroMotion12;
	winrt::com_ptr<ID3D12Resource> flatDepth12;
	winrt::com_ptr<ID3D12Resource> responsiveMask12;

	/*
	 * Descriptor heaps used only for ClearUnorderedAccessViewFloat.
	 *
	 * CPU heap:
	 *   non-shader-visible
	 *
	 * GPU heap:
	 *   shader-visible
	 *
	 * UAVs are created in the CPU heap and copied into the GPU heap.
	 */
	winrt::com_ptr<ID3D12DescriptorHeap> clearCpuDescriptorHeap12;
	winrt::com_ptr<ID3D12DescriptorHeap> clearGpuDescriptorHeap12;

	winrt::com_ptr<ID3D11Fence> fence11;
	winrt::com_ptr<ID3D12Fence> fence12;

	xess_context_handle_t xessContext = nullptr;

	std::unique_ptr<HalfResOpticalFlow> opticalFlow;

	uint64_t fenceValue = 0;
	uint64_t lastSubmittedValue = 0;

	uint32_t inputWidth = 0;
	uint32_t inputHeight = 0;
	uint32_t outputWidth = 0;
	uint32_t outputHeight = 0;

	bool convertInputToRgba = false;
	bool enableOpticalFlow = false;
	bool enableJitter = false;

	uint32_t frameIndex = 0;
	bool resetHistory = true;
};

static constexpr char COLOR_CONVERT_HLSL[] = R"(
Texture2D<float4> InputColor : register(t0);
RWTexture2D<float4> OutputColor : register(u0);

[numthreads(8, 8, 1)]
void ConvertToRgba(uint3 tid : SV_DispatchThreadID)
{
	uint width, height;
	OutputColor.GetDimensions(width, height);

	if (tid.x >= width || tid.y >= height)
		return;

	OutputColor[tid.xy] = InputColor.Load(int3(tid.xy, 0));
}
)";

static bool XessSucceeded(
	xess_result_t result,
	std::string_view operation
) noexcept {
	if (result == XESS_RESULT_SUCCESS) {
		return true;
	}

	Logger::Get().Error(
		fmt::format(
			"{} failed (XeSS result {})",
			operation,
			static_cast<int>(result)
		)
	);

	return false;
}

static bool WaitForD3D12(
	XeSSZeroMVUpscaler::Impl& impl
) noexcept {
	if (!impl.queue12 || !impl.fence12) {
		return true;
	}

	const uint64_t value =
		++impl.fenceValue;

	HRESULT hr =
		impl.queue12->Signal(
			impl.fence12.get(),
			value
		);

	if (FAILED(hr)) {
		Logger::Get().ComError(
			"Signal XeSS D3D12 fence failed",
			hr
		);
		return false;
	}

	if (impl.fence12->GetCompletedValue() >= value) {
		return true;
	}

	wil::unique_event_nothrow event;

	hr =
		event.create();

	if (FAILED(hr)) {
		Logger::Get().ComError(
			"Create XeSS fence event failed",
			hr
		);
		return false;
	}

	hr =
		impl.fence12->SetEventOnCompletion(
			value,
			event.get()
		);

	if (FAILED(hr)) {
		Logger::Get().ComError(
			"Set XeSS fence event failed",
			hr
		);
		return false;
	}

	WaitForSingleObject(
		event.get(),
		INFINITE
	);

	return true;
}

static bool WaitForFenceValue(
	XeSSZeroMVUpscaler::Impl& impl,
	uint64_t value
) noexcept {
	if (!impl.fence12 || !value) {
		return true;
	}

	if (impl.fence12->GetCompletedValue() >= value) {
		return true;
	}

	wil::unique_event_nothrow event;

	HRESULT hr =
		event.create();

	if (FAILED(hr)) {
		Logger::Get().ComError(
			"Create XeSS completion event failed",
			hr
		);
		return false;
	}

	hr =
		impl.fence12->SetEventOnCompletion(
			value,
			event.get()
		);

	if (FAILED(hr)) {
		Logger::Get().ComError(
			"Set XeSS completion event failed",
			hr
		);
		return false;
	}

	WaitForSingleObject(
		event.get(),
		INFINITE
	);

	return true;
}

static void DestroyImpl(
	std::unique_ptr<XeSSZeroMVUpscaler::Impl>& impl
) noexcept {
	if (!impl) {
		return;
	}

	/*
	 * Ensure all D3D12 work previously submitted through this backend
	 * has completed before destroying the XeSS context/resources.
	 */
	if (impl->queue12 && impl->fence12) {
		WaitForD3D12(*impl);
	}

	if (impl->xessContext) {
		const xess_result_t result =
			xessDestroyContext(
				impl->xessContext
			);

		if (result != XESS_RESULT_SUCCESS) {
			Logger::Get().Error(
				fmt::format(
					"xessDestroyContext failed during cleanup "
					"(XeSS result {})",
					static_cast<int>(result)
				)
			);
		}

		impl->xessContext = nullptr;
	}

	impl.reset();
}

static bool CreateSharedTexture(
	XeSSZeroMVUpscaler::Impl& impl,
	const D3D11_TEXTURE2D_DESC& sourceDesc,
	winrt::com_ptr<ID3D11Texture2D>& texture11,
	winrt::com_ptr<ID3D12Resource>& texture12
) noexcept {
	D3D11_TEXTURE2D_DESC desc =
		sourceDesc;

	/*
	 * Shared resources must be 1 mip, 1 array slice, 1 sample.
	 */
	if (desc.MipLevels != 1 ||
		desc.ArraySize != 1 ||
		desc.SampleDesc.Count != 1) {
		Logger::Get().Error(
			fmt::format(
				"XeSS shared texture requires 1 mip, "
				"1 array slice and 1 sample "
				"(mips={}, arrays={}, samples={})",
				desc.MipLevels,
				desc.ArraySize,
				desc.SampleDesc.Count
			)
		);

		return false;
	}

	desc.Usage =
		D3D11_USAGE_DEFAULT;

	desc.CPUAccessFlags =
		0;

	/*
	 * Preserve the D3D11/D3D12 interop configuration that worked.
	 */
	desc.BindFlags =
		D3D11_BIND_SHADER_RESOURCE |
		D3D11_BIND_UNORDERED_ACCESS;

	desc.MiscFlags =
		D3D11_RESOURCE_MISC_SHARED |
		D3D11_RESOURCE_MISC_SHARED_NTHANDLE;

	HRESULT hr =
		impl.device11->CreateTexture2D(
			&desc,
			nullptr,
			texture11.put()
		);

	if (FAILED(hr)) {
		Logger::Get().ComError(
			"Create XeSS shared D3D11 texture failed",
			hr
		);
		return false;
	}

	winrt::com_ptr<IDXGIResource1> dxgiResource;

	hr =
		texture11->QueryInterface(
			IID_PPV_ARGS(
				dxgiResource.put()
			)
		);

	if (FAILED(hr)) {
		Logger::Get().ComError(
			"Query XeSS shared IDXGIResource1 failed",
			hr
		);
		return false;
	}

	HANDLE rawHandle =
		nullptr;

	hr =
		dxgiResource->CreateSharedHandle(
			nullptr,
			GENERIC_ALL,
			nullptr,
			&rawHandle
		);

	if (FAILED(hr)) {
		Logger::Get().ComError(
			"Create XeSS texture shared handle failed",
			hr
		);
		return false;
	}

	wil::unique_handle sharedHandle(
		rawHandle
	);

	hr =
		impl.device12->OpenSharedHandle(
			sharedHandle.get(),
			IID_PPV_ARGS(
				texture12.put()
			)
		);

	if (FAILED(hr)) {
		Logger::Get().ComError(
			"Open XeSS texture in D3D12 failed",
			hr
		);
		return false;
	}

	if (!texture12) {
		Logger::Get().Error(
			"Open XeSS texture in D3D12 returned null resource"
		);
		return false;
	}

	return true;
}

static xess_quality_settings_t SelectQuality(
	float scale
) noexcept {
	/*
	 * Keep the quality mapping from the known-working version.
	 */
	if (scale <= 1.5f) {
		return XESS_QUALITY_SETTING_ULTRA_QUALITY;
	}

	if (scale <= 1.7f) {
		return XESS_QUALITY_SETTING_QUALITY;
	}

	if (scale <= 2.0f) {
		return XESS_QUALITY_SETTING_BALANCED;
	}

	if (scale <= 2.3f) {
		return XESS_QUALITY_SETTING_PERFORMANCE;
	}

	return XESS_QUALITY_SETTING_ULTRA_PERFORMANCE;
}

XeSSZeroMVUpscaler::XeSSZeroMVUpscaler() = default;

XeSSZeroMVUpscaler::~XeSSZeroMVUpscaler() {
	DestroyImpl(_impl);
}

bool XeSSZeroMVUpscaler::Initialize(
	DeviceResources& deviceResources,
	ID3D11Texture2D* input,
	ID3D11Texture2D* output,
	bool enableOpticalFlow,
	bool enableJitter
) noexcept {
	if (!input || !output) {
		Logger::Get().Error(
			"XeSS Initialize received a null input/output texture"
		);

		return false;
	}

	/*
	 * Preserve BOTH feature switches exactly as requested by the caller.
	 */
	_enableOpticalFlow =
		enableOpticalFlow;

	_enableJitter =
		enableJitter;

	/*
	 * Safely destroy previous backend instance.
	 */
	DestroyImpl(_impl);

	auto impl =
		std::make_unique<Impl>();

	impl->enableOpticalFlow =
		enableOpticalFlow;

	impl->enableJitter =
		enableJitter;

	impl->device11 =
		deviceResources.GetD3DDevice();

	impl->context11 =
		deviceResources.GetD3DDC();

	if (!impl->device11 ||
		!impl->context11) {
		Logger::Get().Error(
			"XeSS could not obtain the D3D11 device/context"
		);

		return false;
	}

	D3D11_TEXTURE2D_DESC inputDesc{};
	D3D11_TEXTURE2D_DESC outputDesc{};

	input->GetDesc(
		&inputDesc
	);

	output->GetDesc(
		&outputDesc
	);

	if (inputDesc.Width == 0 ||
		inputDesc.Height == 0 ||
		outputDesc.Width == 0 ||
		outputDesc.Height == 0) {
		Logger::Get().Error(
			"XeSS received a zero-sized texture"
		);

		return false;
	}

	if (inputDesc.Width > outputDesc.Width ||
		inputDesc.Height > outputDesc.Height) {
		Logger::Get().Error(
			fmt::format(
				"XeSS Zero-MV only supports upscaling: "
				"{}x{} -> {}x{}",
				inputDesc.Width,
				inputDesc.Height,
				outputDesc.Width,
				outputDesc.Height
			)
		);

		return false;
	}

	const bool supportedInputFormat =
		inputDesc.Format ==
			DXGI_FORMAT_R8G8B8A8_UNORM ||
		inputDesc.Format ==
			DXGI_FORMAT_B8G8R8A8_UNORM;

	if (!supportedInputFormat ||
		outputDesc.Format !=
			DXGI_FORMAT_R8G8B8A8_UNORM) {
		Logger::Get().Error(
			fmt::format(
				"XeSS Zero-MV unsupported texture formats: "
				"input={}, output={}",
				static_cast<uint32_t>(
					inputDesc.Format
				),
				static_cast<uint32_t>(
					outputDesc.Format
				)
			)
		);

		return false;
	}

	const float scaleX =
		static_cast<float>(
			outputDesc.Width
		) /
		static_cast<float>(
			inputDesc.Width
		);

	const float scaleY =
		static_cast<float>(
			outputDesc.Height
		) /
		static_cast<float>(
			inputDesc.Height
		);

	const float scale =
		(std::max)(
			scaleX,
			scaleY
		);

	if (scale > 3.0f) {
		Logger::Get().Error(
			"XeSS Zero-MV supports up to a 3x scale"
		);

		return false;
	}

	const float scaleDifference =
		std::abs(scaleX - scaleY);

	if (scaleDifference > 0.001f) {
		Logger::Get().Error(
			fmt::format(
				"XeSS Zero-MV requires uniform scaling: "
				"scaleX={}, scaleY={}",
				scaleX,
				scaleY
			)
		);

		return false;
	}

	impl->inputWidth =
		inputDesc.Width;

	impl->inputHeight =
		inputDesc.Height;

	impl->outputWidth =
		outputDesc.Width;

	impl->outputHeight =
		outputDesc.Height;

	impl->convertInputToRgba =
		inputDesc.Format ==
			DXGI_FORMAT_B8G8R8A8_UNORM;

	/*
	 * D3D12 device on the same adapter selected by Magpie.
	 */
	HRESULT hr =
		D3D12CreateDevice(
			deviceResources.GetGraphicsAdapter(),
			D3D_FEATURE_LEVEL_11_0,
			IID_PPV_ARGS(
				impl->device12.put()
			)
		);

	if (FAILED(hr)) {
		Logger::Get().ComError(
			"Create XeSS D3D12 device failed",
			hr
		);

		return false;
	}

	D3D12_FEATURE_DATA_SHADER_MODEL shaderModel{
		D3D_SHADER_MODEL_6_4
	};

	hr =
		impl->device12->CheckFeatureSupport(
			D3D12_FEATURE_SHADER_MODEL,
			&shaderModel,
			sizeof(shaderModel)
		);

	if (FAILED(hr) ||
		shaderModel.HighestShaderModel <
			D3D_SHADER_MODEL_6_4) {
		Logger::Get().Error(
			"XeSS cross-vendor path requires "
			"Shader Model 6.4 / DP4a support"
		);

		return false;
	}

	D3D12_COMMAND_QUEUE_DESC queueDesc{};

	queueDesc.Type =
		D3D12_COMMAND_LIST_TYPE_DIRECT;

	hr =
		impl->device12->CreateCommandQueue(
			&queueDesc,
			IID_PPV_ARGS(
				impl->queue12.put()
			)
		);

	if (SUCCEEDED(hr)) {
		hr =
			impl->device12->CreateCommandAllocator(
				D3D12_COMMAND_LIST_TYPE_DIRECT,
				IID_PPV_ARGS(
					impl->allocator12.put()
				)
			);
	}

	if (SUCCEEDED(hr)) {
		hr =
			impl->device12->CreateCommandList(
				0,
				D3D12_COMMAND_LIST_TYPE_DIRECT,
				impl->allocator12.get(),
				nullptr,
				IID_PPV_ARGS(
					impl->commandList12.put()
				)
			);
	}

	if (FAILED(hr)) {
		Logger::Get().ComError(
			"Create XeSS D3D12 command objects failed",
			hr
		);

		return false;
	}

	/*
	 * CreateCommandList returns OPEN.
	 *
	 * It will remain open through initialization so the initial clears can
	 * be recorded without Reset().
	 */

	/*
	 * Internal RGBA input.
	 */
	D3D11_TEXTURE2D_DESC xessInputDesc =
		inputDesc;

	xessInputDesc.Format =
		DXGI_FORMAT_R8G8B8A8_UNORM;

	xessInputDesc.BindFlags =
		D3D11_BIND_SHADER_RESOURCE |
		D3D11_BIND_UNORDERED_ACCESS;

	/*
	 * Internal RGBA output.
	 */
	D3D11_TEXTURE2D_DESC xessOutputDesc =
		outputDesc;

	xessOutputDesc.Format =
		DXGI_FORMAT_R8G8B8A8_UNORM;

	xessOutputDesc.BindFlags =
		D3D11_BIND_SHADER_RESOURCE |
		D3D11_BIND_UNORDERED_ACCESS;

	if (!CreateSharedTexture(
			*impl,
			xessInputDesc,
			impl->sharedInput11,
			impl->sharedInput12) ||
		!CreateSharedTexture(
			*impl,
			xessOutputDesc,
			impl->sharedOutput11,
			impl->sharedOutput12)) {
		return false;
	}

	/*
	 * BGRA -> RGBA conversion.
	 */
	if (impl->convertInputToRgba) {
		hr =
			impl->device11->CreateShaderResourceView(
				input,
				nullptr,
				impl->inputSrv11.put()
			);

		if (SUCCEEDED(hr)) {
			hr =
				impl->device11->CreateUnorderedAccessView(
					impl->sharedInput11.get(),
					nullptr,
					impl->sharedInputUav11.put()
				);
		}

		winrt::com_ptr<ID3DBlob> shaderBlob;

		if (SUCCEEDED(hr) &&
			!DirectXHelper::CompileComputeShader(
				COLOR_CONVERT_HLSL,
				"ConvertToRgba",
				shaderBlob.put(),
				"XeSSColorConvert")) {
			hr = E_FAIL;
		}

		if (SUCCEEDED(hr)) {
			hr =
				impl->device11->CreateComputeShader(
					shaderBlob->GetBufferPointer(),
					shaderBlob->GetBufferSize(),
					nullptr,
					impl->colorConvertShader11.put()
				);
		}

		if (FAILED(hr)) {
			Logger::Get().ComError(
				"Create XeSS BGRA-to-RGBA conversion resources failed",
				hr
			);

			return false;
		}
	}

	D3D12_HEAP_PROPERTIES heapProperties{};

	heapProperties.Type =
		D3D12_HEAP_TYPE_DEFAULT;

	/*
	 * ============================================================
	 * Optical Flow / Zero-MV motion resources
	 * ============================================================
	 */
	if (impl->enableOpticalFlow) {
		/*
		 * XeSS Optical Flow variant.
		 */
		impl->opticalFlow =
			std::make_unique<HalfResOpticalFlow>();

		if (!impl->opticalFlow->Initialize(
				impl->device11,
				impl->context11,
				input)) {
			Logger::Get().Error(
				"Initialize XeSS 50% optical flow failed"
			);

			return false;
		}

		D3D11_TEXTURE2D_DESC motionDesc11{};

		motionDesc11.Width =
			inputDesc.Width;

		motionDesc11.Height =
			inputDesc.Height;

		motionDesc11.MipLevels =
			1;

		motionDesc11.ArraySize =
			1;

		motionDesc11.Format =
			DXGI_FORMAT_R16G16_FLOAT;

		motionDesc11.SampleDesc.Count =
			1;

		motionDesc11.BindFlags =
			D3D11_BIND_SHADER_RESOURCE;

		if (!CreateSharedTexture(
				*impl,
				motionDesc11,
				impl->sharedMotion11,
				impl->sharedMotion12)) {
			return false;
		}
	}
	else {
		/*
		 * XeSS Zero-MV variant.
		 *
		 * HIGH_RES_MV uses target/output-resolution motion vectors.
		 */
		D3D12_RESOURCE_DESC motionDesc{};

		motionDesc.Dimension =
			D3D12_RESOURCE_DIMENSION_TEXTURE2D;

		motionDesc.Width =
			outputDesc.Width;

		motionDesc.Height =
			outputDesc.Height;

		motionDesc.DepthOrArraySize =
			1;

		motionDesc.MipLevels =
			1;

		motionDesc.Format =
			DXGI_FORMAT_R16G16_FLOAT;

		motionDesc.SampleDesc.Count =
			1;

		motionDesc.Layout =
			D3D12_TEXTURE_LAYOUT_UNKNOWN;

		motionDesc.Flags =
			D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;

		hr =
			impl->device12->CreateCommittedResource(
				&heapProperties,
				D3D12_HEAP_FLAG_NONE,
				&motionDesc,
				D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
				nullptr,
				IID_PPV_ARGS(
					impl->zeroMotion12.put()
				)
			);

		if (FAILED(hr)) {
			Logger::Get().ComError(
				"Create XeSS zero motion-vector texture failed",
				hr
			);

			return false;
		}
	}

	/*
	 * Flat depth at input resolution.
	 */
	D3D12_RESOURCE_DESC depthDesc{};

	depthDesc.Dimension =
		D3D12_RESOURCE_DIMENSION_TEXTURE2D;

	depthDesc.Width =
		inputDesc.Width;

	depthDesc.Height =
		inputDesc.Height;

	depthDesc.DepthOrArraySize =
		1;

	depthDesc.MipLevels =
		1;

	depthDesc.Format =
		DXGI_FORMAT_R32_FLOAT;

	depthDesc.SampleDesc.Count =
		1;

	depthDesc.Layout =
		D3D12_TEXTURE_LAYOUT_UNKNOWN;

	depthDesc.Flags =
		D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;

	hr =
		impl->device12->CreateCommittedResource(
			&heapProperties,
			D3D12_HEAP_FLAG_NONE,
			&depthDesc,
			D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
			nullptr,
			IID_PPV_ARGS(
				impl->flatDepth12.put()
			)
		);

	if (FAILED(hr)) {
		Logger::Get().ComError(
			"Create XeSS flat depth texture failed",
			hr
		);

		return false;
	}

	/*
	 * Responsive pixel mask.
	 */
	D3D12_RESOURCE_DESC responsiveMaskDesc =
		depthDesc;

	responsiveMaskDesc.Format =
		DXGI_FORMAT_R8_UNORM;

	hr =
		impl->device12->CreateCommittedResource(
			&heapProperties,
			D3D12_HEAP_FLAG_NONE,
			&responsiveMaskDesc,
			D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
			nullptr,
			IID_PPV_ARGS(
				impl->responsiveMask12.put()
			)
		);

	if (FAILED(hr)) {
		Logger::Get().ComError(
			"Create XeSS responsive pixel mask failed",
			hr
		);

		return false;
	}

	/*
	 * ============================================================
	 * Shared fence
	 * ============================================================
	 */
	hr =
		impl->device11->CreateFence(
			0,
			D3D11_FENCE_FLAG_SHARED,
			IID_PPV_ARGS(
				impl->fence11.put()
			)
		);

	if (FAILED(hr)) {
		Logger::Get().ComError(
			"Create XeSS shared fence failed",
			hr
		);

		return false;
	}

	HANDLE rawFenceHandle =
		nullptr;

	hr =
		impl->fence11->CreateSharedHandle(
			nullptr,
			GENERIC_ALL,
			nullptr,
			&rawFenceHandle
		);

	if (FAILED(hr)) {
		Logger::Get().ComError(
			"Create XeSS fence shared handle failed",
			hr
		);

		return false;
	}

	wil::unique_handle fenceHandle(
		rawFenceHandle
	);

	hr =
		impl->device12->OpenSharedHandle(
			fenceHandle.get(),
			IID_PPV_ARGS(
				impl->fence12.put()
			)
		);

	if (FAILED(hr)) {
		Logger::Get().ComError(
			"Open XeSS fence in D3D12 failed",
			hr
		);

		return false;
	}

	/*
	 * Synchronize before using the initialization queue.
	 */
	if (!WaitForD3D12(*impl)) {
		return false;
	}

	/*
	 * ============================================================
	 * XeSS context
	 * ============================================================
	 */
	const xess_result_t createResult =
		xessD3D12CreateContext(
			impl->device12.get(),
			&impl->xessContext
		);

	if (!XessSucceeded(
			createResult,
			"xessD3D12CreateContext")) {
		return false;
	}

	const xess_quality_settings_t quality =
		SelectQuality(scale);

	xess_d3d12_init_params_t initParams{};

	initParams.outputResolution = {
		outputDesc.Width,
		outputDesc.Height
	};

	initParams.qualitySetting =
		quality;

	initParams.initFlags =
		XESS_INIT_FLAG_LDR_INPUT_COLOR |
		XESS_INIT_FLAG_RESPONSIVE_PIXEL_MASK;

	if (!impl->enableOpticalFlow) {
		initParams.initFlags |=
			XESS_INIT_FLAG_HIGH_RES_MV;
	}

	if (!XessSucceeded(
			xessD3D12Init(
				impl->xessContext,
				&initParams
			),
			"xessD3D12Init")) {

		xessDestroyContext(
			impl->xessContext
		);

		impl->xessContext =
			nullptr;

		return false;
	}

	if (!XessSucceeded(
			xessSetVelocityScale(
				impl->xessContext,
				1.0f,
				1.0f
			),
			"xessSetVelocityScale")) {

		xessDestroyContext(
			impl->xessContext
		);

		impl->xessContext =
			nullptr;

		return false;
	}

	/*
	 * ============================================================
	 * Descriptor heaps for initialization clears
	 * ============================================================
	 *
	 * CPU heap:
	 *   non-shader-visible
	 *
	 * GPU heap:
	 *   shader-visible
	 *
	 * UAVs are created only in the CPU heap. They are then copied
	 * into the GPU heap.
	 */
	D3D12_DESCRIPTOR_HEAP_DESC cpuHeapDesc{};

	cpuHeapDesc.Type =
		D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;

	cpuHeapDesc.NumDescriptors =
		3;

	cpuHeapDesc.Flags =
		D3D12_DESCRIPTOR_HEAP_FLAG_NONE;

	hr =
		impl->device12->CreateDescriptorHeap(
			&cpuHeapDesc,
			IID_PPV_ARGS(
				impl->clearCpuDescriptorHeap12.put()
			)
		);

	if (FAILED(hr)) {
		Logger::Get().ComError(
			"Create XeSS CPU clear descriptor heap failed",
			hr
		);

		xessDestroyContext(
			impl->xessContext
		);

		impl->xessContext =
			nullptr;

		return false;
	}

	D3D12_DESCRIPTOR_HEAP_DESC gpuHeapDesc{};

	gpuHeapDesc.Type =
		D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;

	gpuHeapDesc.NumDescriptors =
		3;

	gpuHeapDesc.Flags =
		D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;

	hr =
		impl->device12->CreateDescriptorHeap(
			&gpuHeapDesc,
			IID_PPV_ARGS(
				impl->clearGpuDescriptorHeap12.put()
			)
		);

	if (FAILED(hr)) {
		Logger::Get().ComError(
			"Create XeSS GPU clear descriptor heap failed",
			hr
		);

		xessDestroyContext(
			impl->xessContext
		);

		impl->xessContext =
			nullptr;

		return false;
	}

	const UINT descriptorSize =
		impl->device12->GetDescriptorHandleIncrementSize(
			D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV
		);

	D3D12_CPU_DESCRIPTOR_HANDLE cpuStart =
		impl->clearCpuDescriptorHeap12
			->GetCPUDescriptorHandleForHeapStart();

	D3D12_CPU_DESCRIPTOR_HANDLE gpuCpuStart =
		impl->clearGpuDescriptorHeap12
			->GetCPUDescriptorHandleForHeapStart();

	D3D12_GPU_DESCRIPTOR_HANDLE gpuStart =
		impl->clearGpuDescriptorHeap12
			->GetGPUDescriptorHandleForHeapStart();

	/*
	 * ============================================================
	 * Descriptor 0: flat depth UAV
	 * ============================================================
	 */
	D3D12_UNORDERED_ACCESS_VIEW_DESC depthUav{};

	depthUav.Format =
		DXGI_FORMAT_R32_FLOAT;

	depthUav.ViewDimension =
		D3D12_UAV_DIMENSION_TEXTURE2D;

	/*
	 * Create UAV only in CPU-visible/non-shader-visible storage heap.
	 */
	impl->device12->CreateUnorderedAccessView(
		impl->flatDepth12.get(),
		nullptr,
		&depthUav,
		cpuStart
	);

	/*
	 * Copy the initialized descriptor into the shader-visible heap.
	 */
	impl->device12->CopyDescriptorsSimple(
		1,
		gpuCpuStart,
		cpuStart,
		D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV
	);

	/*
	 * ============================================================
	 * Descriptor 1: responsive mask UAV
	 * ============================================================
	 */
	D3D12_CPU_DESCRIPTOR_HANDLE responsiveCpu =
		cpuStart;

	responsiveCpu.ptr +=
		descriptorSize;

	D3D12_CPU_DESCRIPTOR_HANDLE responsiveGpuCpu =
		gpuCpuStart;

	responsiveGpuCpu.ptr +=
		descriptorSize;

	D3D12_GPU_DESCRIPTOR_HANDLE responsiveGpu =
		gpuStart;

	responsiveGpu.ptr +=
		descriptorSize;

	D3D12_UNORDERED_ACCESS_VIEW_DESC maskUav{};

	maskUav.Format =
		DXGI_FORMAT_R8_UNORM;

	maskUav.ViewDimension =
		D3D12_UAV_DIMENSION_TEXTURE2D;

	impl->device12->CreateUnorderedAccessView(
		impl->responsiveMask12.get(),
		nullptr,
		&maskUav,
		responsiveCpu
	);

	impl->device12->CopyDescriptorsSimple(
		1,
		responsiveGpuCpu,
		responsiveCpu,
		D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV
	);

	/*
	 * ============================================================
	 * Descriptor 2: zero-motion UAV
	 * ============================================================
	 */
	D3D12_CPU_DESCRIPTOR_HANDLE motionCpu =
		cpuStart;

	motionCpu.ptr +=
		descriptorSize * 2;

	D3D12_CPU_DESCRIPTOR_HANDLE motionGpuCpu =
		gpuCpuStart;

	motionGpuCpu.ptr +=
		descriptorSize * 2;

	D3D12_GPU_DESCRIPTOR_HANDLE motionGpu =
		gpuStart;

	motionGpu.ptr +=
		descriptorSize * 2;

	if (!impl->enableOpticalFlow) {
		D3D12_UNORDERED_ACCESS_VIEW_DESC motionUav{};

		motionUav.Format =
			DXGI_FORMAT_R16G16_FLOAT;

		motionUav.ViewDimension =
			D3D12_UAV_DIMENSION_TEXTURE2D;

		impl->device12->CreateUnorderedAccessView(
			impl->zeroMotion12.get(),
			nullptr,
			&motionUav,
			motionCpu
		);

		impl->device12->CopyDescriptorsSimple(
			1,
			motionGpuCpu,
			motionCpu,
			D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV
		);
	}

	/*
	 * Bind the shader-visible heap to the command list.
	 */
	ID3D12DescriptorHeap* clearHeaps[] = {
		impl->clearGpuDescriptorHeap12.get()
	};

	impl->commandList12->SetDescriptorHeaps(
		1,
		clearHeaps
	);

	/*
	 * ============================================================
	 * Initial clears
	 * ============================================================
	 */

	/*
	 * flat depth = 1.0
	 */
	const float flatDepthValue[4]{
		1.0f,
		1.0f,
		1.0f,
		1.0f
	};

	impl->commandList12->ClearUnorderedAccessViewFloat(
		gpuStart,
		cpuStart,
		impl->flatDepth12.get(),
		flatDepthValue,
		0,
		nullptr
	);

	/*
	 * responsive mask = 0.5
	 */
	const float responsiveValue[4]{
		0.5f,
		0.5f,
		0.5f,
		0.5f
	};

	impl->commandList12->ClearUnorderedAccessViewFloat(
		responsiveGpu,
		responsiveCpu,
		impl->responsiveMask12.get(),
		responsiveValue,
		0,
		nullptr
	);

	/*
	 * zero motion = 0
	 */
	if (!impl->enableOpticalFlow) {
		const float zeroMotionValue[4]{};

		impl->commandList12->ClearUnorderedAccessViewFloat(
			motionGpu,
			motionCpu,
			impl->zeroMotion12.get(),
			zeroMotionValue,
			0,
			nullptr
		);
	}

	/*
	 * ============================================================
	 * Initial resource transitions
	 * ============================================================
	 */
	D3D12_RESOURCE_BARRIER auxiliaryBarriers[3]{};

	auxiliaryBarriers[0].Type =
		D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;

	auxiliaryBarriers[0].Transition.pResource =
		impl->flatDepth12.get();

	auxiliaryBarriers[0].Transition.StateBefore =
		D3D12_RESOURCE_STATE_UNORDERED_ACCESS;

	auxiliaryBarriers[0].Transition.StateAfter =
		D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE;

	auxiliaryBarriers[0].Transition.Subresource =
		D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;

	auxiliaryBarriers[1] =
		auxiliaryBarriers[0];

	auxiliaryBarriers[1].Transition.pResource =
		impl->responsiveMask12.get();

	UINT auxiliaryBarrierCount =
		2;

	if (!impl->enableOpticalFlow) {
		auxiliaryBarriers[2] =
			auxiliaryBarriers[0];

		auxiliaryBarriers[2].Transition.pResource =
			impl->zeroMotion12.get();

		auxiliaryBarrierCount =
			3;
	}

	impl->commandList12->ResourceBarrier(
		auxiliaryBarrierCount,
		auxiliaryBarriers
	);

	/*
	 * CreateCommandList() returned OPEN.
	 * This is its first Close().
	 */
	hr =
		impl->commandList12->Close();

	if (FAILED(hr)) {
		Logger::Get().ComError(
			"Close XeSS initialization command list failed",
			hr
		);

		xessDestroyContext(
			impl->xessContext
		);

		impl->xessContext =
			nullptr;

		return false;
	}

	ID3D12CommandList* initializationLists[] = {
		impl->commandList12.get()
	};

	impl->queue12->ExecuteCommandLists(
		1,
		initializationLists
	);

	/*
	 * Wait until the initialization clear/transition work is complete.
	 */
	if (!WaitForD3D12(*impl)) {
		xessDestroyContext(
			impl->xessContext
		);

		impl->xessContext =
			nullptr;

		return false;
	}

	Logger::Get().Info(
		fmt::format(
			"XeSS experimental D3D11/D3D12 backend initialized "
			"(quality {}, {}, jitter={}, BGRA conversion={}): "
			"{}x{} -> {}x{}",
			static_cast<int>(quality),
			impl->enableOpticalFlow
				? "OpticalFlow-lowresMV"
				: "Zero-MV-highresMV",
			impl->enableJitter
				? "on"
				: "off",
			impl->convertInputToRgba,
			inputDesc.Width,
			inputDesc.Height,
			outputDesc.Width,
			outputDesc.Height
		)
	);

	_impl =
		std::move(impl);

	return true;
}

bool XeSSZeroMVUpscaler::Resize(
	DeviceResources& deviceResources,
	ID3D11Texture2D* input,
	ID3D11Texture2D* output
) noexcept {
	/*
	 * Preserve BOTH variant switches.
	 */
	return Initialize(
		deviceResources,
		input,
		output,
		_enableOpticalFlow,
		_enableJitter
	);
}

static float Halton(
	uint32_t index,
	uint32_t base
) noexcept {
	float result = 0.0f;
	float fraction = 1.0f;

	while (index) {
		fraction /=
			static_cast<float>(base);

		result +=
			fraction *
			static_cast<float>(
				index % base
			);

		index /=
			base;
	}

	return result;
}

bool XeSSZeroMVUpscaler::Draw(
	const NativeEffectDrawContext& drawContext
) noexcept {
	ID3D11Texture2D* input =
		drawContext.input;

	ID3D11Texture2D* output =
		drawContext.output;

	if (!input || !output) {
		Logger::Get().Error(
			"XeSS Draw received a null input/output texture"
		);

		return false;
	}

	if (!_impl ||
		!_impl->xessContext ||
		!_impl->device11 ||
		!_impl->context11 ||
		!_impl->device12 ||
		!_impl->queue12 ||
		!_impl->allocator12 ||
		!_impl->commandList12 ||
		!_impl->fence11 ||
		!_impl->fence12) {
		return false;
	}

	Impl& impl =
		*_impl;

	/*
	 * Do not reuse the D3D12 allocator until the previous submission
	 * has completed.
	 */
	if (!WaitForFenceValue(
			impl,
			impl.lastSubmittedValue)) {
		return false;
	}

	/*
	 * Upload source pixels.
	 */
	if (impl.convertInputToRgba) {
		ID3D11ShaderResourceView* inputSrv =
			impl.inputSrv11.get();

		ID3D11UnorderedAccessView* outputUav =
			impl.sharedInputUav11.get();

		impl.context11->CSSetShader(
			impl.colorConvertShader11.get(),
			nullptr,
			0
		);

		impl.context11->CSSetShaderResources(
			0,
			1,
			&inputSrv
		);

		impl.context11->CSSetUnorderedAccessViews(
			0,
			1,
			&outputUav,
			nullptr
		);

		impl.context11->Dispatch(
			(impl.inputWidth + 7) / 8,
			(impl.inputHeight + 7) / 8,
			1
		);

		ID3D11ShaderResourceView* nullSrv =
			nullptr;

		ID3D11UnorderedAccessView* nullUav =
			nullptr;

		impl.context11->CSSetShaderResources(
			0,
			1,
			&nullSrv
		);

		impl.context11->CSSetUnorderedAccessViews(
			0,
			1,
			&nullUav,
			nullptr
		);

		impl.context11->CSSetShader(
			nullptr,
			nullptr,
			0
		);
	}
	else {
		impl.context11->CopyResource(
			impl.sharedInput11.get(),
			input
		);
	}

	/*
	 * ============================================================
	 * Optical Flow
	 * ============================================================
	 */
	if (impl.enableOpticalFlow) {
		if (!impl.opticalFlow) {
			Logger::Get().Error(
				"XeSS optical flow object is unexpectedly null"
			);

			return false;
		}

		if (!impl.opticalFlow->Estimate(
				input)) {
			Logger::Get().Error(
				"Estimate XeSS 50% optical flow failed"
			);

			return false;
		}

		impl.context11->CopyResource(
			impl.sharedMotion11.get(),
			impl.opticalFlow->GetMotionTexture()
		);
	}

	/*
	 * D3D11 -> D3D12 synchronization.
	 */
	const uint64_t inputReady =
		++impl.fenceValue;

	HRESULT hr =
		impl.context11->Signal(
			impl.fence11.get(),
			inputReady
		);

	if (FAILED(hr)) {
		Logger::Get().ComError(
			"Signal XeSS input-ready fence failed",
			hr
		);

		return false;
	}

	impl.context11->Flush();

	hr =
		impl.queue12->Wait(
			impl.fence12.get(),
			inputReady
		);

	if (FAILED(hr)) {
		Logger::Get().ComError(
			"Wait for XeSS D3D11 input failed",
			hr
		);

		return false;
	}

	/*
	 * Initialization command list is closed now, so Reset is legal.
	 */
	hr =
		impl.allocator12->Reset();

	if (SUCCEEDED(hr)) {
		hr =
			impl.commandList12->Reset(
				impl.allocator12.get(),
				nullptr
			);
	}

	if (FAILED(hr)) {
		Logger::Get().ComError(
			"Reset XeSS command list failed",
			hr
		);

		return false;
	}

	D3D12_RESOURCE_BARRIER barriers[3]{};

	/*
	 * Shared input:
	 * COMMON -> NON_PIXEL_SHADER_RESOURCE
	 */
	barriers[0].Type =
		D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;

	barriers[0].Transition.pResource =
		impl.sharedInput12.get();

	barriers[0].Transition.StateBefore =
		D3D12_RESOURCE_STATE_COMMON;

	barriers[0].Transition.StateAfter =
		D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE;

	barriers[0].Transition.Subresource =
		D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;

	/*
	 * Shared output:
	 * COMMON -> UNORDERED_ACCESS
	 */
	barriers[1].Type =
		D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;

	barriers[1].Transition.pResource =
		impl.sharedOutput12.get();

	barriers[1].Transition.StateBefore =
		D3D12_RESOURCE_STATE_COMMON;

	barriers[1].Transition.StateAfter =
		D3D12_RESOURCE_STATE_UNORDERED_ACCESS;

	barriers[1].Transition.Subresource =
		D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;

	UINT barrierCount =
		2;

	/*
	 * Optical Flow motion:
	 * COMMON -> NON_PIXEL_SHADER_RESOURCE
	 */
	if (impl.enableOpticalFlow) {
		barriers[2].Type =
			D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;

		barriers[2].Transition.pResource =
			impl.sharedMotion12.get();

		barriers[2].Transition.StateBefore =
			D3D12_RESOURCE_STATE_COMMON;

		barriers[2].Transition.StateAfter =
			D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE;

		barriers[2].Transition.Subresource =
			D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;

		barrierCount =
			3;
	}

	impl.commandList12->ResourceBarrier(
		barrierCount,
		barriers
	);

	/*
	 * ============================================================
	 * XeSS execution
	 * ============================================================
	 */
	xess_d3d12_execute_params_t params{};

	params.pColorTexture =
		impl.sharedInput12.get();

	params.pVelocityTexture =
		impl.enableOpticalFlow
			? impl.sharedMotion12.get()
			: impl.zeroMotion12.get();

	params.pDepthTexture =
		impl.flatDepth12.get();

	params.pResponsivePixelMaskTexture =
		impl.responsiveMask12.get();

	params.pOutputTexture =
		impl.sharedOutput12.get();

	/*
	 * ============================================================
	 * Jitter
	 * ============================================================
	 *
	 * Restored to the original behavior.
	 */
	if (impl.enableJitter) {
		const uint32_t sample =
			(impl.frameIndex++ & 7u) + 1u;

		params.jitterOffsetX =
			Halton(sample, 2) - 0.5f;

		params.jitterOffsetY =
			Halton(sample, 3) - 0.5f;
	}
	else {
		params.jitterOffsetX =
			0.0f;

		params.jitterOffsetY =
			0.0f;
	}

	params.exposureScale =
		1.0f;

	params.resetHistory =
		impl.resetHistory
			? 1u
			: 0u;

	params.inputWidth =
		impl.inputWidth;

	params.inputHeight =
		impl.inputHeight;

	const xess_result_t result =
		xessD3D12Execute(
			impl.xessContext,
			impl.commandList12.get(),
			&params
		);

	if (!XessSucceeded(
			result,
			"xessD3D12Execute")) {
		return false;
	}

	/*
	 * Return shared resources to COMMON.
	 */
	std::swap(
		barriers[0].Transition.StateBefore,
		barriers[0].Transition.StateAfter
	);

	std::swap(
		barriers[1].Transition.StateBefore,
		barriers[1].Transition.StateAfter
	);

	if (impl.enableOpticalFlow) {
		std::swap(
			barriers[2].Transition.StateBefore,
			barriers[2].Transition.StateAfter
		);
	}

	impl.commandList12->ResourceBarrier(
		barrierCount,
		barriers
	);

	hr =
		impl.commandList12->Close();

	if (FAILED(hr)) {
		Logger::Get().ComError(
			"Close XeSS command list failed",
			hr
		);

		return false;
	}

	ID3D12CommandList* lists[] = {
		impl.commandList12.get()
	};

	impl.queue12->ExecuteCommandLists(
		1,
		lists
	);

	/*
	 * Signal D3D12 -> D3D11.
	 */
	const uint64_t outputReady =
		++impl.fenceValue;

	hr =
		impl.queue12->Signal(
			impl.fence12.get(),
			outputReady
		);

	if (FAILED(hr)) {
		Logger::Get().ComError(
			"Signal XeSS output-ready fence failed",
			hr
		);

		return false;
	}

	impl.lastSubmittedValue =
		outputReady;

	hr =
		impl.context11->Wait(
			impl.fence11.get(),
			outputReady
		);

	if (FAILED(hr)) {
		Logger::Get().ComError(
			"Synchronize XeSS output failed",
			hr
		);

		return false;
	}

	impl.context11->CopyResource(
		output,
		impl.sharedOutput11.get()
	);

	impl.resetHistory =
		false;

	return true;
}

} // namespace Magpie

#else

namespace Magpie {

struct XeSSZeroMVUpscaler::Impl {};

XeSSZeroMVUpscaler::XeSSZeroMVUpscaler() = default;

XeSSZeroMVUpscaler::~XeSSZeroMVUpscaler() = default;

bool XeSSZeroMVUpscaler::Initialize(
	DeviceResources&,
	ID3D11Texture2D*,
	ID3D11Texture2D*,
	bool,
	bool
) noexcept {
	Logger::Get().Error(
		"XeSS Zero-MV support is not enabled in this build"
	);

	return false;
}

bool XeSSZeroMVUpscaler::Resize(
	DeviceResources&,
	ID3D11Texture2D*,
	ID3D11Texture2D*
) noexcept {
	return false;
}

bool XeSSZeroMVUpscaler::Draw(
	const NativeEffectDrawContext&
) noexcept {
	return false;
}

} // namespace Magpie

#endif