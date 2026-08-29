#pragma once

namespace Magpie {

class DeviceResources;
class NativeEffectBackend;
struct EffectOption;

struct NativeEffectBackendResult {
	bool recognized = false;
	std::unique_ptr<NativeEffectBackend> backend;
};

NativeEffectBackendResult CreateNativeEffectBackend(
	std::string_view effectName,
	const EffectOption& option,
	DeviceResources& resources,
	ID3D11Texture2D* input,
	ID3D11Texture2D* output
) noexcept;

}
