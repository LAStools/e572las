# e572las build

This guide explains how to compile and build the `e572las` binary.
There are 2 dependencies:
- LAStools
- libe57
We build both, LAStools and libe57 from the public github sources.
To solve the dependencies of libe57 we use vcpkg.

These are the steps to compile and build `e572las`:

# vcpkg
First install [vcpkg](https://vcpkg.io/).

######################################################################
# LAStools

You need LAStools because `e572las` will link against LASlib and LASzip from this repository.

    git clone https://github.com/LAStools/LAStools.git

Then build LAStools according to its own instructions (e.g. using its provided Makefiles or CMake setup).
Typically this is done by
    cmake .
    cmake --build . --config Release
   

######################################################################
# libe57

## Download libe57 source

Download the E57 reference implementation archive (source code only) from:

    http://www.libe57.org/download.html
    http://sourceforge.net/projects/e57-3d-imgfmt/files/E57Refimpl-src/E57RefImpl_src-1.1.312.zip/download
   
Unpack it so that you have a directory like:

    /path_to/E57RefImpl_src-1.1.312/

## Update CMakeLists.txt for a modern toolchain

Open `CMakeLists.txt` in `E57RefImpl_src-1.1.312/` and replace:

```cpp
cmake_minimum_required(VERSION 2.8.2)
```

with:

```cpp
cmake_minimum_required(VERSION 3.21)
# Allow use of the old FindBoost module on new CMake versions
if(POLICY CMP0167)
  cmake_policy(SET CMP0167 OLD)
endif()
```

This raises the required CMake version and configures the Boost policy so that the legacy FindBoost logic still works as expected on newer CMake versions.
Then make the following replacements in the same `CMakeLists.txt`:

- Replace all occurrences of `Xerces` with `XercesC` (required on windows, optional on linux).

This aligns the CMake configuration with the target name used by the Xerces-C package as provided by vcpkg and modern CMake find modules.

- Replace:

```cpp
find_package(Boost
COMPONENTS
  program_options
  system
  thread
  filesystem
QUIET
)
if (NOT Boost_FOUND)
  set(BOOST_ROOT CACHE PATH  "Location of the boost root directory" )
  message(FATAL_ERROR
"Unable to find boost library.
Please set the BOOST_ROOT to point to the boost distribution files."
)
endif(NOT Boost_FOUND)
```

with:

```cpp
set(Boost_USE_STATIC_LIBS   ON)
set(Boost_USE_STATIC_RUNTIME ON)
set(Boost_USE_MULTITHREADED ON)
find_package(Boost REQUIRED COMPONENTS program_options filesystem)
if (NOT Boost_FOUND)
  set(BOOST_ROOT CACHE PATH  "Location of the boost root directory" )
  message(FATAL_ERROR
"Unable to find boost library.
Please set the BOOST_ROOT to point to the boost distribution files."
)
endif(NOT Boost_FOUND)
link_directories(
     ${Boost_LIBRARY_DIRS}
)
```

This configures Boost to use static, multithreaded libraries and switches to the components you actually use (`program_options` and `filesystem`). It also ensures the Boost library directories are added to the linker search path.

- Under the comment block:

```cpp
#
# The reference implementation
#
```

add the E57Simple static library target:

```cpp
add_library( E57Simple STATIC
  src/refimpl/E57Simple.cpp
  src/refimpl/E57SimpleImpl.cpp
  include/time_conversion/time_conversion.h
  src/time_conversion/time_conversion.c
)
set_target_properties( E57Simple
  PROPERTIES DEBUG_POSTFIX "-d"
)
```

This defines a static library target `E57Simple` which you can later link against from your own code (`e572las`).

- Replace the `target_link_libraries` call for `e57unpack`:

```cpp
target_link_libraries( e57unpack
  E57RefImpl
  ${XML_LIBRARIES}
  ${Boost_LIBRARIES}
  ${CMAKE_THREAD_LIBS_INIT}
)
```

with:

```cpp
target_link_libraries( e57unpack
  E57RefImpl
  ${XML_LIBRARIES}
  Boost::program_options
  Boost::filesystem
  ${CMAKE_THREAD_LIBS_INIT}
)
```

This switches linking from the old `${Boost_LIBRARIES}` variable to modern imported Boost targets created by `find_package(Boost)`.

## Create a vcpkg manifest for libe57

In the same folder as `CMakeLists.txt` (i.e. `E57RefImpl_src-1.1.312/`), create a file called `vcpkg.json` with the following content:

```json
{
  "name": "e57simple",
  "version": "1.0.0",
  "dependencies": [
    "boost-crc",
    "boost-uuid",
    "boost-math",
    "boost-variant",
    "boost-format",
    "boost-program-options",
    "boost-filesystem",
    "xerces-c"
 ]
}
```

This manifest tells vcpkg which dependencies to install for the libe57 project.

## Adjust E57Simple.h to handle Windows headers cleanly

In `include/E57Simple.h` replace the lines

```cpp
#ifndef _C_TIMECONV_H_
#include "time_conversion.h"
#endif
```
with

```cpp
#ifdef _WIN32
#  define NOMINMAX
#  include <windows.h>
#undef min
#undef max
#endif
```
This ensures that on Windows, the `min` and `max` macros from `windows.h` do not interfere with standard C++ functions or templates, and it stops including `time_conversion.h` directly from `E57Simple.h`.

## Fix Windows min/max interference in E57SimpleImpl.cpp

At the beginning of `src/refimpl/E57SimpleImpl.cpp` add 

```cpp
#    undef min
#    undef max
```
behind the line

```cpp
#    include <windows.h>
```
This again makes sure that `min` and `max` macros from the Windows headers do not break C++ code relying on those names.


## Modernize e57fields.cpp to use <memory>

In `src/tools/e57fields.cpp`, replace:

```cpp
#if defined(_MSC_VER)
#   include <memory>
#else
#   include <tr1/memory>
#endif
```
with:

```cpp
#include <memory>
```

and remove the 

```cpp
using namespace std::tr1;
```
This assumes a modern compiler where `<memory>` and `std::shared_ptr` are available everywhere, and removes the outdated dependency on the outdated technical report 1 `<tr1/memory>`.


## Configure and build libe57 with vcpkg toolchain

From the `E57RefImpl_src-1.1.312/` directory, configure and build using the vcpkg toolchain file:

```bash
cmake -B build -S . -DCMAKE_TOOLCHAIN_FILE=/path_to/vcpkg/scripts/buildsystems/vcpkg.cmake
cmake --build build/ --config Release
```

Replace `/path_to/vcpkg` with the actual path to your vcpkg installation. This will generate the `libE57RefImpl` and `libE57Simple` libraries in the `build` directory (typically under `build/Release` on multi-config generators).


######################################################################
# Compile e572las with vcpkg
## Linux

After libe57 and LAStools are built, you can configure and build `e572las` so that it links against these libraries.

From the `e572las` source directory, run:

```bash
cmake -S . -B build -DCMAKE_TOOLCHAIN_FILE=/path_to/vcpkg/scripts/buildsystems/vcpkg.cmake -DE57LIBS="/path_to/E57RefImpl_src-1.1.312/build/libE57Simple.a;path_to/E57RefImpl_src-1.1.312/build/libE57RefImpl.a" -DE57LIB_INCLUDE_DIRS="/path_to/E57RefImpl_src-1.1.312/include" -DLASLIB_INCLUDE_DIRS="/path_to/LAStools/LASlib/inc;/path_to/LAStools/LASzip/include/laszip;/path_to/LAStools/LASzip/src" -DLASLIB_LIB="/path_to/LAStools/LASlib/lib/libLASlib.a"
cmake --build build --config Release
```

- `-DCMAKE_TOOLCHAIN_FILE=/path_to/vcpkg/scripts/buildsystems/vcpkg.cmake`
  tells CMake to use vcpkg for dependency resolution.
- `-DE57LIBS="...libE57Simple.a;...libE57RefImpl.a"`  
  points to the static E57 libraries built in the previous steps.
- `-DE57LIB_INCLUDE_DIRS="/path_to/E57RefImpl_src-1.1.312"`  
  points to the E57RefImpl include root so that `#include <E57Simple.h>` and related headers are found.
- `-DLASLIB_INCLUDE_DIRS="...LAStools/LASlib/inc;...LAStools/LASzip/include/laszip;.../LAStools/LASzip/src"` and `-DLASLIB_LIB="...libLASlib.a"`  
  point to your LAStools includes and LASlib static library.

Adjust the paths so they match your local folder structure.

## Windows (MSVC)

After building libE57 and LAStools on Windows, run:

```powershell
cmake -S . -B build -DCMAKE_TOOLCHAIN_FILE=/path_to/vcpkg/scripts/buildsystems/vcpkg.cmake ^
 -DE57LIBS="/path_to/E57RefImpl.lib;/path_to/E57Simple.lib" ^
 -DE57LIB_INCLUDE_DIRS="/path_to/E57RefImpl_src-1.1.312/include" ^
 -DLASLIB_INCLUDE_DIRS="/path_to/LAStools/LASlib/inc;/path_to/LAStools/LASzip/include/laszip;/path_to/LAStools/LASzip/src" ^
 -DLASLIB_LIB="/path_to/LAStools/LASlib/lib/Debug/LASlib.lib"

cmake --build build --config Release
```
- `-DCMAKE_TOOLCHAIN_FILE=C:/path_to/vcpkg/scripts/buildsystems/vcpkg.cmake`  
  tells CMake to use vcpkg for dependency resolution.
- `-DE57LIBS="...E57Simple.lib;...E57RefImpl.lib"`  
  points to the E57 static libraries (`E57Simple.lib` and `E57RefImpl.lib`) built in the previous steps.
- `-DE57LIB_INCLUDE_DIRS="C:/path_to/E57RefImpl_src-1.1.312"`  
  points to the E57RefImpl include root so that `#include <E57Simple.h>` and related headers are found.
- `-DLASLIB_INCLUDE_DIRS="...LAStools/LASlib/inc;...LAStools/LASzip/include/laszip;.../LAStools/LASzip/src"` and `-DLASLIB_LIB="...LASlib.lib"`  
  point to your LAStools includes and the LASlib static library (`LASlib.lib`).

Adjust the paths so they match your local folder structure.

After a successful build the binary
    build\Release\e572las.exe
was created and is ready to use.

Please see the README.md file how to use the program.
