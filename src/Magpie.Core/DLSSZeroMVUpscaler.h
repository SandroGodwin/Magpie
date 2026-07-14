#pragma once
#include "HalfResOpticalFlow.h"

namespace Magpie {

class DeviceResources;

// Experimental DLSS-SR adapter for captured colour frames. Both depth and
// motion vectors are deliberately cleared, and jitter is fixed at zero.
class DLSSZeroMVUpscaler {
public:
	DLSSZeroMVUpscaler() = default;
	DLSSZeroMVUpscaler(const DLSSZeroMVUpscaler&) = delete;
	DLSSZeroMVUpscaler& operator=(const DLSSZeroMVUpscaler&) = delete;
	~DLSSZeroMVUpscaler();

	bool Initialize(
		DeviceResources& deviceResources,
		ID3D11Texture2D* input,
		ID3D11Texture2D* output,
		bool enableJitter = false,
		bool enableOpticalFlow = false
	) noexcept;

	bool Resize(
		DeviceResources& deviceResources,
		ID3D11Texture2D* input,
		ID3D11Texture2D* output
	) noexcept;

	bool Draw(ID3D11Texture2D* input, ID3D11Texture2D* output) noexcept;

private:
	void _Reset() noexcept;

	ID3D11Device5* _device = nullptr;
	ID3D11DeviceContext4* _d3dDC = nullptr;
	winrt::com_ptr<ID3D11Texture2D> _zeroMotionVectors;
	winrt::com_ptr<ID3D11UnorderedAccessView> _zeroMotionVectorsUav;
	winrt::com_ptr<ID3D11Texture2D> _zeroDepth;
	winrt::com_ptr<ID3D11UnorderedAccessView> _zeroDepthUav;
	void* _parameters = nullptr;
	void* _feature = nullptr;
	bool _ngxInitialized = false;
	bool _resetHistory = true;
	bool _enableJitter = false;
	bool _enableOpticalFlow = false;
	uint32_t _frameIndex = 0;
	std::unique_ptr<HalfResOpticalFlow> _opticalFlow;
};

}
