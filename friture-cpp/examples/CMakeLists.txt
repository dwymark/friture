# Example programs for testing dependencies

# 1. PortAudio test
add_executable(test_portaudio test_portaudio.cpp)
target_link_libraries(test_portaudio ${PORTAUDIO_LIBRARY} asound pthread m)

# 2. SDL2 test
add_executable(test_sdl2 test_sdl2.cpp)
target_link_libraries(test_sdl2 ${SDL2_LIBRARIES})

# 3. FFT test
add_executable(test_fft test_fft.cpp)
if(USE_FFTW)
    target_link_libraries(test_fft ${FFTW3_LIBRARY} m)
else()
    target_link_libraries(test_fft Eigen3::Eigen)
endif()

# 4. Eigen test
add_executable(test_eigen test_eigen.cpp)
target_link_libraries(test_eigen Eigen3::Eigen)
