cmake_minimum_required(VERSION 3.3)

# Use solution folders.
set_property(GLOBAL PROPERTY USE_FOLDERS ON)

include_directories(../Source) # required for configure.h in Logger.h, for targetver.h
# set (CMAKE_FIND_PACKAGE_PREFER_CONFIG ON)

set (CURRENT_SOLUTION_FOLDER "Tests")
add_subdirectory ("UnitTests")
add_subdirectory ("PerformanceTests")
