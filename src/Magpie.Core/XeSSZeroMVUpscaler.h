#pragma once

namespace Magpie {

class DeviceResources;

// Experimental colour-only XeSS-SR adapter. Magpie renders with D3D11, while
// the cross-vendor XeSS path is D3D12, so resources are shared between APIs.
class XeSSZeroMVUpscaler {
public:
	struct Impl;

	XeSSZeroMVUpscaler();
	XeSSZeroMVUpscaler(const XeSSZeroMVUpscaler&) = delete;
	XeSSZeroMVUpscaler& operator=(const XeSSZeroMVUpscaler&) = delete;
	~XeSSZeroMVUpscaler();

	bool Initialize(
		DeviceResources& deviceResources,
		ID3D11Texture2D* input,
		ID3D11Texture2D* output,
		bool enableOpticalFlow = false,
		bool enableJitter = false
	) noexcept;

	bool Resize(
		DeviceResources& deviceResources,
		ID3D11Texture2D* input,
		ID3D11Texture2D* output
	) noexcept;

	bool Draw(ID3D11Texture2D* input, ID3D11Texture2D* output) noexcept;

private:
	std::unique_ptr<Impl> _impl;
	bool _enableOpticalFlow = false;
	bool _enableJitter = false;
};

}
