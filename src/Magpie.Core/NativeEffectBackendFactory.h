#pragma once

namespace Magpie {

class DeviceResources;
class NativeEffectBackend;

struct NativeEffectBackendResult {
	bool recognized = false;
	std::unique_ptr<NativeEffectBackend> backend;
};

NativeEffectBackendResult CreateNativeEffectBackend(
	std::string_view effectName,
	DeviceResources& resources,
	ID3D11Texture2D* input,
	ID3D11Texture2D* output
) noexcept;

}
