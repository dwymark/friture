# Rendering components

add_library(friture_rendering
    spectrogram_image.cpp
)

target_include_directories(friture_rendering PUBLIC
    ${CMAKE_SOURCE_DIR}/include
)

# No external dependencies for SpectrogramImage (pure C++)
