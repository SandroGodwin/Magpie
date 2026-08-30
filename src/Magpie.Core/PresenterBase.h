#pragma once

namespace Magpie {

class DeviceResources;

class PresenterBase {
public:
	virtual ~PresenterBase() noexcept {}

	bool Initialize(HWND hwndAttach, const DeviceResources& deviceResources) noexcept;

	virtual bool BeginFrame(
		winrt::com_ptr<ID3D11Texture2D>& frameTex,
		winrt::com_ptr<ID3D11RenderTargetView>& frameRtv,
		POINT& drawOffset
	) noexcept = 0;

	// Returns true when the frame was submitted to the presentation backend.
	// Successful status codes such as DXGI_STATUS_OCCLUDED still count as a
	// submission so a deliberately hidden first-frame window can be shown.
	virtual bool EndFrame(bool waitForGpu = false) noexcept = 0;

	// A presenter with a frame-latency waitable object already provides queue
	// capacity pacing in BeginFrame. DLSSFG must not add a DWM wait on top of it.
	virtual bool UsesFrameLatencyWaitableObject() const noexcept {
		return false;
	}

	virtual bool OnResize() noexcept = 0;

	virtual void OnEndResize(bool& shouldRedraw) noexcept {
		shouldRedraw = false;
	}

protected:
	virtual bool _Initialize(HWND hwndAttach) noexcept = 0;

	void _WaitForGpu() noexcept;

	static uint32_t _CalcBufferCount() noexcept;

	const DeviceResources* _deviceResources = nullptr;

private:
	winrt::com_ptr<ID3D11Fence> _fence;
	uint64_t _fenceValue = 0;
	wil::unique_event_nothrow _fenceEvent;
};

}
