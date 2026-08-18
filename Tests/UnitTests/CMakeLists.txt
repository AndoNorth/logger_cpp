add_executable(UnitTests
   UnitTests.cpp
   Logger_tests.cpp
)

target_link_libraries(UnitTests PRIVATE
   CommonTools
   gtest gtest_main
   gmock gmock_main
)

target_compile_definitions(UnitTests PRIVATE MSS_MODULE_NAME="unittests")

include(GoogleTest)
gtest_discover_tests(UnitTests)
