#include "pch.h"
#include "DepthInferenceBackend.h"
#include "Logger.h"

#ifdef MP_ENABLE_DEPTH_ANYTHING_V2
#include <onnxruntime_c_api.h>
#include <dml_provider_factory.h>
#include <d3d12.h>
#include <atomic>
#include <mutex>
#include <thread>
#include <unordered_map>

namespace Magpie {

namespace {

constexpr char MODEL_INPUT_NAME[] = "pixel_values";
constexpr char MODEL_OUTPUT_NAME[] = "predicted_depth";

class OnnxRuntimeDepthBackend : public IDepthInferenceBackend {
public:
	explicit OnnxRuntimeDepthBackend(bool tensorRT) noexcept :
		_tensorRT(tensorRT) {}
	~OnnxRuntimeDepthBackend() override { _Release(); }

	std::string_view Name() const noexcept override {
		return _tensorRT ? "TensorRT FP16" : "ONNX Runtime DirectML FP16";
	}

	bool Initialize(const DepthInferenceConfig& config) noexcept override {
		_Release();
		if (_tensorRT && !config.isNvidiaAdapter) return false;
		if (!config.inputWidth || !config.inputHeight) return false;
		if (!std::filesystem::is_regular_file(config.modelPath)) {
			Logger::Get().Warn(fmt::format(
				"{} unavailable: model not found at {}", Name(),
				config.modelPath.string()));
			return false;
		}
		if (_tensorRT) {
			const std::filesystem::path localCudnn =
				config.runtimeDirectory / L"cudnn64_9.dll";
			std::wstring found(32768, L'\0');
			const DWORD length = SearchPathW(
				nullptr, L"cudnn64_9.dll", nullptr,
				static_cast<DWORD>(found.size()), found.data(), nullptr);
			if (!std::filesystem::is_regular_file(localCudnn) &&
				(!length || length >= found.size())) {
				Logger::Get().Info(
					"TensorRT FP16 skipped: cudnn64_9.dll is unavailable");
				return false;
			}
		}

		const std::filesystem::path dllPath =
			config.runtimeDirectory / L"onnxruntime.dll";
		_module = LoadLibraryExW(
			dllPath.c_str(), nullptr, LOAD_WITH_ALTERED_SEARCH_PATH);
		if (!_module) {
			Logger::Get().Warn(fmt::format(
				"{} unavailable: failed to load {} (Win32={})", Name(),
				dllPath.string(), GetLastError()));
			return false;
		}

		using GetApiBaseFn = const OrtApiBase*(ORT_API_CALL*)();
		auto getApiBase = reinterpret_cast<GetApiBaseFn>(
			GetProcAddress(_module, "OrtGetApiBase"));
		if (!getApiBase || !(_api = getApiBase()->GetApi(ORT_API_VERSION))) {
			_Release();
			return false;
		}

		const auto start = std::chrono::steady_clock::now();
		if (!_Check(_api->CreateEnv(
			ORT_LOGGING_LEVEL_WARNING,
			_tensorRT ? "MagpieDAV2TensorRT" : "MagpieDAV2DirectML", &_env)) ||
			!_Check(_api->CreateSessionOptions(&_sessionOptions)) ||
			// ORT_ENABLE_BASIC: SimplifiedLayerNormFusion 在 DAV2 模型上触发 ORT
			// graph_utils.cc GetIndexFromName 断言（TRT EP 分区路径），降级到 BASIC 绕开；
			// TRT EP 有自己的 kernel，扩展级图融合收益极小
			!_Check(_api->SetSessionGraphOptimizationLevel(
				_sessionOptions, ORT_ENABLE_BASIC)) ||
			!_Check(_api->AddFreeDimensionOverrideByName(
				_sessionOptions, "batch_size", 1)) ||
			!_Check(_api->AddFreeDimensionOverrideByName(
				_sessionOptions, "height", config.inputHeight)) ||
			!_Check(_api->AddFreeDimensionOverrideByName(
				_sessionOptions, "width", config.inputWidth))) {
			_Release();
			return false;
		}

		bool providerReady = false;
		if (_tensorRT) {
			providerReady = _AppendTensorRT(config);
		} else {
			providerReady = _AppendDirectML(config);
		}
		if (!providerReady ||
			!_Check(_api->CreateSession(
				_env, config.modelPath.c_str(), _sessionOptions, &_session)) ||
			!_Check(_api->CreateCpuMemoryInfo(
				OrtArenaAllocator, OrtMemTypeDefault, &_memoryInfo))) {
			_Release();
			return false;
		}
		_inputWidth = config.inputWidth;
		_inputHeight = config.inputHeight;
		const size_t plane = size_t(_inputWidth) * _inputHeight;
		_inputBuffer.resize(plane * 3);
		_outputBuffer.resize(plane);
		const int64_t inputDimensions[]{
			1, 3, static_cast<int64_t>(_inputHeight),
			static_cast<int64_t>(_inputWidth)
		};
		const int64_t outputDimensions[]{
			1, static_cast<int64_t>(_inputHeight),
			static_cast<int64_t>(_inputWidth)
		};
		if (!_Check(_api->CreateTensorWithDataAsOrtValue(
			_memoryInfo, _inputBuffer.data(), _inputBuffer.size() * sizeof(float),
			inputDimensions, ARRAYSIZE(inputDimensions),
			ONNX_TENSOR_ELEMENT_DATA_TYPE_FLOAT, &_inputValue)) ||
			!_Check(_api->CreateTensorWithDataAsOrtValue(
				_memoryInfo, _outputBuffer.data(), _outputBuffer.size() * sizeof(float),
				outputDimensions, ARRAYSIZE(outputDimensions),
				ONNX_TENSOR_ELEMENT_DATA_TYPE_FLOAT, &_outputValue))) {
			_Release();
			return false;
		}

		const auto elapsed = std::chrono::duration<double, std::milli>(
			std::chrono::steady_clock::now() - start).count();
		Logger::Get().Info(fmt::format(
			"Frame Guidance depth backend initialized: {} in {:.1f} ms",
			Name(), elapsed));
		return true;
	}

	bool Run(
		const DepthInferenceInput& input,
		DepthInferenceOutput& output
	) noexcept override {
		if (!_session || !_inputValue || !_outputValue || !input.values ||
			input.width != _inputWidth || input.height != _inputHeight ||
			input.valueCount != _inputBuffer.size()) {
			return false;
		}
		std::copy_n(input.values, input.valueCount, _inputBuffer.data());
		const char* inputNames[]{ MODEL_INPUT_NAME };
		const char* outputNames[]{ MODEL_OUTPUT_NAME };
		const OrtValue* inputValues[]{ _inputValue };
		OrtValue* outputValue = _outputValue;
		const bool ran = _Check(_api->Run(
			_session, nullptr, inputNames,
			inputValues, 1,
			outputNames, 1, &outputValue));
		if (!ran || outputValue != _outputValue) return false;
		output.width = _inputWidth;
		output.height = _inputHeight;
		output.values.assign(_outputBuffer.begin(), _outputBuffer.end());
		return true;
	}

	bool IsReady() const noexcept override { return _session != nullptr; }

private:
	using AppendDmlFn = OrtStatus*(ORT_API_CALL*)(OrtSessionOptions*, int);

	bool _AppendTensorRT(const DepthInferenceConfig& config) noexcept {
		OrtTensorRTProviderOptionsV2* options = nullptr;
		if (!_Check(_api->CreateTensorRTProviderOptions(&options))) return false;
		std::error_code ec;
		std::filesystem::create_directories(config.engineCacheDirectory, ec);
		const std::string cachePath = config.engineCacheDirectory.string();
		const char* keys[]{
			"device_id", "trt_fp16_enable", "trt_int8_enable",
			"trt_engine_cache_enable", "trt_engine_cache_path",
			"trt_timing_cache_enable", "trt_timing_cache_path",
			"trt_engine_hw_compatible", "trt_builder_optimization_level",
			"trt_profile_min_shapes", "trt_profile_opt_shapes",
			"trt_profile_max_shapes"
		};
		const std::string deviceId = std::to_string(config.adapterIndex);
		const std::string fixedShape = fmt::format(
			"pixel_values:1x3x{}x{}", config.inputHeight, config.inputWidth);
		// 布尔键必须用 True/False（与官方 ORT TRT EP 的解析保持一致）
		const char* values[]{
			deviceId.c_str(), "True", "False", "True", cachePath.c_str(),
			"True", cachePath.c_str(), "False", "3",
			fixedShape.c_str(), fixedShape.c_str(), fixedShape.c_str()
		};
		const bool result = _Check(_api->UpdateTensorRTProviderOptions(
			options, keys, values, ARRAYSIZE(keys))) &&
			_Check(_api->SessionOptionsAppendExecutionProvider_TensorRT_V2(
				_sessionOptions, options));
		_api->ReleaseTensorRTProviderOptions(options);
		return result;
	}

	bool _AppendDirectML(const DepthInferenceConfig& config) noexcept {
		if (!_Check(_api->DisableMemPattern(_sessionOptions)) ||
			!_Check(_api->SetSessionExecutionMode(
				_sessionOptions, ORT_SEQUENTIAL))) {
			return false;
		}
		const OrtDmlApi* dmlApi = nullptr;
		if (_Check(_api->GetExecutionProviderApi(
			"DML", ORT_API_VERSION,
			reinterpret_cast<const void**>(&dmlApi))) && dmlApi) {
			winrt::com_ptr<IDXGIFactory1> factory;
			winrt::com_ptr<IDXGIAdapter1> adapter;
			const std::filesystem::path directMLPath =
				config.runtimeDirectory / L"DirectML.dll";
			_directMLModule = LoadLibraryExW(
				directMLPath.c_str(), nullptr, LOAD_WITH_ALTERED_SEARCH_PATH);
			using DmlCreateDeviceFn = HRESULT(WINAPI*)(
				ID3D12Device*, DML_CREATE_DEVICE_FLAGS, REFIID, void**);
			auto createDmlDevice = _directMLModule ?
				reinterpret_cast<DmlCreateDeviceFn>(
					GetProcAddress(_directMLModule, "DMLCreateDevice")) : nullptr;
			if (createDmlDevice &&
				SUCCEEDED(CreateDXGIFactory1(IID_PPV_ARGS(factory.put()))) &&
				SUCCEEDED(factory->EnumAdapters1(config.adapterIndex, adapter.put())) &&
				SUCCEEDED(D3D12CreateDevice(
					adapter.get(), D3D_FEATURE_LEVEL_11_0,
					IID_PPV_ARGS(_dmlDevice12.put())))) {
				D3D12_COMMAND_QUEUE_DESC queueDesc{
					.Type = D3D12_COMMAND_LIST_TYPE_COMPUTE,
					.Priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL
				};
				_dmlDevice12->CreateCommandQueue(
					&queueDesc, IID_PPV_ARGS(_dmlQueue12.put()));
				if (_dmlQueue12 && SUCCEEDED(createDmlDevice(
					_dmlDevice12.get(), DML_CREATE_DEVICE_FLAG_NONE,
					IID_PPV_ARGS(_dmlDevice.put()))) &&
					_Check(dmlApi->SessionOptionsAppendExecutionProvider_DML1(
						_sessionOptions, _dmlDevice.get(), _dmlQueue12.get()))) {
					Logger::Get().Info(
						"DirectML uses an application-owned compute queue");
					return true;
				}
			}
		}
		_dmlDevice = nullptr;
		_dmlQueue12 = nullptr;
		_dmlDevice12 = nullptr;
		if (_directMLModule) {
			FreeLibrary(_directMLModule);
			_directMLModule = nullptr;
		}
		auto append = reinterpret_cast<AppendDmlFn>(
			GetProcAddress(_module, "OrtSessionOptionsAppendExecutionProvider_DML"));
		return append && _Check(append(
			_sessionOptions, static_cast<int>(config.adapterIndex)));
	}

	bool _Check(OrtStatus* status) noexcept {
		if (!status) return true;
		if (_api) {
			Logger::Get().Warn(fmt::format(
				"{}: {}", Name(), _api->GetErrorMessage(status)));
			_api->ReleaseStatus(status);
		}
		return false;
	}

	void _Release() noexcept {
		if (_api) {
			if (_outputValue) _api->ReleaseValue(_outputValue);
			if (_inputValue) _api->ReleaseValue(_inputValue);
			if (_memoryInfo) _api->ReleaseMemoryInfo(_memoryInfo);
			if (_session) _api->ReleaseSession(_session);
			if (_sessionOptions) _api->ReleaseSessionOptions(_sessionOptions);
			if (_env) _api->ReleaseEnv(_env);
		}
		_outputValue = nullptr;
		_inputValue = nullptr;
		_memoryInfo = nullptr;
		_session = nullptr;
		_sessionOptions = nullptr;
		_env = nullptr;
		_api = nullptr;
		_inputBuffer.clear();
		_outputBuffer.clear();
		_inputWidth = 0;
		_inputHeight = 0;
		_dmlDevice = nullptr;
		_dmlQueue12 = nullptr;
		_dmlDevice12 = nullptr;
		if (_directMLModule) FreeLibrary(_directMLModule);
		_directMLModule = nullptr;
		if (_module) FreeLibrary(_module);
		_module = nullptr;
	}

	bool _tensorRT = false;
	HMODULE _module = nullptr;
	const OrtApi* _api = nullptr;
	OrtEnv* _env = nullptr;
	OrtSessionOptions* _sessionOptions = nullptr;
	OrtSession* _session = nullptr;
	OrtMemoryInfo* _memoryInfo = nullptr;
	OrtValue* _inputValue = nullptr;
	OrtValue* _outputValue = nullptr;
	std::vector<float> _inputBuffer;
	std::vector<float> _outputBuffer;
	uint32_t _inputWidth = 0;
	uint32_t _inputHeight = 0;
	HMODULE _directMLModule = nullptr;
	winrt::com_ptr<ID3D12Device> _dmlDevice12;
	winrt::com_ptr<ID3D12CommandQueue> _dmlQueue12;
	winrt::com_ptr<IDMLDevice> _dmlDevice;
};

}

std::unique_ptr<IDepthInferenceBackend> CreateTensorRTDepthBackend() {
	return std::make_unique<OnnxRuntimeDepthBackend>(true);
}

std::unique_ptr<IDepthInferenceBackend> CreateDirectMLDepthBackend() {
	return std::make_unique<OnnxRuntimeDepthBackend>(false);
}

struct SharedTensorRTDepthBackend::Impl {
	explicit Impl(DepthInferenceConfig value) : config(std::move(value)) {}

	DepthInferenceConfig config;
	std::unique_ptr<IDepthInferenceBackend> backend;
	std::mutex runMutex;
	std::atomic<SharedDepthBackendState> state{
		SharedDepthBackendState::Initializing
	};
};

SharedTensorRTDepthBackend::SharedTensorRTDepthBackend(
	DepthInferenceConfig config
) : _impl(std::make_unique<Impl>(std::move(config))) {}

SharedTensorRTDepthBackend::~SharedTensorRTDepthBackend() = default;

SharedDepthBackendState SharedTensorRTDepthBackend::State() const noexcept {
	return _impl->state.load(std::memory_order_acquire);
}

std::string_view SharedTensorRTDepthBackend::Name() const noexcept {
	return _impl->backend ? _impl->backend->Name() : "TensorRT FP16";
}

bool SharedTensorRTDepthBackend::Run(
	const DepthInferenceInput& input,
	DepthInferenceOutput& output
) noexcept {
	if (State() != SharedDepthBackendState::Ready) return false;
	std::lock_guard lock(_impl->runMutex);
	if (State() != SharedDepthBackendState::Ready || !_impl->backend) return false;
	const bool succeeded = _impl->backend->Run(input, output);
	if (!succeeded) {
		_impl->state.store(
			SharedDepthBackendState::Failed, std::memory_order_release);
		Logger::Get().Warn(
			"Frame Guidance shared TensorRT backend entered failed state");
	}
	return succeeded;
}

void SharedTensorRTDepthBackend::Initialize() noexcept {
	const auto totalStart = std::chrono::steady_clock::now();
	std::unique_ptr<IDepthInferenceBackend> backend =
		CreateTensorRTDepthBackend();
	bool initialized = backend && backend->Initialize(_impl->config);
	if (initialized) {
		std::vector<float> warmup(
			size_t(_impl->config.inputWidth) * _impl->config.inputHeight * 3,
			0.0f);
		DepthInferenceOutput ignored;
		const auto start = std::chrono::steady_clock::now();
		initialized = backend->Run({
			.values = warmup.data(),
			.valueCount = warmup.size(),
			.width = _impl->config.inputWidth,
			.height = _impl->config.inputHeight
		}, ignored);
		Logger::Get().Info(fmt::format(
			"Frame Guidance shared TensorRT warmup result={} time={:.1f} ms",
			initialized,
			std::chrono::duration<double, std::milli>(
				std::chrono::steady_clock::now() - start).count()));
	}
	if (initialized) _impl->backend = std::move(backend);
	_impl->state.store(
		initialized ? SharedDepthBackendState::Ready :
			SharedDepthBackendState::Failed,
		std::memory_order_release);
	Logger::Get().Info(fmt::format(
		"Frame Guidance shared TensorRT initialization completed state={} "
		"input={}x{} total={:.1f} ms",
		initialized ? "ready" : "failed", _impl->config.inputWidth,
		_impl->config.inputHeight,
		std::chrono::duration<double, std::milli>(
			std::chrono::steady_clock::now() - totalStart).count()));
}

namespace {

struct SharedTensorRTRegistry {
	std::mutex mutex;
	std::unordered_map<
		std::wstring,
		std::shared_ptr<SharedTensorRTDepthBackend>
	> entries;
};

SharedTensorRTRegistry& GetSharedTensorRTRegistry() noexcept {
	// TensorRT sessions intentionally live until process termination. Leaking the
	// registry also prevents static destruction from waiting for CreateSession.
	static SharedTensorRTRegistry* registry = new SharedTensorRTRegistry();
	return *registry;
}

std::mutex& GetTensorRTInitializationMutex() noexcept {
	// Never build two TensorRT engines concurrently. This mutex has the same
	// process lifetime as the registry and is safe for detached initializers.
	static std::mutex* mutex = new std::mutex();
	return *mutex;
}

std::wstring MakeSharedTensorRTKey(const DepthInferenceConfig& config) {
	return fmt::format(
		L"{}\n{}\n{}\n{}\n{}x{}\n{}",
		config.modelPath.native(), config.runtimeDirectory.native(),
		config.engineCacheDirectory.native(), config.adapterIndex,
		config.inputWidth, config.inputHeight, config.isNvidiaAdapter);
}

const char* SharedStateName(SharedDepthBackendState state) noexcept {
	switch (state) {
	case SharedDepthBackendState::Initializing: return "initializing";
	case SharedDepthBackendState::Ready: return "ready";
	case SharedDepthBackendState::Failed: return "failed";
	default: return "unknown";
	}
}

}

std::shared_ptr<SharedTensorRTDepthBackend>
AcquireSharedTensorRTDepthBackend(const DepthInferenceConfig& config) {
	SharedTensorRTRegistry& registry = GetSharedTensorRTRegistry();
	const std::wstring key = MakeSharedTensorRTKey(config);
	std::lock_guard lock(registry.mutex);
	if (const auto it = registry.entries.find(key); it != registry.entries.end()) {
		Logger::Get().Info(fmt::format(
			"Frame Guidance reusing shared TensorRT backend input={}x{} state={}",
			config.inputWidth, config.inputHeight,
			SharedStateName(it->second->State())));
		return it->second;
	}

	auto backend = std::shared_ptr<SharedTensorRTDepthBackend>(
		new SharedTensorRTDepthBackend(config));
	registry.entries.emplace(key, backend);
	Logger::Get().Info(fmt::format(
		"Frame Guidance creating shared TensorRT backend input={}x{}",
		config.inputWidth, config.inputHeight));
	try {
		std::thread([backend] {
			const HRESULT apartment = CoInitializeEx(nullptr, COINIT_MULTITHREADED);
			{
				std::lock_guard initializationLock(
					GetTensorRTInitializationMutex());
				backend->Initialize();
			}
			if (SUCCEEDED(apartment)) CoUninitialize();
		}).detach();
	} catch (...) {
		backend->_impl->state.store(
			SharedDepthBackendState::Failed, std::memory_order_release);
		Logger::Get().Error(
			"Create shared Frame Guidance TensorRT initializer failed");
	}
	return backend;
}

}

#else

namespace Magpie {

std::unique_ptr<IDepthInferenceBackend> CreateTensorRTDepthBackend() { return {}; }
std::unique_ptr<IDepthInferenceBackend> CreateDirectMLDepthBackend() { return {}; }
std::shared_ptr<SharedTensorRTDepthBackend>
AcquireSharedTensorRTDepthBackend(const DepthInferenceConfig&) { return {}; }

}

#endif
