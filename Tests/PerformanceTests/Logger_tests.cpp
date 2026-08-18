#ifdef _WINDOWS
#include "stdafx.h"
#endif

#include <benchmark/benchmark.h>

#include <cmath>
#include <filesystem>

#include <Logger.h>
#include <LogTargetFile.h>

/**
 * custom range from 10^start to 10^end, to avoid computation in test.
 */
std::function<void(benchmark::internal::Benchmark*)> Custom_range_power_of_ten(int start, int end) {
	return [start, end](benchmark::internal::Benchmark* b) {
		for (int i = start; i <= end; ++i) {
			b->Args({ static_cast<int>(std::pow(10, i)) });
		}
	};
}

/**
 * helper method used to suppress output to stdout
 *
 * @param predicate Method where we want to suppress stdout
 *
 * @return suppressed_buffer_string the suppressed buffer stream as a string
*/
std::string Capture_stdout(std::function<void()> decorated_function) {
	std::stringbuf suppressed_buffer;
	std::streambuf* old_buffer = std::cout.rdbuf(&suppressed_buffer);

	decorated_function();

	std::cout.rdbuf(old_buffer);  // Restore the original stream buffer
	return suppressed_buffer.str();
}

/**
 * referenced from trace.cpp in linux build for trace.Format().
 */
std::string Linux_trace_format(const char* format, ...)
{
	va_list arguments;
	va_start(arguments, format);
	char trace_text[800];
	int  text_length = vsnprintf(trace_text, sizeof(trace_text) - 1, format, arguments);
	va_end(arguments);
	return std::string(trace_text);
}

/**
 * referenced from trace.h in windows build for trace.Format().
 */
std::string Windows_trace_format(const char* format, ...) {
	va_list g;
	va_start(g, format);
	CString buf;
	buf.FormatV(format, g);
	return (LPCSTR)buf;
}

static void BM_Linux_trace_format_int(benchmark::State& state) {

	int test_int1 = 5;
	int test_int2 = 10;

	for (auto _ : state) {
		for (int i = 0; i < state.range(0); i++) {
			std::string out = Linux_trace_format("int1=%d, int2=%d", test_int1, test_int2);
		}
	}
}

static void BM_Linux_trace_format_str(benchmark::State& state) {

	std::string test_str = "hello, world";

	for (auto _ : state) {
		for (int i = 0; i < state.range(0); i++) {
			std::string out = Linux_trace_format("str=%s", test_str);
		}
	}
}

static void BM_Linux_trace_format_mixed(benchmark::State& state) {

	std::string test_str = "hello, world";
	int test_int1 = 5;
	int test_int2 = 10;

	for (auto _ : state) {
		for (int i = 0; i < state.range(0); i++) {
			std::string out = Linux_trace_format("str=%s, int1=%d, int2=%d", test_str, test_int1, test_int2);
		}
	}
}

static void BM_Windows_trace_format_int(benchmark::State& state) {

	int test_int1 = 5;
	int test_int2 = 10;

	for (auto _ : state) {
		for (int i = 0; i < state.range(0); i++) {
			std::string out = Windows_trace_format("int1=%d, int2=%d", test_int1, test_int2);
		}
	}
}

static void BM_Windows_trace_format_str(benchmark::State& state) {

	std::string test_str = "hello, world";

	for (auto _ : state) {
		for (int i = 0; i < state.range(0); i++) {
			std::string out = Windows_trace_format("str=%s", test_str);
		}
	}
}

static void BM_Windows_trace_format_mixed(benchmark::State& state) {

	std::string test_str = "hello, world";
	int test_int1 = 5;
	int test_int2 = 10;

	for (auto _ : state) {
		for (int i = 0; i < state.range(0); i++) {
			std::string out = Windows_trace_format("str=%s, int1=%d, int2=%d", test_str, test_int1, test_int2);
		}
	}
}

// helper methods
void Logger_Write_log(benchmark::State& state, MessirLogger::Logger* logger) {

	// while(state.keeprunning())
	for (auto _ : state) {
		for (int i = 0; i < state.range(0); i++) {
			MessirLogger::LogRecord test_record = MessirLogger::LogRecord(
				MessirLogger::LogLevel::LEVEL_INFO, MessirLogger::LogKindSet().All_set(),
				MSS_MODULE_NAME, std::source_location::current(), "",
				"log message " + std::to_string(i));
			logger->Log_entry(test_record);
			benchmark::ClobberMemory();
		}
	}
}


// tests
class TargetStandardOutputSync : public benchmark::Fixture {
public:
	MessirLogger::Logger* _logger;

public:
	void SetUp(::benchmark::State& state) {
		_logger = new MessirLogger::Logger;
		MessirLogger::LoggerConfig default_config(
			{
				std::make_shared<MessirLogger::TargetConfig>("StdOut",
					"[%%level:%%kinds] - %%log", MessirLogger::TargetType::SYSTEM_OUT_TARGET),
			},
			{
				{MessirLogger::LogLevel::LEVEL_INFO, MessirLogger::LogKindSet().All_set(), {"StdOut"}},
			},
			false,
			false
		);
		_logger->Configure(default_config);
		_logger->Start();
	}

	void TearDown(::benchmark::State& state) {
		delete _logger;
	}
};

BENCHMARK_DEFINE_F(TargetStandardOutputSync, BM_Write_log)(benchmark::State& state) {
	std::string capturedOutput = Capture_stdout([&]() {
		Logger_Write_log(state, _logger);
	});
	state.SetLabel("Logs per iteration = " + std::to_string(state.range(0)));
}

class TargetStandardOutputAsync : public benchmark::Fixture {
public:
	MessirLogger::Logger* _logger;

public:
	void SetUp(::benchmark::State& state) {
		_logger = new MessirLogger::Logger;
		MessirLogger::LoggerConfig default_config(
			{
				std::make_shared<MessirLogger::TargetConfig>("StdOut",
					"[%%level:%%kinds] - %%log", MessirLogger::TargetType::SYSTEM_OUT_TARGET),
			},
			{
				{MessirLogger::LogLevel::LEVEL_INFO, MessirLogger::LogKindSet().All_set(), {"StdOut"}},
			},
			false,
			true
		);
		_logger->Configure(default_config);
		_logger->Start();
	}

	void TearDown(::benchmark::State& state) {
		delete _logger;
	}
};

BENCHMARK_DEFINE_F(TargetStandardOutputAsync, BM_Write_log)(benchmark::State& state) {
	std::string capturedOutput = Capture_stdout([&]() {
		Logger_Write_log(state, _logger);
		_logger->Stop(); // flushes the remaining log records
	});
	state.SetLabel("Logs per iteration = " + std::to_string(state.range(0)));
}

class FileTargetSync : public benchmark::Fixture {
public:
	MessirLogger::Logger* _logger;
	std::string expected_filename = "performance_test.log";

public:
	void SetUp(::benchmark::State& state) {
		_logger = new MessirLogger::Logger;
		MessirLogger::LoggerConfig default_config(
			{
				std::make_shared<MessirLogger::FileTargetConfig>("FileTarget",
					"[%%level:%%kinds] - %%log", "./", "performance_test",
					0, 0, "", "", "%%path%%filename%%suffix"),
			},
			{
				{MessirLogger::LogLevel::LEVEL_INFO, MessirLogger::LogKindSet().All_set(), {"FileTarget"}},
			},
			false,
			false
		);
		_logger->Configure(default_config);
		_logger->Start();
	}

	void TearDown(::benchmark::State& state) {
		delete _logger;
		std::filesystem::remove(expected_filename);
	}
};

BENCHMARK_DEFINE_F(FileTargetSync, BM_Write_log)(benchmark::State& state) {
	Logger_Write_log(state, _logger);
	state.SetLabel("Logs per iteration = "+ std::to_string(state.range(0))
		+ " , filesize=" + std::to_string(std::filesystem::file_size(expected_filename)));
}

class FileTargetAsync : public benchmark::Fixture {
public:
	MessirLogger::Logger* _logger;
	std::string expected_filename = "performance_test.log";

public:
	void SetUp(::benchmark::State& state) {
		_logger = new MessirLogger::Logger;
		MessirLogger::LoggerConfig default_config(
			{
				std::make_shared<MessirLogger::FileTargetConfig>("FileTarget",
					"[%%level:%%kinds] - %%log", "./", "performance_test",
					0, 0, "", "", "%%path%%filename%%suffix"),
			},
			{
				{MessirLogger::LogLevel::LEVEL_INFO, MessirLogger::LogKindSet().All_set(), {"FileTarget"}},
			},
			false,
			false
		);
		_logger->Configure(default_config);
		_logger->Start();
	}

	void TearDown(::benchmark::State& state) {
		delete _logger;
		std::filesystem::remove(expected_filename);
	}
};

BENCHMARK_DEFINE_F(FileTargetAsync, BM_Write_log)(benchmark::State& state) {
	Logger_Write_log(state, _logger);
	_logger->Stop(); // flushes the remaining log records
	state.SetLabel("Logs per iteration = " + std::to_string(state.range(0))
		+ " , filesize=" + std::to_string(std::filesystem::file_size(expected_filename)));
}

// tests
BENCHMARK_REGISTER_F(TargetStandardOutputSync, BM_Write_log)
->Apply([](benchmark::internal::Benchmark* b) {
	Custom_range_power_of_ten(2, 5)(b);
});

BENCHMARK_REGISTER_F(TargetStandardOutputAsync, BM_Write_log)
->Apply([](benchmark::internal::Benchmark* b) {
	Custom_range_power_of_ten(2, 5)(b);
});

BENCHMARK_REGISTER_F(FileTargetSync, BM_Write_log)
->Apply([](benchmark::internal::Benchmark* b) {
	Custom_range_power_of_ten(2, 5)(b);
});

BENCHMARK_REGISTER_F(FileTargetAsync, BM_Write_log)
->Apply([](benchmark::internal::Benchmark* b) {
	Custom_range_power_of_ten(2, 5)(b);
});

BENCHMARK(BM_Linux_trace_format_int)
->Apply([](benchmark::internal::Benchmark* b) {
	Custom_range_power_of_ten(2, 5)(b);
});

BENCHMARK(BM_Linux_trace_format_str)
->Apply([](benchmark::internal::Benchmark* b) {
	Custom_range_power_of_ten(2, 5)(b);
});

BENCHMARK(BM_Linux_trace_format_mixed)
->Apply([](benchmark::internal::Benchmark* b) {
	Custom_range_power_of_ten(2, 5)(b);
});

BENCHMARK(BM_Windows_trace_format_int)
->Apply([](benchmark::internal::Benchmark* b) {
	Custom_range_power_of_ten(2, 5)(b);
});

BENCHMARK(BM_Windows_trace_format_str)
->Apply([](benchmark::internal::Benchmark* b) {
	Custom_range_power_of_ten(2, 5)(b);
});

BENCHMARK(BM_Windows_trace_format_mixed)
->Apply([](benchmark::internal::Benchmark* b) {
	Custom_range_power_of_ten(2, 5)(b);
});
