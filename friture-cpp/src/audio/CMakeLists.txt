# Audio module - WAV file loading

add_library(friture_audio STATIC
    audio_file_loader.cpp
)

target_include_directories(friture_audio PUBLIC
    ${CMAKE_SOURCE_DIR}/include
)

# Link math library if needed
target_link_libraries(friture_audio m)
