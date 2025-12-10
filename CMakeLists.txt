cmake_minimum_required(VERSION 3.15)
# Allow use of the old FindBoost module on new CMake versions
if(POLICY CMP0167)
    cmake_policy(SET CMP0167 OLD)
endif()

project(e572las)

# ------------------------------------------------------------------
# User-configurable options for E57 libraries and include directory
# ------------------------------------------------------------------
set(E57LIBS "" CACHE FILEPATH "Path to the E57LIBS library")
set(E57LIB_INCLUDE_DIRS "" CACHE PATH "Include directory for E57 headers")
set(LASLIB_LIB "" CACHE FILEPATH "Path to laslib library (.lib)")
set(LASLIB_INCLUDE_DIRS "" CACHE PATH "Include root directory for LASlib / LASzip headers")
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

set(XercesC_USE_STATIC_LIBS On)
find_package(XercesC QUIET)
if (NOT XercesC_FOUND)
    set(XercesC_ROOT CACHE PATH "Location of the XercesC library")
    message(FATAL_ERROR
            "Unable to find XercesC library.
Please set the the XercesC_ROOT to point to the root of the XercesC directory."
    )
endif (NOT XercesC_FOUND)


add_definitions(${Boost_LIB_DIAGNOSTIC_DEFINITIONS})
add_definitions(-DBOOST_ALL_NO_LIB -DXercesC_STATIC_LIBRARY)

include_directories(
        ${LASLIB_INCLUDE_DIRS}
        ${E57LIB_INCLUDE_DIRS}
        ${E57LIB_INCLUDE_DIRS}/time_conversion
        ${XercesC_INCLUDE_DIR}
        ${Boost_INCLUDE_DIR}
)

link_directories(
        ${Boost_LIBRARY_DIRS}
)

add_executable( e572las
        e572las.cpp
)
target_link_libraries( e572las
        ${E57LIBS}
        ${LASLIB_LIB}
        ${XercesC_LIBRARY}
        Boost::program_options
        Boost::filesystem
        ${CMAKE_THREAD_LIBS_INIT}
)


