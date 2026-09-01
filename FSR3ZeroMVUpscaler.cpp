#include "pch.h"
#include "FSR3ZeroMVUpscaler.h"
#include "DeviceResources.h"
#include "HalfResOpticalFlow.h"
#include "Logger.h"

#ifdef MP_ENABLE_FSR3_ZEROMV
#include <d3d12.h>
#include <dxgi1_6.h>
#include <ffx_api.h>
#include <dx12/ffx_api_dx12.h>
#include <ffx_upscale.h>
#include <chrono>

namespace Magpie {

struct FSR3ZeroMVUpscaler::Impl {
    ~Impl();

    winrt::com_ptr<ID3D11Device5> device11;
    winrt::com_ptr<ID3D11DeviceContext4> context11;
    winrt::com_ptr<ID3D12Device> device12;
    winrt::com_ptr<ID3D12CommandQueue> queue12;
    winrt::com_ptr<ID3D12CommandAllocator> allocator12;
    winrt::com_ptr<ID3D12GraphicsCommandList> commandList12;
    winrt::com_ptr<ID3D11Texture2D> sharedInput11;
    winrt::com_ptr<ID3D11Texture2D> sharedOutput11;
    winrt::com_ptr<ID3D11Texture2D> sharedMotion11;
    winrt::com_ptr<ID3D11Texture2D> flatDepth11;
    winrt::com_ptr<ID3D11Texture2D> exposure11;
    winrt::com_ptr<ID3D11Texture2D> reactive11;
    winrt::com_ptr<ID3D11Texture2D> transparency11;
    winrt::com_ptr<ID3D11Texture2D> zeroMotion11;
    winrt::com_ptr<ID3D12Resource> sharedInput12;
    winrt::com_ptr<ID3D12Resource> sharedOutput12;
    winrt::com_ptr<ID3D12Resource> sharedMotion12;
    winrt::com_ptr<ID3D12Resource> zeroMotion12;
    winrt::com_ptr<ID3D12Resource> flatDepth12;
    winrt::com_ptr<ID3D12Resource> exposure12;
    winrt::com_ptr<ID3D12Resource> reactive12;
    winrt::com_ptr<ID3D12Resource> transparency12;
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
    bool enableJitter = false;
    bool useFsr4 = false;
    bool resetHistory = true;
    bool loggedFirstFrame = false;
    uint64_t frameCounter = 0;
    std::chrono::steady_clock::time_point lastFrameTime{};
    bool haveLastFrameTime = false;
    winrt::com_ptr<ID3D12QueryHeap> timestampQueryHeap;
    winrt::com_ptr<ID3D12Resource> timestampReadback;
    uint64_t timestampFrequency = 0;
    bool gpuTimingEnabled = false;
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
    winrt::com_ptr<ID3D12Resource>& texture12,
    const D3D11_SUBRESOURCE_DATA* initialData = nullptr,
    UINT bindFlagsOverride = 0
) noexcept {
    D3D11_TEXTURE2D_DESC desc = sourceDesc;
    desc.MipLevels = 1;
    desc.ArraySize = 1;
    desc.SampleDesc.Count = 1;
    desc.SampleDesc.Quality = 0;
    desc.Usage = D3D11_USAGE_DEFAULT;
    desc.CPUAccessFlags = 0;
    // NT-handle sharing requires an actual sharing mode as well.
    // Use the non-keyed shared mode because synchronization is done with the
    // shared D3D11/D3D12 fence below.
    desc.MiscFlags =
        D3D11_RESOURCE_MISC_SHARED |
        D3D11_RESOURCE_MISC_SHARED_NTHANDLE;
    desc.BindFlags = bindFlagsOverride ? bindFlagsOverride :
        D3D11_BIND_SHADER_RESOURCE;

    HRESULT hr = impl.device11->CreateTexture2D(
        &desc,
        initialData,
        texture11.put());
    if (FAILED(hr)) {
        Logger::Get().ComError(
            "Create FSR3 shared D3D11 texture failed",
            hr);
        Logger::Get().Error(fmt::format(
            "FSR3 CreateTexture2D args: {}x{} fmt={} bind=0x{:X} usage={} cpu=0x{:X} misc=0x{:X} mips={} array={} samples={} quality={}",
            desc.Width, desc.Height, (uint32_t)desc.Format, desc.BindFlags,
            (uint32_t)desc.Usage, desc.CPUAccessFlags, desc.MiscFlags,
            desc.MipLevels, desc.ArraySize, desc.SampleDesc.Count,
            desc.SampleDesc.Quality));
        return false;
    }

    winrt::com_ptr<IDXGIResource1> dxgiResource;
    hr = texture11->QueryInterface(IID_PPV_ARGS(dxgiResource.put()));
    if (FAILED(hr)) {
        Logger::Get().ComError(
            "Query shared D3D11 texture as IDXGIResource1 failed",
            hr);
        return false;
    }

    HANDLE rawHandle = nullptr;
    hr = dxgiResource->CreateSharedHandle(
        nullptr,
        GENERIC_ALL,
        nullptr,
        &rawHandle);
    if (FAILED(hr)) {
        Logger::Get().ComError(
            "Create shared D3D11 texture handle failed",
            hr);
        return false;
    }

    wil::unique_handle handle(rawHandle);

    hr = impl.device12->OpenSharedHandle(
        handle.get(),
        IID_PPV_ARGS(texture12.put()));
    if (FAILED(hr)) {
        Logger::Get().ComError(
            "Open FSR3 shared texture in D3D12 failed",
            hr);
        return false;
    }

    if (!texture12) {
        Logger::Get().Error(
            "FSR3: D3D12 shared texture returned null resource");
        return false;
    }

    return true;
}

static bool CreateSharedAuxTexture(
    FSR3ZeroMVUpscaler::Impl& impl,
    DXGI_FORMAT format,
    uint32_t width,
    uint32_t height,
    winrt::com_ptr<ID3D11Texture2D>& texture11,
    winrt::com_ptr<ID3D12Resource>& texture12,
    const float value[4]
) noexcept {
    D3D11_TEXTURE2D_DESC desc{};
    desc.Width = width;
    desc.Height = height;
    desc.MipLevels = 1;
    desc.ArraySize = 1;
    desc.Format = format;
    desc.SampleDesc.Count = 1;
    desc.SampleDesc.Quality = 0;
    desc.Usage = D3D11_USAGE_DEFAULT;
    desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
    desc.CPUAccessFlags = 0;
    desc.MiscFlags =
        D3D11_RESOURCE_MISC_SHARED |
        D3D11_RESOURCE_MISC_SHARED_NTHANDLE;

    const UINT bytesPerPixel =
        format == DXGI_FORMAT_R32_FLOAT ? 4u :
        format == DXGI_FORMAT_R8_UNORM ? 1u :
        format == DXGI_FORMAT_R16G16_FLOAT ? 4u : 0u;

    if (!bytesPerPixel) {
        Logger::Get().Error(
            "FSR3: unsupported auxiliary texture format");
        return false;
    }

    const UINT rowPitch = width * bytesPerPixel;
    std::vector<uint8_t> data(
        static_cast<size_t>(rowPitch) * height,
        0);

    if (format == DXGI_FORMAT_R32_FLOAT) {
        const float v = value[0];
        for (uint32_t y = 0; y < height; ++y) {
            auto* row = reinterpret_cast<float*>(
                data.data() + static_cast<size_t>(y) * rowPitch);
            for (uint32_t x = 0; x < width; ++x) {
                row[x] = v;
            }
        }
    } else if (format == DXGI_FORMAT_R8_UNORM) {
        const uint8_t v = static_cast<uint8_t>(
            std::clamp(value[0], 0.0f, 1.0f) * 255.0f + 0.5f);
        for (uint32_t y = 0; y < height; ++y) {
            std::memset(
                data.data() + static_cast<size_t>(y) * rowPitch,
                v,
                rowPitch);
        }
    } else {
        // R16G16_FLOAT zero initialization requires no conversion.
        if (value[0] != 0.0f || value[1] != 0.0f ||
            value[2] != 0.0f || value[3] != 0.0f) {
            Logger::Get().Error(
                "FSR3: non-zero R16G16_FLOAT auxiliary value unsupported");
            return false;
        }
    }

    D3D11_SUBRESOURCE_DATA initialData{};
    initialData.pSysMem = data.data();
    initialData.SysMemPitch = rowPitch;

    // Aux inputs are created by D3D11 and shared into D3D12.
    // Avoid D3D12 resource creation/UAV-clear path that crashes NVIDIA driver here.
    return CreateSharedTexture(
        impl,
        desc,
        texture11,
        texture12,
        &initialData);
}

FSR3ZeroMVUpscaler::FSR3ZeroMVUpscaler() = default;
FSR3ZeroMVUpscaler::~FSR3ZeroMVUpscaler() = default;

bool FSR3ZeroMVUpscaler::Initialize(
    DeviceResources& resources,
    ID3D11Texture2D* input,
    ID3D11Texture2D* output,
    bool enableOpticalFlow,
    bool enableJitter,
    bool useFsr4
) noexcept {
    // GTX 1050 Ti path: FSR 3.1.5 only.
    Logger::Get().Info("FSR3ZeroMV BUILD 20260901-K GTX1050TI FSR3.1.5 VARIANT-SELECTABLE CLEAN-LOG");
    // GTX 1050 Ti FSR3 path: always resolves to FSR 3.1.5.
    // FSR4-specific behavior should eventually live in FSR4ZeroMVUpscaler.cpp;
    // keeping useFsr4 here is only for the existing FSR4->FSR3 fallback path.
    useFsr4 = false;

    _enableOpticalFlow = enableOpticalFlow;
    _enableJitter = enableJitter;
    _useFsr4 = false;
    _impl.reset();

    if (!input || !output) {
        Logger::Get().Error("FSR3: null input/output texture");
        return false;
    }

    auto impl = std::make_unique<Impl>();
    // Keep our own COM references. DeviceResources may recreate or release
    // its interfaces after backend initialization; raw borrowed pointers are
    // unsafe across the renderer thread lifetime.
    impl->device11.copy_from(resources.GetD3DDevice());
    impl->context11.copy_from(resources.GetD3DDC());
    impl->enableOpticalFlow = enableOpticalFlow;
    impl->enableJitter = enableJitter;
    impl->useFsr4 = false;

    if (!impl->device11 || !impl->context11) {
        Logger::Get().Error("FSR3: invalid D3D11 device/context");
        return false;
    }

    D3D11_TEXTURE2D_DESC inputDesc{};
    D3D11_TEXTURE2D_DESC outputDesc{};
    input->GetDesc(&inputDesc);
    output->GetDesc(&outputDesc);

    if (inputDesc.Format != outputDesc.Format) {
        Logger::Get().Error(fmt::format(
            "FSR3: input/output format mismatch ({} vs {})",
            (uint32_t)inputDesc.Format, (uint32_t)outputDesc.Format));
        return false;
    }

    if (inputDesc.SampleDesc.Count != 1 || outputDesc.SampleDesc.Count != 1) {
        Logger::Get().Error("FSR3: multisampled input/output is unsupported");
        return false;
    }

    if (!inputDesc.Width || !inputDesc.Height || !outputDesc.Width || !outputDesc.Height) {
        Logger::Get().Error("FSR3: invalid input/output dimensions");
        return false;
    }

    if (inputDesc.Width > outputDesc.Width || inputDesc.Height > outputDesc.Height) {
        Logger::Get().Error("FSR3 backend only supports upscaling");
        return false;
    }
    if ((float)outputDesc.Width / inputDesc.Width > 3.0f ||
        (float)outputDesc.Height / inputDesc.Height > 3.0f) {
        Logger::Get().Error("FSR3 backend supports up to a 3x scale");
        return false;
    }

    impl->inputWidth = inputDesc.Width;
    impl->inputHeight = inputDesc.Height;
    impl->outputWidth = outputDesc.Width;
    impl->outputHeight = outputDesc.Height;

    // Critical on hybrid laptops: use exact adapter behind existing D3D11 device.
    winrt::com_ptr<IDXGIDevice> dxgiDevice;
    HRESULT hr = impl->device11->QueryInterface(IID_PPV_ARGS(dxgiDevice.put()));
    if (FAILED(hr)) {
        Logger::Get().ComError("FSR3: QueryInterface IDXGIDevice failed", hr);
        return false;
    }

    winrt::com_ptr<IDXGIAdapter> dxgiAdapter;
    hr = dxgiDevice->GetAdapter(dxgiAdapter.put());
    if (FAILED(hr) || !dxgiAdapter) {
        Logger::Get().ComError("FSR3: GetAdapter from D3D11 device failed", hr);
        return false;
    }

    DXGI_ADAPTER_DESC adapterDesc{};
    hr = dxgiAdapter->GetDesc(&adapterDesc);
    if (FAILED(hr)) {
        Logger::Get().ComError("FSR3: GetDesc adapter failed", hr);
        return false;
    }

    Logger::Get().Info(fmt::format(
        "FSR3 adapter: {} | {:04X}:{:04X}",
        winrt::to_string(adapterDesc.Description),
        adapterDesc.VendorId, adapterDesc.DeviceId));

    // This backend is being targeted at NVIDIA Pascal.
    if (adapterDesc.VendorId != 0x10DE) {
        Logger::Get().Error("FSR3: current D3D11 adapter is not NVIDIA");
        return false;
    }

    DXGI_ADAPTER_DESC1 adapterDesc1{};

    hr = D3D12CreateDevice(
        dxgiAdapter.get(),
        D3D_FEATURE_LEVEL_11_0,
        IID_PPV_ARGS(impl->device12.put()));
    if (FAILED(hr)) {
        Logger::Get().ComError(
            "FSR3: D3D12CreateDevice on D3D11 adapter failed", hr);
        return false;
    }

    // FSR 3.1.5 requires Shader Model 6.2+.
    D3D12_FEATURE_DATA_SHADER_MODEL shaderModel{};
    shaderModel.HighestShaderModel = D3D_SHADER_MODEL_6_6;
    hr = impl->device12->CheckFeatureSupport(
        D3D12_FEATURE_SHADER_MODEL, &shaderModel, sizeof(shaderModel));
    if (FAILED(hr)) {
        shaderModel.HighestShaderModel = D3D_SHADER_MODEL_6_2;
        hr = impl->device12->CheckFeatureSupport(
            D3D12_FEATURE_SHADER_MODEL, &shaderModel, sizeof(shaderModel));
    }
    if (FAILED(hr) || shaderModel.HighestShaderModel < D3D_SHADER_MODEL_6_2) {
        Logger::Get().Error("FSR3.1.5 requires Shader Model 6.2");
        return false;
    }

    D3D12_COMMAND_QUEUE_DESC queueDesc{};
    queueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
    hr = impl->device12->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(impl->queue12.put()));
    if (SUCCEEDED(hr)) hr = impl->device12->CreateCommandAllocator(
        D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(impl->allocator12.put()));
    if (SUCCEEDED(hr)) hr = impl->device12->CreateCommandList(
        0, D3D12_COMMAND_LIST_TYPE_DIRECT, impl->allocator12.get(), nullptr,
        IID_PPV_ARGS(impl->commandList12.put()));
    if (FAILED(hr)) {
        Logger::Get().ComError("Create FSR3 D3D12 command objects failed", hr);
        return false;
    }

    // Optional GPU timing. Timestamp query is low overhead and lets us verify
    // actual GPU execution instead of confusing CPU dispatch/queue waits with
    // FSR execution time.
    D3D12_QUERY_HEAP_DESC queryDesc{};
    queryDesc.Type = D3D12_QUERY_HEAP_TYPE_TIMESTAMP;
    queryDesc.Count = 2;
    hr = impl->device12->CreateQueryHeap(
        &queryDesc, IID_PPV_ARGS(impl->timestampQueryHeap.put()));
    if (SUCCEEDED(hr)) {
        hr = impl->queue12->GetTimestampFrequency(&impl->timestampFrequency);
    }
    if (SUCCEEDED(hr) && impl->timestampFrequency) {
        D3D12_RESOURCE_DESC readbackDesc{};
        readbackDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
        readbackDesc.Width = sizeof(uint64_t) * 2;
        readbackDesc.Height = 1;
        readbackDesc.DepthOrArraySize = 1;
        readbackDesc.MipLevels = 1;
        readbackDesc.Format = DXGI_FORMAT_UNKNOWN;
        readbackDesc.SampleDesc.Count = 1;
        readbackDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
        D3D12_HEAP_PROPERTIES readbackHeap{};
        readbackHeap.Type = D3D12_HEAP_TYPE_READBACK;
        hr = impl->device12->CreateCommittedResource(
            &readbackHeap, D3D12_HEAP_FLAG_NONE, &readbackDesc,
            D3D12_RESOURCE_STATE_COPY_DEST, nullptr,
            IID_PPV_ARGS(impl->timestampReadback.put()));
    }
    impl->gpuTimingEnabled = SUCCEEDED(hr) && impl->timestampQueryHeap &&
        impl->timestampReadback && impl->timestampFrequency;
    if (impl->gpuTimingEnabled) {
    } else {
    }

    // Shared input must be SRV-readable by FSR.
    if (!CreateSharedTexture(
            *impl,
            inputDesc,
            impl->sharedInput11,
            impl->sharedInput12,
            nullptr,
            D3D11_BIND_SHADER_RESOURCE)) {
        return false;
    }

    // Shared output must be UAV-capable for FSR's compute output.
    if (!CreateSharedTexture(
            *impl,
            outputDesc,
            impl->sharedOutput11,
            impl->sharedOutput12,
            nullptr,
            D3D11_BIND_UNORDERED_ACCESS)) {
        return false;
    }

    if (enableOpticalFlow) {
        impl->opticalFlow = std::make_unique<HalfResOpticalFlow>();
        if (!impl->opticalFlow->Initialize(impl->device11.get(), impl->context11.get(), input)) return false;

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

    const float zero[4]{};
    const float one[4]{ 1, 1, 1, 1 };
    const float reactiveOpticalFlow[4]{};
    const float reactiveZeroMotion[4]{ 0.8f, 0.8f, 0.8f, 0.8f };

    // Aux inputs live in D3D11 NT-handle shared textures too.
    // No D3D12 auxiliary resource creation or UAV clear is used.

    if (!CreateSharedAuxTexture(
            *impl,
            DXGI_FORMAT_R32_FLOAT,
            inputDesc.Width,
            inputDesc.Height,
            impl->flatDepth11,
            impl->flatDepth12,
            zero)) {
        return false;
    }

    if (!CreateSharedAuxTexture(
            *impl,
            DXGI_FORMAT_R32_FLOAT,
            1,
            1,
            impl->exposure11,
            impl->exposure12,
            one)) {
        return false;
    }

    if (!CreateSharedAuxTexture(
            *impl,
            DXGI_FORMAT_R8_UNORM,
            inputDesc.Width,
            inputDesc.Height,
            impl->reactive11,
            impl->reactive12,
            enableOpticalFlow ? reactiveOpticalFlow : reactiveZeroMotion)) {
        return false;
    }

    if (!CreateSharedAuxTexture(
            *impl,
            DXGI_FORMAT_R8_UNORM,
            inputDesc.Width,
            inputDesc.Height,
            impl->transparency11,
            impl->transparency12,
            zero)) {
        return false;
    }

    if (!enableOpticalFlow) {
        if (!CreateSharedAuxTexture(
                *impl,
                DXGI_FORMAT_R16G16_FLOAT,
                inputDesc.Width,
                inputDesc.Height,
                impl->zeroMotion11,
                impl->zeroMotion12,
                zero)) {
            return false;
        }
    }

    // Create a fence shared between the existing D3D11 device/context and D3D12 queue.
    // Draw uses it in both directions: D3D11 -> D3D12 for the input, then
    // D3D12 -> D3D11 for the output.
    hr = impl->device11->CreateFence(
        0,
        D3D11_FENCE_FLAG_SHARED,
        IID_PPV_ARGS(impl->fence11.put()));
    if (FAILED(hr)) {
        Logger::Get().ComError(
            "Create FSR3 shared D3D11 fence failed",
            hr);
        return false;
    }

    HANDLE rawFence = nullptr;
    hr = impl->fence11->CreateSharedHandle(
        nullptr,
        GENERIC_ALL,
        nullptr,
        &rawFence);
    if (FAILED(hr)) {
        Logger::Get().ComError(
            "Create FSR3 shared fence handle failed",
            hr);
        return false;
    }

    wil::unique_handle fenceHandle(rawFence);
    hr = impl->device12->OpenSharedHandle(
        fenceHandle.get(),
        IID_PPV_ARGS(impl->fence12.put()));
    if (FAILED(hr) || !impl->fence12) {
        Logger::Get().ComError(
            "Open FSR3 shared fence in D3D12 failed",
            hr);
        return false;
    }


    // Command list starts open after CreateCommandList. No init GPU work needed now.
    // Close it so first Draw can Reset it.
    hr = impl->commandList12->Close();
    if (FAILED(hr)) {
        Logger::Get().ComError(
            "Close FSR3 command list after initialization failed",
            hr);
        return false;
    }

    // Only FSR 3.1.5 provider is requested.
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
        Logger::Get().Error(fmt::format(
            "Query FSR upscaler versions failed ({})", (uint32_t)rc));
        return false;
    }

    std::vector<uint64_t> versionIds(versionCount);
    std::vector<const char*> versionNames(versionCount);
    versionQuery.versionIds = versionIds.data();
    versionQuery.versionNames = versionNames.data();
    rc = impl->query(nullptr, &versionQuery.header);
    if (rc != FFX_API_RETURN_OK) return false;

    uint64_t selectedVersionId = 0;
    const char* requestedVersion = "3.1.5";
    std::string availableVersions;
    for (uint64_t i = 0; i < versionCount; ++i) {
        const char* name = versionNames[i] ? versionNames[i] : "unknown";
        if (!availableVersions.empty()) availableVersions += ", ";
        availableVersions += name;
        if (strstr(name, requestedVersion)) selectedVersionId = versionIds[i];
    }

    if (!selectedVersionId) {
        Logger::Get().Error(fmt::format(
            "FSR 3.1.5 provider not found; available: {}", availableVersions));
        return false;
    }

    impl->createDesc.header.type = FFX_API_CREATE_CONTEXT_DESC_TYPE_UPSCALE;
    impl->createDesc.flags =
        FFX_UPSCALE_ENABLE_DEPTH_INVERTED |
        FFX_UPSCALE_ENABLE_DEPTH_INFINITE |
        FFX_UPSCALE_ENABLE_NON_LINEAR_COLORSPACE;
    impl->createDesc.maxRenderSize = { inputDesc.Width, inputDesc.Height };
    impl->createDesc.maxUpscaleSize = { outputDesc.Width, outputDesc.Height };

    impl->backendDesc.header.type = FFX_API_CREATE_CONTEXT_DESC_TYPE_BACKEND_DX12;
    impl->backendDesc.device = impl->device12.get();
    impl->apiVersion.header.type = FFX_API_CREATE_CONTEXT_DESC_TYPE_UPSCALE_VERSION;
    impl->apiVersion.version = FFX_UPSCALER_VERSION;
    impl->overrideVersion.header.type = FFX_API_DESC_TYPE_OVERRIDE_VERSION;
    impl->overrideVersion.versionId = selectedVersionId;

    impl->createDesc.header.pNext = &impl->backendDesc.header;
    impl->backendDesc.header.pNext = &impl->apiVersion.header;
    impl->apiVersion.header.pNext = &impl->overrideVersion.header;

    rc = impl->createContext(&impl->context, &impl->createDesc.header, nullptr);
    if (rc != FFX_API_RETURN_OK || !impl->context) {
        Logger::Get().Error(fmt::format(
            "Create FSR 3.1.5 context failed ({})", (uint32_t)rc));
        return false;
    }

    Logger::Get().Info(fmt::format(
        "FSR 3.1.5 initialized (opticalFlow={}, jitter={}): {}x{} -> {}x{}",
        enableOpticalFlow, enableJitter, inputDesc.Width, inputDesc.Height,
        outputDesc.Width, outputDesc.Height));

    _impl = std::move(impl);
    return true;
}

bool FSR3ZeroMVUpscaler::Resize(
    DeviceResources& resources,
    ID3D11Texture2D* input,
    ID3D11Texture2D* output
) noexcept {
    return Initialize(resources, input, output, _enableOpticalFlow, _enableJitter, false);
}

static float Halton(uint32_t index, uint32_t base) noexcept {
    float result = 0.0f;
    float fraction = 1.0f;
    while (index) {
        fraction /= (float)base;
        result += fraction * (float)(index % base);
        index /= base;
    }
    return result;
}

bool FSR3ZeroMVUpscaler::Draw(const NativeEffectDrawContext& drawContext) noexcept {
    ID3D11Texture2D* input = drawContext.input;
    ID3D11Texture2D* output = drawContext.output;
    if (!_impl || !_impl->context || !input || !output) return false;

    Impl& impl = *_impl;
    const auto drawStart = std::chrono::steady_clock::now();
    if (!impl.device11 || !impl.context11 || !impl.fence11 ||
        !impl.device12 || !impl.queue12 || !impl.allocator12 ||
        !impl.commandList12 || !impl.fence12 ||
        !impl.sharedInput11 || !impl.sharedOutput11 ||
        !impl.sharedInput12 || !impl.sharedOutput12) {
        Logger::Get().Error("FSR3: Draw resources are not initialized");
        return false;
    }

    D3D11_TEXTURE2D_DESC runtimeInputDesc{};
    D3D11_TEXTURE2D_DESC runtimeOutputDesc{};
    input->GetDesc(&runtimeInputDesc);
    output->GetDesc(&runtimeOutputDesc);
    if (runtimeInputDesc.Width != impl.inputWidth || runtimeInputDesc.Height != impl.inputHeight ||
        runtimeOutputDesc.Width != impl.outputWidth || runtimeOutputDesc.Height != impl.outputHeight ||
        runtimeInputDesc.Format != runtimeOutputDesc.Format) {
        Logger::Get().Error("FSR3: Draw texture dimensions/format changed without Resize");
        return false;
    }

    if (!WaitForFence(impl, impl.lastSubmittedValue)) return false;

    impl.context11->CopyResource(impl.sharedInput11.get(), input);

    if (impl.enableOpticalFlow) {
        if (!impl.opticalFlow || !impl.opticalFlow->Estimate(input)) return false;
        impl.context11->CopyResource(
            impl.sharedMotion11.get(), impl.opticalFlow->GetMotionTexture());
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

    D3D12_RESOURCE_BARRIER barriers[8]{};

    auto makeReadBarrier = [](D3D12_RESOURCE_BARRIER& barrier, ID3D12Resource* resource) {
        barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
        barrier.Transition = {
            resource,
            D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES,
            D3D12_RESOURCE_STATE_COMMON,
            D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE
        };
    };

    auto makeWriteBarrier = [](D3D12_RESOURCE_BARRIER& barrier, ID3D12Resource* resource) {
        barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
        barrier.Transition = {
            resource,
            D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES,
            D3D12_RESOURCE_STATE_COMMON,
            D3D12_RESOURCE_STATE_UNORDERED_ACCESS
        };
    };

    UINT barrierCount = 0;
    makeReadBarrier(barriers[barrierCount++], impl.sharedInput12.get());
    makeWriteBarrier(barriers[barrierCount++], impl.sharedOutput12.get());

    if (impl.enableOpticalFlow) {
        makeReadBarrier(barriers[barrierCount++], impl.sharedMotion12.get());
    } else {
        makeReadBarrier(barriers[barrierCount++], impl.zeroMotion12.get());
    }

    makeReadBarrier(barriers[barrierCount++], impl.flatDepth12.get());
    makeReadBarrier(barriers[barrierCount++], impl.exposure12.get());
    makeReadBarrier(barriers[barrierCount++], impl.reactive12.get());
    makeReadBarrier(barriers[barrierCount++], impl.transparency12.get());

    impl.commandList12->ResourceBarrier(barrierCount, barriers);

    ffxDispatchDescUpscale desc{};
    if (impl.gpuTimingEnabled) {
        impl.commandList12->EndQuery(
            impl.timestampQueryHeap.get(), D3D12_QUERY_TYPE_TIMESTAMP, 0);
    }
    desc.header.type = FFX_API_DISPATCH_DESC_TYPE_UPSCALE;
    desc.commandList = impl.commandList12.get();
    desc.color = ffxApiGetResourceDX12(
        impl.sharedInput12.get(), FFX_API_RESOURCE_STATE_COMPUTE_READ);
    desc.depth = ffxApiGetResourceDX12(
        impl.flatDepth12.get(), FFX_API_RESOURCE_STATE_COMPUTE_READ);
    desc.motionVectors = ffxApiGetResourceDX12(
        impl.enableOpticalFlow ? impl.sharedMotion12.get() : impl.zeroMotion12.get(),
        FFX_API_RESOURCE_STATE_COMPUTE_READ);
    desc.exposure = ffxApiGetResourceDX12(
        impl.exposure12.get(), FFX_API_RESOURCE_STATE_COMPUTE_READ);
    desc.reactive = ffxApiGetResourceDX12(
        impl.reactive12.get(), FFX_API_RESOURCE_STATE_COMPUTE_READ);
    desc.transparencyAndComposition = ffxApiGetResourceDX12(
        impl.transparency12.get(), FFX_API_RESOURCE_STATE_COMPUTE_READ);
    desc.output = ffxApiGetResourceDX12(
        impl.sharedOutput12.get(), FFX_API_RESOURCE_STATE_UNORDERED_ACCESS);

    if (impl.enableJitter) {
        // This is the dedicated ZeroMV+Jitter variant. Keep the same 8-sample
        // Halton sequence used by the original experimental implementation.
        // ZeroMV itself keeps jitter disabled.
        const uint32_t sample = (uint32_t)((impl.frameCounter & 7u) + 1u);
        desc.jitterOffset = {
            Halton(sample, 2) - 0.5f,
            Halton(sample, 3) - 0.5f
        };
    } else {
        desc.jitterOffset = { 0.0f, 0.0f };
    }
    desc.motionVectorScale = { 1.0f, 1.0f };
    desc.renderSize = { impl.inputWidth, impl.inputHeight };
    desc.upscaleSize = { impl.outputWidth, impl.outputHeight };
    desc.enableSharpening = true;
    desc.sharpness = 0.2f;

    const float frameTimeDeltaMs = 16.6667f;

    desc.preExposure = 1.0f;
    desc.reset = impl.resetHistory;
    desc.cameraNear = 1.0f;
    desc.cameraFar = FLT_MAX;
    desc.cameraFovAngleVertical = 1.04719755f;
    desc.viewSpaceToMetersFactor = 1.0f;
    desc.flags = FFX_UPSCALE_FLAG_NON_LINEAR_COLOR_SRGB;

    const auto dispatchStart = std::chrono::steady_clock::now();
    const ffxReturnCode_t rc = impl.dispatch(&impl.context, &desc.header);
    const double dispatchCpuMs = std::chrono::duration<double, std::milli>(
        std::chrono::steady_clock::now() - dispatchStart).count();
    if (rc != FFX_API_RETURN_OK) {
        Logger::Get().Error(fmt::format(
            "Dispatch FSR 3.1.5 failed ({})", (uint32_t)rc));
        return false;
    }

    ++impl.frameCounter;

    if (impl.gpuTimingEnabled) {
        impl.commandList12->EndQuery(
            impl.timestampQueryHeap.get(), D3D12_QUERY_TYPE_TIMESTAMP, 1);
        impl.commandList12->ResolveQueryData(
            impl.timestampQueryHeap.get(),
            D3D12_QUERY_TYPE_TIMESTAMP,
            0, 2,
            impl.timestampReadback.get(),
            0);
    }

    for (UINT i = 0; i < barrierCount; ++i) {
        std::swap(barriers[i].Transition.StateBefore,
                  barriers[i].Transition.StateAfter);
    }
    impl.commandList12->ResourceBarrier(barrierCount, barriers);

    hr = impl.commandList12->Close();
    if (FAILED(hr)) return false;

    ID3D12CommandList* lists[]{ impl.commandList12.get() };
    impl.queue12->ExecuteCommandLists(1, lists);

    const uint64_t outputReady = ++impl.fenceValue;
    hr = impl.queue12->Signal(impl.fence12.get(), outputReady);
    if (FAILED(hr)) return false;

    impl.lastSubmittedValue = outputReady;

    hr = impl.context11->Wait(impl.fence11.get(), outputReady);
    if (FAILED(hr)) {
        Logger::Get().ComError("FSR3: D3D11 wait for output fence failed", hr);
        return false;
    }

    if (impl.gpuTimingEnabled) {
        uint64_t* timestamps = nullptr;
        D3D12_RANGE readRange{0, sizeof(uint64_t) * 2};
        hr = impl.timestampReadback->Map(0, &readRange, reinterpret_cast<void**>(&timestamps));
        if (SUCCEEDED(hr) && timestamps) {
            const uint64_t begin = timestamps[0];
            const uint64_t end = timestamps[1];
            if (end >= begin && impl.timestampFrequency) {
                const double gpuMs = (double)(end - begin) * 1000.0 /
                    (double)impl.timestampFrequency;
                if (!impl.loggedFirstFrame || (impl.frameCounter % 120u) == 0u) {
                    Logger::Get().Info(fmt::format(
                        "FSR3: frame={} dt={:.3f} ms dispatchCPU={:.3f} ms gpu={:.3f} ms",
                        impl.frameCounter,
                        frameTimeDeltaMs,
                        dispatchCpuMs,
                        gpuMs));
                    impl.loggedFirstFrame = true;
                }
            }
            D3D12_RANGE writtenRange{0, 0};
            impl.timestampReadback->Unmap(0, &writtenRange);
        }
    } else if (!impl.loggedFirstFrame) {
        Logger::Get().Info(fmt::format(
            "FSR3: first frame submitted dt={:.3f} ms dispatchCPU={:.3f} ms",
            frameTimeDeltaMs, dispatchCpuMs));
        impl.loggedFirstFrame = true;
    }

    impl.context11->CopyResource(output, impl.sharedOutput11.get());
    impl.resetHistory = false;
    return true;
}

} // namespace Magpie

#else

namespace Magpie {

struct FSR3ZeroMVUpscaler::Impl {};
FSR3ZeroMVUpscaler::FSR3ZeroMVUpscaler() = default;
FSR3ZeroMVUpscaler::~FSR3ZeroMVUpscaler() = default;

bool FSR3ZeroMVUpscaler::Initialize(
    DeviceResources&, ID3D11Texture2D*, ID3D11Texture2D*,
    bool, bool, bool) noexcept {
    Logger::Get().Error("FSR3 support is not enabled in this build");
    return false;
}

bool FSR3ZeroMVUpscaler::Resize(
    DeviceResources&, ID3D11Texture2D*, ID3D11Texture2D*) noexcept {
    return false;
}

bool FSR3ZeroMVUpscaler::Draw(
    const NativeEffectDrawContext&) noexcept {
    return false;
}

} // namespace Magpie

#endif
