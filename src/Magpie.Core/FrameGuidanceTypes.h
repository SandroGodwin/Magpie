#pragma once

namespace Magpie {

using FrameGuidanceFrameId = uint64_t;

struct FrameGuidanceExtent {
	uint32_t width = 0;
	uint32_t height = 0;

	bool IsValid() const noexcept {
		return width != 0 && height != 0;
	}

	bool operator==(const FrameGuidanceExtent&) const noexcept = default;
};

struct FrameGuidanceRegion {
	uint32_t x = 0;
	uint32_t y = 0;
	uint32_t width = 0;
	uint32_t height = 0;

	static FrameGuidanceRegion Full(FrameGuidanceExtent extent) noexcept {
		return { 0, 0, extent.width, extent.height };
	}

	bool IsInside(FrameGuidanceExtent extent) const noexcept {
		return width != 0 && height != 0 && x <= extent.width &&
			y <= extent.height && width <= extent.width - x &&
			height <= extent.height - y;
	}

	bool operator==(const FrameGuidanceRegion&) const noexcept = default;
};

enum class FrameGuidanceResetReason : uint8_t {
	None,
	Initialize,
	Resize,
	SceneChange,
	CaptureInterrupted,
	DeviceRecreated,
	LongPause,
	ProviderFailure
};

enum class FrameGuidanceMotionDirection : uint8_t {
	CurrentToPrevious
};

enum class FrameGuidanceMotionUnit : uint8_t {
	SourcePixels
};

enum class FrameGuidanceDepthConvention : uint8_t {
	RelativeInverse
};

struct FrameGuidanceRequirements {
	bool zero = false;
	bool motion = false;
	bool depth = false;
	uint32_t depthInferenceInterval = 1;

	bool Any() const noexcept { return zero || motion || depth; }

	void Merge(const FrameGuidanceRequirements& other) noexcept {
		zero = zero || other.zero;
		motion = motion || other.motion;
		if (other.depth) {
			depthInferenceInterval = depth ?
				std::min(depthInferenceInterval, other.depthInferenceInterval) :
				other.depthInferenceInterval;
			depth = true;
		}
	}

	bool operator==(const FrameGuidanceRequirements&) const noexcept = default;
};

struct FrameGuidanceSyncPoint {
	// Null means that the texture is ordered on Renderer’s immediate D3D11
	// context. Providers that execute elsewhere must publish a fence and value.
	ID3D11Fence* fence = nullptr;
	uint64_t value = 0;
};

struct FrameGuidanceMetadata {
	FrameGuidanceFrameId frameId = 0;
	FrameGuidanceExtent sourceExtent{};
	FrameGuidanceRegion validRegion{};
	FrameGuidanceSyncPoint sync{};
	FrameGuidanceResetReason resetReason = FrameGuidanceResetReason::None;
	bool valid = false;
	bool isZero = false;
	bool requiresHistoryReset = false;
};

struct FrameGuidanceResource {
	ID3D11Texture2D* texture = nullptr;
	DXGI_FORMAT format = DXGI_FORMAT_UNKNOWN;
	FrameGuidanceMetadata metadata{};

	bool IsValid(
		DXGI_FORMAT expectedFormat,
		FrameGuidanceFrameId expectedFrameId,
		FrameGuidanceExtent expectedExtent
	) const noexcept {
		return texture && format == expectedFormat && metadata.valid &&
			metadata.frameId == expectedFrameId &&
			metadata.sourceExtent == expectedExtent &&
			metadata.validRegion.IsInside(expectedExtent);
	}
};

struct FrameGuidanceView {
	FrameGuidanceResource depth{};
	FrameGuidanceResource motion{};
	FrameGuidanceResource confidence{};
	// Optional diagnostics supplied by learned depth providers. They are not
	// part of the DLSS consumption contract.
	FrameGuidanceResource rawDepth{};
	FrameGuidanceResource depthResidual{};
	FrameGuidanceMotionDirection motionDirection =
		FrameGuidanceMotionDirection::CurrentToPrevious;
	FrameGuidanceMotionUnit motionUnit = FrameGuidanceMotionUnit::SourcePixels;
	FrameGuidanceDepthConvention depthConvention =
		FrameGuidanceDepthConvention::RelativeInverse;
	bool depthInverted = true;
	bool requiresHistoryReset = false;

	bool IsValidFor(
		FrameGuidanceFrameId frameId,
		FrameGuidanceExtent extent
	) const noexcept {
		return depth.IsValid(DXGI_FORMAT_R32_FLOAT, frameId, extent) &&
			motion.IsValid(DXGI_FORMAT_R16G16_FLOAT, frameId, extent) &&
			confidence.IsValid(DXGI_FORMAT_R8_UNORM, frameId, extent) &&
			depth.metadata.validRegion == motion.metadata.validRegion &&
			motion.metadata.validRegion == confidence.metadata.validRegion;
	}
};

inline FrameGuidanceView SelectFrameGuidanceChannels(
	const FrameGuidanceView& produced,
	const FrameGuidanceView& zero,
	FrameGuidanceFrameId frameId,
	FrameGuidanceExtent extent,
	bool useMotion,
	bool useDepth
) noexcept {
	if (!zero.IsValidFor(frameId, extent)) return {};

	FrameGuidanceView selected = produced.IsValidFor(frameId, extent) ?
		produced : zero;
	if (!useMotion) {
		selected.motion = zero.motion;
		selected.confidence = zero.confidence;
	}
	if (!useDepth) {
		selected.depth = zero.depth;
		selected.rawDepth = {};
		selected.depthResidual = {};
	}
	selected.requiresHistoryReset =
		selected.depth.metadata.requiresHistoryReset ||
		selected.motion.metadata.requiresHistoryReset ||
		selected.confidence.metadata.requiresHistoryReset;
	return selected.IsValidFor(frameId, extent) ? selected : zero;
}

struct MotionVectorProviderOutput;

struct FrameGuidanceFrame {
	ID3D11Texture2D* color = nullptr;
	FrameGuidanceFrameId frameId = 0;
	FrameGuidanceExtent sourceExtent{};
	FrameGuidanceRegion validRegion{};
	// Motion is produced first so depth providers can reproject their previous
	// result without owning or instantiating a motion provider.
	const MotionVectorProviderOutput* motionGuidance = nullptr;
};

struct DepthProviderOutput {
	FrameGuidanceResource depth{};
	FrameGuidanceResource rawDepth{};
	FrameGuidanceResource depthResidual{};
};

struct MotionVectorProviderOutput {
	FrameGuidanceResource motion{};
	FrameGuidanceResource confidence{};
};

}
