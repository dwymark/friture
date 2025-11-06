cmake_minimum_required(VERSION 3.20)
project(friture-cpp VERSION 0.1.0 LANGUAGES CXX C)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Compiler warnings and optimization
add_compile_options(-Wall -Wextra -Wpedantic -O2)

# Options
option(BUILD_TESTS "Build tests" ON)
option(BUILD_EXAMPLES "Build example programs" ON)
option(USE_FFTW "Use FFTW3 for FFT" ON)

# Find dependencies
find_package(PkgConfig REQUIRED)

# SDL2
pkg_check_modules(SDL2 REQUIRED sdl2)
include_directories(${SDL2_INCLUDE_DIRS})

# PortAudio
find_path(PORTAUDIO_INCLUDE_DIR portaudio.h)
find_library(PORTAUDIO_LIBRARY portaudio)
if(NOT PORTAUDIO_INCLUDE_DIR OR NOT PORTAUDIO_LIBRARY)
    message(FATAL_ERROR "PortAudio not found")
endif()

# Eigen3
find_package(Eigen3 REQUIRED NO_MODULE)

# FFTW3
if(USE_FFTW)
    find_path(FFTW3_INCLUDE_DIR fftw3.h)
    find_library(FFTW3_LIBRARY fftw3)
    if(FFTW3_INCLUDE_DIR AND FFTW3_LIBRARY)
        add_definitions(-DUSE_FFTW)
        message(STATUS "Using FFTW3 for FFT")
    else()
        message(STATUS "FFTW3 not found, will use Eigen")
        set(USE_FFTW OFF)
    endif()
endif()

# GoogleTest
if(BUILD_TESTS)
    find_package(GTest REQUIRED)
    enable_testing()
endif()

# Include directories
include_directories(
    ${CMAKE_SOURCE_DIR}/include
    ${PORTAUDIO_INCLUDE_DIR}
    ${FFTW3_INCLUDE_DIR}
)

# Subdirectories
if(BUILD_EXAMPLES)
    add_subdirectory(examples)
endif()

if(BUILD_TESTS)
    add_subdirectory(tests)
endif()

# Print configuration summary
message(STATUS "=== Friture C++ Configuration ===")
message(STATUS "C++ Standard: ${CMAKE_CXX_STANDARD}")
message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")
message(STATUS "SDL2: ${SDL2_VERSION}")
message(STATUS "Eigen3: ${EIGEN3_VERSION_STRING}")
message(STATUS "FFTW3: ${USE_FFTW}")
message(STATUS "Build tests: ${BUILD_TESTS}")
message(STATUS "Build examples: ${BUILD_EXAMPLES}")
