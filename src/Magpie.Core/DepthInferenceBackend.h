#pragma once

namespace Magpie {

struct DepthInferenceConfig {
	std::filesystem::path modelPath;
	std::filesystem::path runtimeDirectory;
	std::filesystem::path engineCacheDirectory;
	uint32_t adapterIndex = 0;
	uint32_t inputWidth = 0;
	uint32_t inputHeight = 0;
	bool isNvidiaAdapter = false;
};

struct DepthInferenceInput {
	const float* values = nullptr;
	size_t valueCount = 0;
	uint32_t width = 0;
	uint32_t height = 0;
};

struct DepthInferenceOutput {
	std::vector<float> values;
	uint32_t width = 0;
	uint32_t height = 0;
};

class IDepthInferenceBackend {
public:
	virtual ~IDepthInferenceBackend() = default;
	virtual std::string_view Name() const noexcept = 0;
	virtual bool Initialize(const DepthInferenceConfig& config) noexcept = 0;
	virtual bool Run(
		const DepthInferenceInput& input,
		DepthInferenceOutput& output
	) noexcept = 0;
	virtual bool IsReady() const noexcept = 0;
};

std::unique_ptr<IDepthInferenceBackend> CreateTensorRTDepthBackend();
std::unique_ptr<IDepthInferenceBackend> CreateDirectMLDepthBackend();

}
