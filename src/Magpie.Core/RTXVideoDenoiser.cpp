#include "pch.h"
#include "RTXVideoDenoiser.h"
#include "DeviceResources.h"
#include "Logger.h"

#ifdef MP_ENABLE_RTX_VIDEO_DENOISE

#include <nvCVImage.h>
#include <nvTransferD3D11.h>
#include <nvVideoEffects.h>

// Required by NVIDIA's MIT-licensed proxy loader. USE_APP_PATH below makes it
// load the runtime copied beside Magpie.exe rather than an obsolete system SDK.
char* g_nvVFXSDKPath = nullptr;

namespace Magpie {

using NvVFXCudaStreamSynchronizeFn = NvCV_Status(NvVFX_API*)(CUstream);

static bool VFXSucceeded(NvCV_Status status, const char* operation) noexcept {
	if (status == NVCV_SUCCESS) {
		return true;
	}

	const char* message = NvCV_GetErrorStringFromCode(status);
	Logger::Get().Error(fmt::format(
		"{} failed: {} ({})", operation, message ? message : "unknown VFX error", (int)status));
	return false;
}

struct RTXVideoDenoiser::Impl {
	NvVFX_Handle effect = nullptr;
	CUstream stream = nullptr;
	NvCVImage inputD3D;
	NvCVImage outputD3D;
	NvCVImage inputGPU;
	NvCVImage outputGPU;
	NvCVImage temporaryGPU;
	NvVFXCudaStreamSynchronizeFn synchronize = nullptr;
	float inputScale = 1.0f;
	float outputScale = 1.0f;

	~Impl() {
		if (effect) {
			NvVFX_DestroyEffect(effect);
			effect = nullptr;
		}

		// Dealloc also unregisters D3D resources initialized through
		// NvCVImage_InitFromD3D11Texture. It clears the descriptors, making
		// their subsequent C++ destructors no-ops.
		NvCVImage_Dealloc(&temporaryGPU);
		NvCVImage_Dealloc(&outputGPU);
		NvCVImage_Dealloc(&inputGPU);
		NvCVImage_Dealloc(&outputD3D);
		NvCVImage_Dealloc(&inputD3D);

		if (stream) {
			NvVFX_CudaStreamDestroy(stream);
			stream = nullptr;
		}
	}
};

RTXVideoDenoiser::RTXVideoDenoiser() = default;
RTXVideoDenoiser::~RTXVideoDenoiser() = default;

bool RTXVideoDenoiser::Initialize(
	DeviceResources&,
	ID3D11Texture2D* input,
	ID3D11Texture2D* output,
	uint32_t qualityLevel
) noexcept {
	_impl.reset();
	_qualityLevel = qualityLevel;
	_initializationError = ScalingError::NoError;

	const bool isUpscaleQuality = qualityLevel >= 1 && qualityLevel <= 4;
	const bool isDenoiseQuality = qualityLevel >= 8 && qualityLevel <= 11;
	const bool isHighBitrateQuality = qualityLevel >= 16 && qualityLevel <= 19;
	if (!isUpscaleQuality && !isDenoiseQuality && !isHighBitrateQuality) {
		Logger::Get().Error(fmt::format("Invalid RTX Video denoise quality level: {}", qualityLevel));
		return false;
	}

	D3D11_TEXTURE2D_DESC inputDesc{};
	D3D11_TEXTURE2D_DESC outputDesc{};
	input->GetDesc(&inputDesc);
	output->GetDesc(&outputDesc);
	if ((isDenoiseQuality &&
		(inputDesc.Width != outputDesc.Width || inputDesc.Height != outputDesc.Height)) ||
		inputDesc.SampleDesc.Count != 1 || outputDesc.SampleDesc.Count != 1) {
		Logger::Get().Error("RTX Video received invalid output dimensions or an MSAA texture");
		return false;
	}

	// NVIDIA's proxy loader checks this before its first LoadLibrary call.
	SetEnvironmentVariableW(L"NV_VIDEO_EFFECTS_PATH", L"USE_APP_PATH");

	auto impl = std::make_unique<Impl>();
	auto isFloatFormat = [](DXGI_FORMAT format) noexcept {
		return format == DXGI_FORMAT_R16G16B16A16_FLOAT ||
			format == DXGI_FORMAT_R32G32B32A32_FLOAT ||
			format == DXGI_FORMAT_R11G11B10_FLOAT;
	};
	// VideoSuperRes consumes integer RGBA in the 0..255 range. Magpie normally
	// renders effects into FP16 textures whose color range is 0..1.
	impl->inputScale = isFloatFormat(inputDesc.Format) ? 255.0f : 1.0f;
	impl->outputScale = isFloatFormat(outputDesc.Format) ? (1.0f / 255.0f) : 1.0f;
	if (!VFXSucceeded(NvVFX_CudaStreamCreate(&impl->stream), "NvVFX_CudaStreamCreate") ||
		!VFXSucceeded(NvCVImage_InitFromD3D11Texture(&impl->inputD3D, input),
			"NvCVImage_InitFromD3D11Texture(input)") ||
		!VFXSucceeded(NvCVImage_InitFromD3D11Texture(&impl->outputD3D, output),
			"NvCVImage_InitFromD3D11Texture(output)") ||
		!VFXSucceeded(NvCVImage_Alloc(&impl->inputGPU, inputDesc.Width, inputDesc.Height,
			NVCV_RGBA, NVCV_U8, NVCV_INTERLEAVED, NVCV_GPU, 32), "NvCVImage_Alloc(input)") ||
		!VFXSucceeded(NvCVImage_Alloc(&impl->outputGPU, outputDesc.Width, outputDesc.Height,
			NVCV_RGBA, NVCV_U8, NVCV_INTERLEAVED, NVCV_GPU, 32), "NvCVImage_Alloc(output)")) {
		return false;
	}

	const NvCV_Status createEffectStatus =
		NvVFX_CreateEffect("VideoSuperRes", &impl->effect);
	if (!VFXSucceeded(createEffectStatus, "NvVFX_CreateEffect(VideoSuperRes)")) {
		if (createEffectStatus == NVCV_ERR_UNIMPLEMENTED) {
			_initializationError = ScalingError::NvidiaVsrPathUnsupported;
		}
		return false;
	}

	if (!VFXSucceeded(NvVFX_SetImage(impl->effect, NVVFX_INPUT_IMAGE, &impl->inputGPU),
			"NvVFX_SetImage(input)") ||
		!VFXSucceeded(NvVFX_SetImage(impl->effect, NVVFX_OUTPUT_IMAGE, &impl->outputGPU),
			"NvVFX_SetImage(output)") ||
		!VFXSucceeded(NvVFX_SetCudaStream(impl->effect, NVVFX_CUDA_STREAM, impl->stream),
			"NvVFX_SetCudaStream") ||
		!VFXSucceeded(NvVFX_SetU32(impl->effect, "QualityLevel", qualityLevel),
			"NvVFX_SetU32(QualityLevel)") ||
		!VFXSucceeded(NvVFX_Load(impl->effect), "NvVFX_Load(VideoSuperRes)")) {
		return false;
	}

	HMODULE module = GetModuleHandleW(L"NVVideoEffects.dll");
	if (module) {
		impl->synchronize = reinterpret_cast<NvVFXCudaStreamSynchronizeFn>(
			GetProcAddress(module, "NvVFX_CudaStreamSynchronize"));
	}
	if (!impl->synchronize) {
		Logger::Get().Error("NvVFX_CudaStreamSynchronize is unavailable");
		return false;
	}

	Logger::Get().Info(fmt::format(
		"RTX Video denoise initialized: quality={}, {}x{}, inputFormat={}, outputFormat={}, scales={}/{}",
		qualityLevel, inputDesc.Width, inputDesc.Height,
		(int)inputDesc.Format, (int)outputDesc.Format, impl->inputScale, impl->outputScale));
	_impl = std::move(impl);
	return true;
}

bool RTXVideoDenoiser::Resize(
	DeviceResources& deviceResources,
	ID3D11Texture2D* input,
	ID3D11Texture2D* output
) noexcept {
	return Initialize(deviceResources, input, output, _qualityLevel);
}

bool RTXVideoDenoiser::Draw(const NativeEffectDrawContext& drawContext) noexcept {
	ID3D11Texture2D* input = drawContext.input;
	ID3D11Texture2D* output = drawContext.output;
	if (!_impl) {
		return false;
	}

	// Resize should have recreated the wrappers whenever these resources change.
	(void)input;
	(void)output;

	bool inputMapped = false;
	bool outputMapped = false;
	auto unmapResources = [&]() noexcept {
		if (outputMapped) {
			NvCVImage_UnmapResource(&_impl->outputD3D, _impl->stream);
			outputMapped = false;
		}
		if (inputMapped) {
			NvCVImage_UnmapResource(&_impl->inputD3D, _impl->stream);
			inputMapped = false;
		}
	};

	if (!VFXSucceeded(NvCVImage_MapResource(&_impl->inputD3D, _impl->stream),
		"NvCVImage_MapResource(input)")) {
		return false;
	}
	inputMapped = true;
	if (!VFXSucceeded(NvCVImage_Transfer(&_impl->inputD3D, &_impl->inputGPU,
		_impl->inputScale, _impl->stream, &_impl->temporaryGPU), "NvCVImage_Transfer(input)")) {
		unmapResources();
		return false;
	}
	if (!VFXSucceeded(NvCVImage_UnmapResource(&_impl->inputD3D, _impl->stream),
		"NvCVImage_UnmapResource(input)")) {
		inputMapped = false;
		return false;
	}
	inputMapped = false;

	if (!VFXSucceeded(NvVFX_Run(_impl->effect, 0), "NvVFX_Run(VideoSuperRes)")) {
		return false;
	}

	if (!VFXSucceeded(NvCVImage_MapResource(&_impl->outputD3D, _impl->stream),
		"NvCVImage_MapResource(output)")) {
		return false;
	}
	outputMapped = true;
	if (!VFXSucceeded(NvCVImage_Transfer(&_impl->outputGPU, &_impl->outputD3D,
		_impl->outputScale, _impl->stream, &_impl->temporaryGPU), "NvCVImage_Transfer(output)")) {
		unmapResources();
		return false;
	}
	if (!VFXSucceeded(NvCVImage_UnmapResource(&_impl->outputD3D, _impl->stream),
		"NvCVImage_UnmapResource(output)")) {
		outputMapped = false;
		return false;
	}
	outputMapped = false;

	return VFXSucceeded(_impl->synchronize(_impl->stream), "NvVFX_CudaStreamSynchronize");
}

}

#else

namespace Magpie {

struct RTXVideoDenoiser::Impl {};

RTXVideoDenoiser::RTXVideoDenoiser() = default;
RTXVideoDenoiser::~RTXVideoDenoiser() = default;

bool RTXVideoDenoiser::Initialize(
	DeviceResources&, ID3D11Texture2D*, ID3D11Texture2D*, uint32_t) noexcept {
	Logger::Get().Error("RTX Video denoise is disabled at build time");
	return false;
}

bool RTXVideoDenoiser::Resize(DeviceResources&, ID3D11Texture2D*, ID3D11Texture2D*) noexcept {
	return false;
}

bool RTXVideoDenoiser::Draw(const NativeEffectDrawContext&) noexcept {
	return false;
}

}

#endif
