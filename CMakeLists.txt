cmake_minimum_required(VERSION 3.3)

list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/Build/cmake)

set(CMAKE_INSTALL_LIBDIR lib)
set(CMAKE_INSTALL_BINDIR bin)
set(CMAKE_INSTALL_INCLUDEDIR includes)

set (MSS_DEPENDENCIES_DIR ${CMAKE_SOURCE_DIR}/../messir-mss-dependencies_install )
cmake_path(NORMAL_PATH MSS_DEPENDENCIES_DIR OUTPUT_VARIABLE MSS_DEPENDENCIES_DIR)
set (MSS_DEPENDENCIES_DIR ${MSS_DEPENDENCIES_DIR} CACHE PATH "Installation folder of mss-dependencies" FORCE)

if (NOT WIN32)
   set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -static-libstdc++ -static-libgcc")
   set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -static-libstdc++ -static-libgcc")
endif (NOT WIN32)

set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -D_DEBUG")
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -D_DEBUG")

set(CMAKE_CONFIGURATION_TYPES "Debug;RelWithDebInfo" CACHE STRING "")

# Set this to "on" to have a verbose build
set(CMAKE_VERBOSE_MAKEFILE off)


MATH(EXPR stack_size "16 * 1024 * 1024") # 16 Mb
set(MSS_STACK_SIZE ${stack_size} CACHE STRING "")

project(messir-mss)

   add_subdirectory("Source")
   add_subdirectory("Documentation")
	add_subdirectory("Tests")


if(CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
   set (CMAKE_INSTALL_PREFIX ${CMAKE_SOURCE_DIR}/../messir-mss_install)
   cmake_path(NORMAL_PATH CMAKE_INSTALL_PREFIX OUTPUT_VARIABLE CMAKE_INSTALL_PREFIX)
   set (CMAKE_INSTALL_PREFIX ${CMAKE_INSTALL_PREFIX} CACHE PATH "" FORCE)
endif()
   