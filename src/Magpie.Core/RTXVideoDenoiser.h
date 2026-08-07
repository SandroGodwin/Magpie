#pragma once
#include "NativeEffectBackend.h"

namespace Magpie {

class DeviceResources;

// NVIDIA VideoSuperRes modes 8-11 perform same-resolution denoising. The
// native backend uses D3D11/CUDA interop, so no frame is copied through CPU.
class RTXVideoDenoiser final : public NativeEffectBackend {
public:
	RTXVideoDenoiser();
	RTXVideoDenoiser(const RTXVideoDenoiser&) = delete;
	RTXVideoDenoiser& operator=(const RTXVideoDenoiser&) = delete;
	~RTXVideoDenoiser() override;

	bool Initialize(
		DeviceResources& deviceResources,
		ID3D11Texture2D* input,
		ID3D11Texture2D* output,
		uint32_t qualityLevel
	) noexcept;

	bool Resize(
		DeviceResources& deviceResources,
		ID3D11Texture2D* input,
		ID3D11Texture2D* output
	) noexcept override;

	bool Draw(ID3D11Texture2D* input, ID3D11Texture2D* output) noexcept override;

private:
	struct Impl;
	std::unique_ptr<Impl> _impl;
	uint32_t _qualityLevel = 8;
};

}
