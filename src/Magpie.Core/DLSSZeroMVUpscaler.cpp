#include "pch.h"
#include "DLSSZeroMVUpscaler.h"
#include "DeviceResources.h"
#include "DirectXHelper.h"
#include "Logger.h"
#include "HalfResOpticalFlow.h"

#ifdef MP_ENABLE_DLSS_ZEROMV
#include <nvsdk_ngx.h>
#include <nvsdk_ngx_helpers.h>

namespace Magpie {

static bool NGXSucceeded(NVSDK_NGX_Result result) noexcept {
	return NVSDK_NGX_SUCCEED(result);
}

DLSSZeroMVUpscaler::~DLSSZeroMVUpscaler() {
	_Reset();
}

void DLSSZeroMVUpscaler::_Reset() noexcept {
	if (_feature) {
		NVSDK_NGX_D3D11_ReleaseFeature(static_cast<NVSDK_NGX_Handle*>(_feature));
		_feature = nullptr;
	}
	if (_parameters) {
		NVSDK_NGX_D3D11_DestroyParameters(static_cast<NVSDK_NGX_Parameter*>(_parameters));
		_parameters = nullptr;
	}
	if (_ngxInitialized) {
		NVSDK_NGX_D3D11_Shutdown1(_device);
		_ngxInitialized = false;
	}

	_zeroDepthUav = nullptr;
	_zeroDepth = nullptr;
	_zeroMotionVectorsUav = nullptr;
	_zeroMotionVectors = nullptr;
	_device = nullptr;
	_d3dDC = nullptr;
	_resetHistory = true;
	_enableJitter = false;
	_enableOpticalFlow = false;
	_opticalFlow.reset();
	_frameIndex = 0;
}

bool DLSSZeroMVUpscaler::Initialize(
	DeviceResources& deviceResources,
	ID3D11Texture2D* input,
	ID3D11Texture2D* output,
	bool enableJitter,
	bool enableOpticalFlow
) noexcept {
	_Reset();
	_enableJitter = enableJitter;
	_enableOpticalFlow = enableOpticalFlow;
	_device = deviceResources.GetD3DDevice();
	_d3dDC = deviceResources.GetD3DDC();

	D3D11_TEXTURE2D_DESC inputDesc{};
	D3D11_TEXTURE2D_DESC outputDesc{};
	input->GetDesc(&inputDesc);
	output->GetDesc(&outputDesc);
	if (inputDesc.Width > outputDesc.Width || inputDesc.Height > outputDesc.Height) {
		Logger::Get().Error("DLSS Zero-MV only supports upscaling");
		_Reset();
		return false;
	}

	_zeroMotionVectors = DirectXHelper::CreateTexture2D(
		_device,
		DXGI_FORMAT_R16G16_FLOAT,
		inputDesc.Width,
		inputDesc.Height,
		D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_UNORDERED_ACCESS
	);
	_zeroDepth = DirectXHelper::CreateTexture2D(
		_device,
		DXGI_FORMAT_R32_FLOAT,
		inputDesc.Width,
		inputDesc.Height,
		D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_UNORDERED_ACCESS
	);
	if (!_zeroMotionVectors || !_zeroDepth) {
		Logger::Get().Error("Create DLSS Zero-MV auxiliary textures failed");
		_Reset();
		return false;
	}
	if (_enableOpticalFlow) {
		_opticalFlow = std::make_unique<HalfResOpticalFlow>();
		if (!_opticalFlow->Initialize(_device, _d3dDC, input)) {
			Logger::Get().Error("Initialize DLSS half-resolution optical flow failed");
			_Reset();
			return false;
		}
	}

	HRESULT hr = _device->CreateUnorderedAccessView(
		_zeroMotionVectors.get(), nullptr, _zeroMotionVectorsUav.put());
	if (SUCCEEDED(hr)) {
		hr = _device->CreateUnorderedAccessView(
			_zeroDepth.get(), nullptr, _zeroDepthUav.put());
	}
	if (FAILED(hr)) {
		Logger::Get().ComError("Create DLSS Zero-MV UAV failed", hr);
		_Reset();
		return false;
	}

	// A stable custom project identifier is permitted by NGX when an NVIDIA
	// application ID has not been assigned to an integration.
	NVSDK_NGX_Result result = NVSDK_NGX_D3D11_Init_with_ProjectID(
		"7c134ab9-9677-4af5-a2b2-bca943350861",
		NVSDK_NGX_ENGINE_TYPE_CUSTOM,
		"Magpie-ZeroMV-1",
		L"logs",
		_device
	);
	if (!NGXSucceeded(result)) {
		Logger::Get().Error(fmt::format("NVSDK_NGX_D3D11_Init failed ({:#x})", (uint32_t)result));
		_Reset();
		return false;
	}
	_ngxInitialized = true;

	NVSDK_NGX_Parameter* parameters = nullptr;
	result = NVSDK_NGX_D3D11_GetCapabilityParameters(&parameters);
	if (!NGXSucceeded(result) || !parameters) {
		Logger::Get().Error(fmt::format("NVSDK_NGX_D3D11_GetCapabilityParameters failed ({:#x})", (uint32_t)result));
		_Reset();
		return false;
	}
	_parameters = parameters;

	// Preset J trades a little more flickering for less ghosting than the
	// default transformer preset K. That trade-off is preferable here because
	// captured frames have no valid motion vectors to reproject history with.
	NVSDK_NGX_Parameter_SetI(
		parameters,
		NVSDK_NGX_Parameter_DLSS_Hint_Render_Preset_Balanced,
		NVSDK_NGX_DLSS_Hint_Render_Preset_J
	);

	NVSDK_NGX_DLSS_Create_Params createParams{
		.Feature{
			.InWidth = inputDesc.Width,
			.InHeight = inputDesc.Height,
			.InTargetWidth = outputDesc.Width,
			.InTargetHeight = outputDesc.Height,
			.InPerfQualityValue = NVSDK_NGX_PerfQuality_Value_Balanced
		},
		.InFeatureCreateFlags = NVSDK_NGX_DLSS_Feature_Flags_MVLowRes |
			NVSDK_NGX_DLSS_Feature_Flags_AutoExposure,
		.InEnableOutputSubrects = false
	};

	NVSDK_NGX_Handle* feature = nullptr;
	result = NGX_D3D11_CREATE_DLSS_EXT(_d3dDC, &feature, parameters, &createParams);
	if (!NGXSucceeded(result) || !feature) {
		Logger::Get().Error(fmt::format("NGX_D3D11_CREATE_DLSS_EXT failed ({:#x})", (uint32_t)result));
		_Reset();
		return false;
	}
	_feature = feature;

	Logger::Get().Info(fmt::format(
		"DLSS experimental backend initialized (Balanced, preset J, jitter={}, opticalFlow={}): {}x{} -> {}x{}",
		_enableJitter, _enableOpticalFlow,
		inputDesc.Width, inputDesc.Height, outputDesc.Width, outputDesc.Height));
	return true;
}

bool DLSSZeroMVUpscaler::Resize(
	DeviceResources& deviceResources,
	ID3D11Texture2D* input,
	ID3D11Texture2D* output
) noexcept {
	const bool enableJitter = _enableJitter;
	const bool enableOpticalFlow = _enableOpticalFlow;
	return Initialize(deviceResources, input, output, enableJitter, enableOpticalFlow);
}

static float Halton(uint32_t index, uint32_t base) noexcept {
	float result = 0.0f;
	float fraction = 1.0f;
	while (index) {
		fraction /= (float)base;
		result += fraction * (float)(index % base);
		index /= base;
	}
	return result;
}

bool DLSSZeroMVUpscaler::Draw(ID3D11Texture2D* input, ID3D11Texture2D* output) noexcept {
	if (!_feature || !_parameters || !_zeroMotionVectorsUav || !_zeroDepthUav) {
		return false;
	}

	static constexpr float ZERO[4]{};
	ID3D11Texture2D* motionVectors = _zeroMotionVectors.get();
	if (_enableOpticalFlow) {
		if (!_opticalFlow || !_opticalFlow->Estimate(input)) return false;
		motionVectors = _opticalFlow->GetMotionTexture();
	} else {
		_d3dDC->ClearUnorderedAccessViewFloat(_zeroMotionVectorsUav.get(), ZERO);
	}
	_d3dDC->ClearUnorderedAccessViewFloat(_zeroDepthUav.get(), ZERO);

	D3D11_TEXTURE2D_DESC inputDesc{};
	input->GetDesc(&inputDesc);
	NVSDK_NGX_D3D11_DLSS_Eval_Params evalParams{};
	evalParams.Feature.pInColor = input;
	evalParams.Feature.pInOutput = output;
	evalParams.pInDepth = _zeroDepth.get();
	evalParams.pInMotionVectors = motionVectors;
	if (_enableJitter) {
		// An 8-sample Halton(2,3) sequence centered around zero. Since Magpie
		// cannot jitter the source application's projection, this is deliberately
		// exposed as a separate metadata-only experiment.
		const uint32_t sample = (_frameIndex++ & 7u) + 1u;
		evalParams.InJitterOffsetX = Halton(sample, 2) - 0.5f;
		evalParams.InJitterOffsetY = Halton(sample, 3) - 0.5f;
	} else {
		evalParams.InJitterOffsetX = 0.0f;
		evalParams.InJitterOffsetY = 0.0f;
	}
	evalParams.InRenderSubrectDimensions = { inputDesc.Width, inputDesc.Height };
	evalParams.InReset = _resetHistory ? 1 : 0;
	evalParams.InMVScaleX = 1.0f;
	evalParams.InMVScaleY = 1.0f;
	evalParams.InPreExposure = 1.0f;
	evalParams.InExposureScale = 1.0f;

	const NVSDK_NGX_Result result = NGX_D3D11_EVALUATE_DLSS_EXT(
		_d3dDC,
		static_cast<NVSDK_NGX_Handle*>(_feature),
		static_cast<NVSDK_NGX_Parameter*>(_parameters),
		&evalParams
	);
	if (!NGXSucceeded(result)) {
		Logger::Get().Error(fmt::format("NGX_D3D11_EVALUATE_DLSS_EXT failed ({:#x})", (uint32_t)result));
		return false;
	}

	_resetHistory = false;
	return true;
}

}

#else

namespace Magpie {

DLSSZeroMVUpscaler::~DLSSZeroMVUpscaler() = default;
void DLSSZeroMVUpscaler::_Reset() noexcept {}

bool DLSSZeroMVUpscaler::Initialize(DeviceResources&, ID3D11Texture2D*, ID3D11Texture2D*, bool, bool) noexcept {
	Logger::Get().Error("DLSS Zero-MV is disabled at build time");
	return false;
}

bool DLSSZeroMVUpscaler::Resize(DeviceResources&, ID3D11Texture2D*, ID3D11Texture2D*) noexcept {
	return false;
}

bool DLSSZeroMVUpscaler::Draw(ID3D11Texture2D*, ID3D11Texture2D*) noexcept {
	return false;
}

}

#endif
