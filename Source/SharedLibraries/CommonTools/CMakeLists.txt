add_library(CommonTools STATIC
   Logger.cpp
   LogTargetFile.cpp
   LogTargetStandardOutput.cpp
   LogTargetStandardError.cpp
   ScheduleUtils.cpp
   ProcessName.cpp
)

target_include_directories(CommonTools PUBLIC
   include
   ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(CommonTools PUBLIC
   nlohmann_json::nlohmann_json
   nlohmann_json_schema_validator
)

# Every messir-mss CMake target defines this per its own project name
# (see Build/cmake/CoroborExeTemplate.cmake); Logger.h's log macros expect it.
target_compile_definitions(CommonTools PRIVATE MSS_MODULE_NAME="commontools")
