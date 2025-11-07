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
if(WIN32)
    find_library(FFTW3F_LIBRARY fftw3f)
    target_link_libraries(friture_processing PUBLIC
        ${FFTW3F_LIBRARY}  # Float version for single-precision
    )
else()
    target_link_libraries(friture_processing PUBLIC
        fftw3f  # Float version for single-precision
        m       # Math library for log10, cos, etc.
    )
endif()

# Enable all warnings
if(MSVC)
    target_compile_options(friture_processing PRIVATE /W4)
else()
    target_compile_options(friture_processing PRIVATE -Wall -Wextra -Wpedantic)
endif()
