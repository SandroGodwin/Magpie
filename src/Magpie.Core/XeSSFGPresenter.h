#pragma once
#include "PresenterBase.h"

namespace Magpie {

// XeSS-FG is exposed as an effect, but Intel's API generates frames inside a
// D3D12 proxy swap chain. This presenter bridges Magpie's D3D11 frontend into
// that swap chain while providing flat depth and zero motion vectors.
class XeSSFGPresenter final : public PresenterBase {
public:
	struct Impl;

	explicit XeSSFGPresenter(uint32_t requestedMultiplier = 2);
	~XeSSFGPresenter() noexcept override;

	bool BeginFrame(
		winrt::com_ptr<ID3D11Texture2D>& frameTex,
		winrt::com_ptr<ID3D11RenderTargetView>& frameRtv,
		POINT& drawOffset
	) noexcept override;

	bool EndFrame(bool waitForGpu = false) noexcept override;

	bool UsesFrameLatencyWaitableObject() const noexcept override {
		return true;
	}

	bool OnResize() noexcept override;

protected:
	bool _Initialize(HWND hwndAttach) noexcept override;

private:
	std::unique_ptr<Impl> _impl;
	uint32_t _requestedMultiplier = 2;
};

}
