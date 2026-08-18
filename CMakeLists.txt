cmake_minimum_required(VERSION 3.14)

project(logger_cpp LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

enable_testing()

if (NOT WIN32)
   set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -static-libstdc++ -static-libgcc")
endif ()

include(FetchContent)

FetchContent_Declare(
   googletest
   GIT_REPOSITORY https://github.com/google/googletest.git
   GIT_TAG v1.15.2
)
set(INSTALL_GTEST OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

FetchContent_Declare(
   benchmark
   GIT_REPOSITORY https://github.com/google/benchmark.git
   GIT_TAG v1.9.1
)
set(BENCHMARK_ENABLE_TESTING OFF CACHE BOOL "" FORCE)
set(BENCHMARK_ENABLE_GTEST_TESTS OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(benchmark)

FetchContent_Declare(
   nlohmann_json
   GIT_REPOSITORY https://github.com/nlohmann/json.git
   GIT_TAG v3.11.3
)
FetchContent_MakeAvailable(nlohmann_json)

set(JSON_VALIDATOR_INSTALL OFF CACHE BOOL "" FORCE)
set(BUILD_TESTS OFF CACHE BOOL "" FORCE)
FetchContent_Declare(
   nlohmann_json_schema_validator
   GIT_REPOSITORY https://github.com/pboettch/json-schema-validator.git
   GIT_TAG 2.3.0
)
FetchContent_MakeAvailable(nlohmann_json_schema_validator)

add_subdirectory("Source")
add_subdirectory("Tests")
