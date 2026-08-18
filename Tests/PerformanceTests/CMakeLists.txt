add_executable(PerformanceTests
   PerformanceTests.cpp
   Logger_tests.cpp
)

target_link_libraries(PerformanceTests PRIVATE
   CommonTools
   benchmark::benchmark
)

target_compile_definitions(PerformanceTests PRIVATE MSS_MODULE_NAME="performancetests")
