# Messir MSS


- [Messir MSS](#messir-mss)
    - [Build](#build)
        - [Using CMake (recommended)](#using-cmake-recommended)
        - [Using Visual Studio (Windows only)](#using-visual-studio-windows-only)
        - [Docker build](#docker-build)
    - [Repository structure](#repository-structure)


This repository contains Messir-MSS source code. It needs [messir-mss-dependencies](https://bitbucket.org/corobor/messir-mss-dependencies/) to have been previously installed and built and installed.


## Build

Before anything, pull clone this repository.

```shell
git clone git@bitbucket.org:corobor/messir-mss.git
```

### Using CMake (recommended)

On Linux and Windows, simply use the cmake projects, and build using following commands (make sure cmake is in your PATH):

First, configure the cmake build : 

  - On Windows : `cmake -G "Visual Studio 17 2022" -A Win32 -B .build`
  - On Linux : `cmake -B .build`

Then, you can build : 

```shell
cd .build
cmake --build . --config RelWithDebInfo
cmake --install . --config RelWithDebInfo
```

To build debug binaries, replace `--config RelWithDebInfo` with `--config Debug` in previous command lines.

The resulting binaries are in `../messir-mss_install/bin`.

### Using Visual Studio (Windows only)

One can use the MSVC solution located in `Build/Meteo.sln`.

This should only be used to do quick tests on the compile options. The produced binaries shouldn't be used in production, not even on Corobor test plateforms.


### Docker build

Messir-MSS docker image (defined in `Dockerfile`, and used by `bitbucket-pipeline.yml` for BitBucket CI) inherits from a couple of other images, as described in the following diagram : 

![Documentation/docker-images.svg](Documentation/docker-images.svg)

The main point is that more or less everything is bundled in [messir-mss-base](https://bitbucket.org/corobor/messir-mss-base/) docker image.

The dependencies are built in a seperate [messir-mss-dependencies](https://bitbucket.org/corobor/messir-mss-dependencies/) image, which is then temporarily imported in messir-mss image (using [docker multi-stage build feature](https://docs.docker.com/develop/develop-images/multistage-build/)) to produce Messir-MSS binaries, which are finally installed in the final Messir-MSS image.


Usual standard way of building a docker image : 

`docker build . --tag coroborsystems/messir-mss:<tag>`


## Repository structure

The source code is organized as follows : 

- [/](.) : CI scripts (Dockerfile, bitbucket-pipeline.yml, CMakeLists.txt toplevel project)

- [Build/](Build/) : all build related tools (cmake scripts, VS solution, etc.)
  
- [Documentation/](Documentation/) : Messir MSS documentations : 
    - [Developers documentation](Documentation/not yet added)
    - [Project Manager's Manual](Documentation/not yet added)
    - [User Manual](Documentation/not yet added)
    - [Testing documentation](Documentation/not yet added)
    - [Release notes](Documentation/not yet added)
  
- [Source/](Source/) : Messir-MSS source tree
    - [Comm/](Source/Comm/) : Messir-MSS usual binaries (Messir-Comm, CommUI, etc.)
    - [ImageMaker/](Source/ImageMaker/) : ImageMaker libraries and binaries
    - [SharedLibraries/](Source/SharedLibraries/) : Corobor-made libraries on which all other objects rely
    - [Tools/](Source/Tools/) : Historical tools (some are still used, some don't even compile anymore)
    - [Vision/](Source/Vision/) : Messir-Vision related binaries

- [Tests/](Tests/) : unit tests, functional tests scripts
