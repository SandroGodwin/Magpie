#pragma once

namespace Magpie {

class DeviceResources;

// Common lifetime and rendering contract for native SDK-backed effects.
// Creation parameters remain in NativeEffectBackendFactory so Renderer does
// not need one parallel container and one name-dispatch branch per SDK.
class NativeEffectBackend {
public:
	virtual ~NativeEffectBackend() = default;

	virtual bool Resize(
		DeviceResources& resources,
		ID3D11Texture2D* input,
		ID3D11Texture2D* output
	) noexcept = 0;

	virtual bool Draw(ID3D11Texture2D* input, ID3D11Texture2D* output) noexcept = 0;
};

}
