#include "pch.h"
#include "NativeEffectBackendFactory.h"
#include "DLSSZeroMVUpscaler.h"
#include "FSR2ZeroMVUpscaler.h"
#include "FSR3ZeroMVUpscaler.h"
#include "RTXVideoDenoiser.h"
#include "XeSSZeroMVUpscaler.h"
#include "Logger.h"

namespace Magpie {

template <typename T, typename... Args>
static NativeEffectBackendResult CreateBackend(
	std::string_view displayName,
	DeviceResources& resources,
	ID3D11Texture2D* input,
	ID3D11Texture2D* output,
	Args&&... args
) noexcept {
	auto backend = std::make_unique<T>();
	if (!backend->Initialize(
		resources, input, output, std::forward<Args>(args)...)) {
		Logger::Get().Error(fmt::format("Initialize native effect {} failed", displayName));
		return { true, nullptr };
	}
	return { true, std::move(backend) };
}

NativeEffectBackendResult CreateNativeEffectBackend(
	std::string_view effectName,
	DeviceResources& resources,
	ID3D11Texture2D* input,
	ID3D11Texture2D* output
) noexcept {
	if (effectName == "DLSS\\DLSS_ZeroMV" ||
		effectName == "DLSS\\DLSS_ZeroMV_Jitter" ||
		effectName == "DLSS\\DLSS_OpticalFlow") {
		return CreateBackend<DLSSZeroMVUpscaler>(effectName, resources, input, output,
			effectName == "DLSS\\DLSS_ZeroMV_Jitter",
			effectName == "DLSS\\DLSS_OpticalFlow");
	}

	if (effectName == "FSR2\\FSR2_ZeroMV" ||
		effectName == "FSR2\\FSR2_ZeroMV_Jitter" ||
		effectName == "FSR2\\FSR2_OpticalFlow") {
		return CreateBackend<FSR2ZeroMVUpscaler>(effectName, resources, input, output,
			effectName == "FSR2\\FSR2_OpticalFlow",
			effectName == "FSR2\\FSR2_ZeroMV_Jitter");
	}

	const bool isFsr3 = effectName == "FSR3\\FSR3_ZeroMV" ||
		effectName == "FSR3\\FSR3_ZeroMV_Jitter" ||
		effectName == "FSR3\\FSR3_OpticalFlow";
	const bool isFsr4 = effectName == "FSR4\\FSR4_ZeroMV" ||
		effectName == "FSR4\\FSR4_ZeroMV_Jitter" ||
		effectName == "FSR4\\FSR4_OpticalFlow";
	if (isFsr3 || isFsr4) {
		return CreateBackend<FSR3ZeroMVUpscaler>(effectName, resources, input, output,
			effectName.ends_with("OpticalFlow"),
			effectName.ends_with("ZeroMV_Jitter"), isFsr4);
	}

	if (effectName == "XeSS\\XeSS_ZeroMV" ||
		effectName == "XeSS\\XeSS_ZeroMV_Jitter" ||
		effectName == "XeSS\\XeSS_OpticalFlow") {
		return CreateBackend<XeSSZeroMVUpscaler>(effectName, resources, input, output,
			effectName == "XeSS\\XeSS_OpticalFlow",
			effectName == "XeSS\\XeSS_ZeroMV_Jitter");
	}

	const bool isRtxVideo = effectName.starts_with("RTXVideo\\RTXVideo_Denoise_") ||
		effectName.starts_with("RTXVideo\\RTXVideo_VSR_");
	if (isRtxVideo) {
		const bool isVsr = effectName.find("_VSR_") != std::string_view::npos;
		uint32_t qualityLevel = 8;
		if (isVsr) {
			qualityLevel = effectName.ends_with("_Low") ? 1 :
				effectName.ends_with("_Medium") ? 2 :
				effectName.ends_with("_High") ? 3 : 4;
		} else if (effectName.ends_with("_Medium")) {
			qualityLevel = 9;
		} else if (effectName.ends_with("_High")) {
			qualityLevel = 10;
		} else if (effectName.ends_with("_Ultra")) {
			qualityLevel = 11;
		}
		return CreateBackend<RTXVideoDenoiser>(
			effectName, resources, input, output, qualityLevel);
	}

	return {};
}

}
