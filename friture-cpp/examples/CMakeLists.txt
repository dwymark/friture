# Example programs for testing dependencies

# ============================================================================
# Pipeline Integration Test
# ============================================================================

add_executable(pipeline_test pipeline_test.cpp)

target_link_libraries(pipeline_test
    friture_processing
    friture_rendering
)

target_include_directories(pipeline_test PRIVATE
    ${CMAKE_SOURCE_DIR}/include
)

# ============================================================================
# Dependency Tests
# ============================================================================

# 1. PortAudio test
add_executable(test_portaudio test_portaudio.cpp)
if(WIN32)
    target_link_libraries(test_portaudio ${PORTAUDIO_LIBRARY})
else()
    target_link_libraries(test_portaudio ${PORTAUDIO_LIBRARY} asound pthread m)
endif()

# 2. SDL2 test
add_executable(test_sdl2 test_sdl2.cpp)
target_link_libraries(test_sdl2 ${SDL2_LIBRARIES})

# 3. FFT test
add_executable(test_fft test_fft.cpp)
if(USE_FFTW)
    if(WIN32)
        target_link_libraries(test_fft ${FFTW3_LIBRARY} fftw3f)
    else()
        target_link_libraries(test_fft ${FFTW3_LIBRARY} fftw3f m)
    endif()
else()
    target_link_libraries(test_fft Eigen3::Eigen)
endif()

# 4. Eigen test
add_executable(test_eigen test_eigen.cpp)
target_link_libraries(test_eigen Eigen3::Eigen)
