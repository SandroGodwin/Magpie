#pragma once
#include "NativeEffectBackend.h"

namespace Magpie {

enum class FrameGuidanceDiagnosticKind : uint8_t {
	Motion,
	Confidence,
	Depth,
	DepthResidual
};

struct FrameGuidanceDiagnosticSettings {
	FrameGuidanceDiagnosticKind kind = FrameGuidanceDiagnosticKind::Motion;
	float gain = 1.0f;
	bool invert = false;
	bool showRawDepth = false;
};

class FrameGuidanceDiagnostics final : public NativeEffectBackend {
public:
	FrameGuidanceRequirements GetFrameGuidanceRequirements() const noexcept override;
	bool Initialize(
		DeviceResources& resources,
		ID3D11Texture2D* input,
		ID3D11Texture2D* output,
		const FrameGuidanceDiagnosticSettings& settings
	) noexcept;
	bool Resize(
		DeviceResources& resources,
		ID3D11Texture2D* input,
		ID3D11Texture2D* output
	) noexcept override;
	bool Draw(const NativeEffectDrawContext& context) noexcept override;

private:
	ID3D11Device5* _device = nullptr;
	ID3D11DeviceContext4* _context = nullptr;
	FrameGuidanceDiagnosticSettings _settings;
	winrt::com_ptr<ID3D11ComputeShader> _shader;
	winrt::com_ptr<ID3D11Buffer> _params;
};

}
