#include "stdafx.h"

#include <benchmark/benchmark.h>

// PerformanceTests.exe --benchmark_out=results.csv --benchmark_out_format=csv
// PerformanceTests.exe --benchmark_filter=".*trace_format.*"

// BENCHMARK_MAIN();
int main(int argc, char** argv) {
	char arg0_default[] = "benchmark"; char* args_default = arg0_default; if (!argv) {
		argc = 1; argv = &args_default;
	} ::benchmark::Initialize(&argc, argv); if (::benchmark::ReportUnrecognizedArguments(argc, argv)) return 1; ::benchmark::RunSpecifiedBenchmarks(); ::benchmark::Shutdown(); return 0;
} int main(int, char**);
