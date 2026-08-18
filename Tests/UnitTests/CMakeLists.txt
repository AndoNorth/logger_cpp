cmake_minimum_required(VERSION 3.3)

project(UnitTests LANGUAGES CXX)

   # specify the C++ standard
   set(CMAKE_CXX_STANDARD 20)
   set(CMAKE_CXX_STANDARD_REQUIRED ON)

   # ${PROJECT_NAME}_INCLUDE_DIRS
   list (APPEND ${PROJECT_NAME}_INCLUDE_DIRS
      .
		../
   )
   
   # ${PROJECT_NAME}_PUBLIC_HEADERS
   # file (GLOB_RECURSE ${PROJECT_NAME}_PUBLIC_HEADERS "*.h")

   # ${PROJECT_NAME}_PRIVATE_HEADERS
   # file (GLOB_RECURSE ${PROJECT_NAME}_PRIVATE_HEADERS "*.h")

   # ${PROJECT_NAME}_SOURCES
   file (GLOB_RECURSE ${PROJECT_NAME}_SOURCES
      "*.cpp"
   )

   # ${PROJECT_NAME}_LINKS
   list (APPEND ${PROJECT_NAME}_LINKS 
      CommObjects CommonTools
      gtest gtest_main gmock gmock_main
	)

   # ${PROJECT_NAME}_DEPENDENCIES
   list (APPEND ${PROJECT_NAME}_DEPENDENCIES
      CommObjects CommonTools
      gtest gtest_main gmock gmock_main
	)
   
   # ${PROJECT_NAME}_PRIVATE_DEFINITIONS
   if (WIN32)
      set(CMAKE_MFC_FLAG 2)
      list (APPEND ${PROJECT_NAME}_PRIVATE_DEFINITIONS _USE_32BIT_TIME_T)
   endif ()

   include(CoroborExeTemplate)