#pragma once

namespace Magpie {

class DeviceResources;

// Experimental FSR 3.1.5 upscaler running on D3D12 through resources shared
// with Magpie's D3D11 renderer. Frame generation is deliberately not included.
class FSR3ZeroMVUpscaler {
public:
	struct Impl;

	FSR3ZeroMVUpscaler();
	FSR3ZeroMVUpscaler(const FSR3ZeroMVUpscaler&) = delete;
	FSR3ZeroMVUpscaler& operator=(const FSR3ZeroMVUpscaler&) = delete;
	~FSR3ZeroMVUpscaler();

	bool Initialize(DeviceResources& resources, ID3D11Texture2D* input,
		ID3D11Texture2D* output, bool enableOpticalFlow = false) noexcept;
	bool Resize(DeviceResources& resources, ID3D11Texture2D* input,
		ID3D11Texture2D* output) noexcept;
	bool Draw(ID3D11Texture2D* input, ID3D11Texture2D* output) noexcept;

private:
	std::unique_ptr<Impl> _impl;
	bool _enableOpticalFlow = false;
};

}
