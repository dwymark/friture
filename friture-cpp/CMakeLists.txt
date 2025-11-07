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

# SDL2 (optional for now - needed for rendering later)
pkg_check_modules(SDL2 sdl2)
if(SDL2_FOUND)
    include_directories(${SDL2_INCLUDE_DIRS})
    message(STATUS "SDL2 found: ${SDL2_VERSION}")
else()
    message(STATUS "SDL2 not found - rendering features will be unavailable")
endif()

# SDL2_ttf (for text rendering)
find_library(SDL2_TTF_LIBRARY SDL2_ttf)
find_path(SDL2_TTF_INCLUDE_DIR SDL2/SDL_ttf.h)
if(SDL2_TTF_LIBRARY AND SDL2_TTF_INCLUDE_DIR)
    include_directories(${SDL2_TTF_INCLUDE_DIR})
    message(STATUS "SDL2_ttf found")
else()
    message(STATUS "SDL2_ttf not found - text rendering will be unavailable")
endif()

# PortAudio (optional for now - needed for audio input later)
find_path(PORTAUDIO_INCLUDE_DIR portaudio.h)
find_library(PORTAUDIO_LIBRARY portaudio)
if(PORTAUDIO_INCLUDE_DIR AND PORTAUDIO_LIBRARY)
    message(STATUS "PortAudio found")
else()
    message(STATUS "PortAudio not found - audio input features will be unavailable")
endif()

# Eigen3 (optional - was for FFT fallback, but we use FFTW3)
find_package(Eigen3 NO_MODULE QUIET)
if(Eigen3_FOUND)
    message(STATUS "Eigen3 found: ${EIGEN3_VERSION_STRING}")
else()
    message(STATUS "Eigen3 not found - not required for current implementation")
endif()

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
)

# Add optional include directories only if found
if(PORTAUDIO_INCLUDE_DIR)
    include_directories(${PORTAUDIO_INCLUDE_DIR})
endif()

if(FFTW3_INCLUDE_DIR)
    include_directories(${FFTW3_INCLUDE_DIR})
endif()

# Subdirectories
add_subdirectory(src)

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
