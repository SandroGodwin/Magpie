#pragma once

namespace Magpie {

class DeviceResources;

// Experimental DLSS Frame Generation adapter. Magpie only has captured color
// frames, so depth and motion vectors are virtual zero-filled resources.
class DLSSFrameGenerator {
public:
	struct Impl;
	using PublishCallback = std::function<bool(ID3D11Texture2D*)>;

	DLSSFrameGenerator();
	DLSSFrameGenerator(const DLSSFrameGenerator&) = delete;
	DLSSFrameGenerator& operator=(const DLSSFrameGenerator&) = delete;
	~DLSSFrameGenerator();

	bool Initialize(DeviceResources& resources, ID3D11Texture2D* input,
		uint32_t multiplier) noexcept;
	bool Resize(DeviceResources& resources, ID3D11Texture2D* input) noexcept;
	bool Draw(ID3D11Texture2D* input,
		const PublishCallback& publishGeneratedFrame) noexcept;
	void RequestHistoryReset() noexcept;

	uint32_t Multiplier() const noexcept;

private:
	std::unique_ptr<Impl> _impl;
	uint32_t _requestedMultiplier = 2;
};

}
