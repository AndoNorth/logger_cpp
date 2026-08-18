# logger_cpp

A C++20 structured logger, extracted from messir-mss with its full commit history:
async dispatch to multiple targets (stdout, stderr, file), JSON-configurable
targets/levels via `nlohmann/json-schema`, and file rollover with age-based cleanup.

## Build

```shell
cmake -B build
cmake --build build -j
```

Dependencies (googletest, google benchmark, nlohmann_json, json-schema-validator)
are fetched automatically via CMake `FetchContent` - no separate install step.

## Run tests

```shell
./build/Tests/UnitTests/UnitTests
./build/Tests/PerformanceTests/PerformanceTests
```

`Tests/e2e/test_config_logger.py` is a pytest suite for the `/config/logger`
REST endpoints exposed by the full messir-mss server. It needs that server
running and isn't runnable against this standalone repo; kept for history
and as a reference for the config wire format.

## Repository structure

- `Source/SharedLibraries/CommonTools/` - the logger itself (`Logger.*`,
  `LogTarget*.*`), plus the small pieces of messir-mss's `CommonTools` it
  genuinely depends on (`ScheduleUtils` for rollover timing, `StringHelpers`
  for ANSI formatting, `ProcessName` for the `%%pid`/`%%procname` placeholders).
- `Source/SharedLibraries/CommonTools/include/WinEmul.h` - a minimal stand-in
  for messir-mss's real Windows-emulation header, just enough (`CString`,
  a couple of registry-lookup stubs) for `SerializerJSON.h` to compile
  without pulling in Qt/curl.
- `Tests/UnitTests/`, `Tests/PerformanceTests/` - gtest/gmock unit tests and
  Google Benchmark performance tests.
- `Tests/e2e/` - pytest e2e tests against the full messir-mss server (see above).
