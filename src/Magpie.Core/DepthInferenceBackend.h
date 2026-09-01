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

enum class SharedDepthBackendState : uint8_t {
	Initializing,
	Ready,
	Failed
};

// Process-wide TensorRT backend for one exact model/GPU/input-shape
// configuration. The first acquisition starts initialization asynchronously;
// later scaling sessions reuse the same initialized inference session.
class SharedTensorRTDepthBackend {
public:
	struct Impl;

	SharedTensorRTDepthBackend(const SharedTensorRTDepthBackend&) = delete;
	SharedTensorRTDepthBackend& operator=(const SharedTensorRTDepthBackend&) = delete;
	~SharedTensorRTDepthBackend();

	SharedDepthBackendState State() const noexcept;
	std::string_view Name() const noexcept;
	bool Run(
		const DepthInferenceInput& input,
		DepthInferenceOutput& output
	) noexcept;

private:
	explicit SharedTensorRTDepthBackend(DepthInferenceConfig config);
	void Initialize() noexcept;

	std::unique_ptr<Impl> _impl;

	friend std::shared_ptr<SharedTensorRTDepthBackend>
		AcquireSharedTensorRTDepthBackend(const DepthInferenceConfig& config);
};

std::shared_ptr<SharedTensorRTDepthBackend>
	AcquireSharedTensorRTDepthBackend(const DepthInferenceConfig& config);

std::unique_ptr<IDepthInferenceBackend> CreateTensorRTDepthBackend();
std::unique_ptr<IDepthInferenceBackend> CreateDirectMLDepthBackend();

}
