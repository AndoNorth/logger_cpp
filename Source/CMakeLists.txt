cmake_minimum_required(VERSION 3.3)

# Use solution folders.
set_property(GLOBAL PROPERTY USE_FOLDERS ON)

include_directories(${CMAKE_CURRENT_SOURCE_DIR})

# set (CMAKE_FIND_PACKAGE_PREFER_CONFIG ON)


# Shared Libraries
set (CURRENT_SOLUTION_FOLDER "Shared Libraries")
add_subdirectory ("SharedLibraries/CommonTools")
add_subdirectory ("SharedLibraries/libgrib-c")
add_subdirectory ("SharedLibraries/libgrib-cpp")
add_subdirectory ("SharedLibraries/VisionTools")
if (WIN32)
   add_subdirectory ("SharedLibraries/MediaTools")
endif()
add_subdirectory ("SharedLibraries/Grib")
add_subdirectory ("SharedLibraries/Plotting")
add_subdirectory ("SharedLibraries/Satellite")
add_subdirectory ("SharedLibraries/FlightFolder")
add_subdirectory ("SharedLibraries/CommObjects")
if (WIN32)
   add_subdirectory ("SharedLibraries/MediaObjects")
endif ()

# Vision
if (WIN32)
   set (CURRENT_SOLUTION_FOLDER "Vision")
   add_subdirectory ("Vision/Profile")
   add_subdirectory ("Vision/Meteogram")
   add_subdirectory ("Vision/MessirVision")
   set (CURRENT_SOLUTION_FOLDER "Shared Libraries")
   add_subdirectory ("SharedLibraries/VisionObjects")
endif ()

# Commm
set (CURRENT_SOLUTION_FOLDER "Comm")
add_subdirectory ("Comm/MessirComm")
if (WIN32)
   add_subdirectory ("Comm/CommMenu")
   # add_subdirectory ("Comm/CommProber")
   add_subdirectory ("Comm/MessirLicense")
   add_subdirectory ("Comm/ConfigExplorer")
   add_subdirectory ("Comm/IngestSrv")

   # add_subdirectory ("Comm/ProcessingSrv")
   # add_subdirectory ("Comm/CommSwitcher")

   add_subdirectory ("Comm/MeteoSurvey")
   add_subdirectory ("Comm/MsgConsole")
   add_subdirectory ("Comm/ProdSrv")
   add_subdirectory ("Comm/SnmpAgent")
   add_subdirectory ("Comm/TestComm")
endif ()



# ImageMaker
set (CURRENT_SOLUTION_FOLDER "ImageMaker")
if (WIN32)
   add_subdirectory ("ImageMaker/ImageMakerTools")
   add_subdirectory ("ImageMaker/ImageMakerService")
   add_subdirectory ("ImageMaker/ImageMakerUI")
endif ()


# Clim
set (CURRENT_SOLUTION_FOLDER "Clim")
# add_subdirectory ("Clim/ClimAuth")
# add_subdirectory ("Clim/ClimImport")
# add_subdirectory ("Clim/ClimClient)
# add_subdirectory ("Clim/ClimServer")
# add_subdirectory ("Clim/ClimConverter")

# add_subdirectory ("Clim/ClimCommad")
# add_subdirectory ("Clim/ClimDevel")
# add_subdirectory ("Clim/ClimTasks")

# Tools
set (CURRENT_SOLUTION_FOLDER "Tools")
add_subdirectory ("Tools/Comp_format")
add_subdirectory ("Tools/GlobalLogger")
add_subdirectory ("Tools/ODBRecover")

