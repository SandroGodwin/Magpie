#pragma once
#include "DepthInferenceBackend.h"
#include "FrameGuidanceProvider.h"

namespace Magpie {

class DepthAnythingV2Provider final : public IDepthProvider {
public:
	struct Impl;

	explicit DepthAnythingV2Provider(uint32_t inferenceInterval = 1);
	DepthAnythingV2Provider(const DepthAnythingV2Provider&) = delete;
	DepthAnythingV2Provider& operator=(const DepthAnythingV2Provider&) = delete;
	~DepthAnythingV2Provider() override;

	bool Initialize(
		DeviceResources& resources,
		FrameGuidanceExtent sourceExtent
	) noexcept override;
	bool BeginFrame(
		const FrameGuidanceFrame& frame,
		DepthProviderOutput& output
	) noexcept override;
	void Reset(FrameGuidanceResetReason reason) noexcept override;
	bool Resize(FrameGuidanceExtent sourceExtent) noexcept override;

private:
	std::unique_ptr<Impl> _impl;
};

}
