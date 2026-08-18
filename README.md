# logger_cpp

A standalone C++20 structured logger: async dispatch to multiple targets
(stdout, stderr, file), JSON-configurable targets/levels via
`nlohmann/json-schema`, and file rollover with age-based cleanup.

Extracted from a larger internal project (messir-mss) with its full commit
history; a few small support files (`ScheduleUtils`, `StringHelpers`) came
along with it since the logger genuinely depends on them.

## Requirements

- CMake 3.14+
- A C++20 compiler (tested with GCC 13; needs working `<source_location>` and `<semaphore>`)
- git and network access at first configure - dependencies are fetched, not vendored (see below)

## Build

```shell
cmake -B build
cmake --build build -j
```

Dependencies (googletest, google benchmark, nlohmann_json, json-schema-validator)
are fetched automatically via CMake `FetchContent` - no separate install step,
but the first `cmake -B build` needs network access to clone them and will
take noticeably longer than subsequent ones.

## Run tests

```shell
ctest --test-dir build --output-on-failure   # both suites, gtest-discovered
```

or run the binaries directly:

```shell
./build/Tests/UnitTests/UnitTests          # functional/behavioural coverage
./build/Tests/PerformanceTests/PerformanceTests  # throughput/latency benchmarks
```

## Usage

Configure a logger with one or more targets and a dispatch table (which
levels/kinds go to which targets), then start it:

```cpp
MessirLogger::Logger logger;

MessirLogger::LoggerConfig config(
    { std::make_shared<MessirLogger::TargetConfig>(
          "StdOut", "[%%level:%%kinds] - %%log", MessirLogger::TargetType::SYSTEM_OUT_TARGET) },
    { { MessirLogger::LogLevel::LEVEL_INFO, MessirLogger::LogKindSet().All_set(), {"StdOut"} } },
    /* asynchronous_mode */ false,
    /* use_fallback */ false
);

logger.Configure(config);
logger.Start();
```

Full worked examples - multiple targets, file rollover config, loading
config from JSON (`Load_config`) - are in `Tests/UnitTests/Logger_tests.cpp`
(`LoggerConfigurationUTests`) and `Tests/PerformanceTests/Logger_tests.cpp`.

Once configured, log through the `MSS_*` macros rather than calling
`Write_log` directly - they capture the call site (`std::source_location`)
for you:

```cpp
MSS_DEBUG(MessirLogger::LogKind::KIND_TECHNICAL, "my_module") << "starting up";
MSS_ERROR(MessirLogger::LogKind::KIND_ACTION, "my_module") << "failed: " << error;
```

See the doc comment above the macro block in `Source/SharedLibraries/CommonTools/include/Logger.h`
for the full set (`MSS_DEBUG` through `MSS_FATAL`, plus `_EXTRA` variants)
and what each parameter means - or write your own on the same pattern if
these don't fit (they're a thin wrapper around `log_line`).

### Adding a target

Built-in targets are stdout, stderr, and file (`MessirLogger::TargetType::SYSTEM_OUT_TARGET`
/ `SYSTEM_ERR_TARGET` / `FILE_TARGET`). To ship logs somewhere else - an
OpenTelemetry collector, an AMQP queue, whatever - subclass
`MessirLogger::Target` (`Source/SharedLibraries/CommonTools/include/Logger.h`)
and implement `Setup`, `Maintenance`, `Refresh`, and `Try_write_log`.
`TestTarget` in `Tests/UnitTests/Logger_tests.cpp` is a minimal example.
To make it configurable through `LoggerConfig` like the built-ins, add a
`TargetType` enum value and a case in `New_log_target` (`Logger.cpp`).

## Repository structure

- `Source/SharedLibraries/CommonTools/` - the logger itself (`Logger.*`,
  `LogTarget*.*`), plus the small pieces of its dependency it genuinely
  needs (`ScheduleUtils` for rollover timing, `StringHelpers` for ANSI
  formatting, `ProcessName` for the `%%pid`/`%%procname` placeholders).
- `Source/SharedLibraries/CommonTools/include/WinEmul.h` - a minimal stand-in
  for a much larger Windows-emulation header from the logger's original
  codebase, just enough (`CString`, a couple of registry-lookup stubs) for
  `SerializerJSON.h` to compile without pulling in Qt/curl.
- `Tests/UnitTests/` - gtest/gmock functional and behavioural tests.
- `Tests/PerformanceTests/` - Google Benchmark performance tests.
