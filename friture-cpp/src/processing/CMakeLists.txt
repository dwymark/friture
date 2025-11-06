# Processing library - FFT and signal processing components

add_library(friture_processing
    fft_processor.cpp
    frequency_resampler.cpp
    color_transform.cpp
)

target_include_directories(friture_processing PUBLIC
    ${CMAKE_SOURCE_DIR}/include
)

# Link FFTW3 float library
target_link_libraries(friture_processing PUBLIC
    fftw3f  # Float version for single-precision
    m       # Math library for log10, cos, etc.
)

# Enable all warnings
target_compile_options(friture_processing PRIVATE
    -Wall -Wextra -Wpedantic
)
