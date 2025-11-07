# Audio module - WAV file loading and real-time input

add_library(friture_audio STATIC
    audio_file_loader.cpp
    audio_engine.cpp
)

target_include_directories(friture_audio PUBLIC
    ${CMAKE_SOURCE_DIR}/include
)

# Platform-specific RtAudio configuration
# Use system RtAudio library on all platforms
find_package(PkgConfig REQUIRED)
pkg_check_modules(RTAUDIO REQUIRED rtaudio)

target_include_directories(friture_audio PUBLIC ${RTAUDIO_INCLUDE_DIRS})
target_link_libraries(friture_audio ${RTAUDIO_LIBRARIES} pthread m)
