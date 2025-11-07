# Audio module - WAV file loading and real-time input

add_library(friture_audio STATIC
    audio_file_loader.cpp
    audio_engine.cpp
)

target_include_directories(friture_audio PUBLIC
    ${CMAKE_SOURCE_DIR}/include
)

# Add RtAudio source files
target_sources(friture_audio PRIVATE
    ${CMAKE_SOURCE_DIR}/third_party/rtaudio/RtAudio.cpp
)

# Include RtAudio headers
target_include_directories(friture_audio PRIVATE
    ${CMAKE_SOURCE_DIR}/third_party/rtaudio
)

# Platform-specific audio libraries for RtAudio
if(UNIX AND NOT APPLE)
    # Linux: ALSA
    find_package(ALSA REQUIRED)
    target_link_libraries(friture_audio ALSA::ALSA pthread m)
    target_compile_definitions(friture_audio PRIVATE __LINUX_ALSA__)
elseif(APPLE)
    # macOS: CoreAudio
    find_library(COREAUDIO_LIBRARY CoreAudio)
    find_library(COREFOUNDATION_LIBRARY CoreFoundation)
    target_link_libraries(friture_audio
        ${COREAUDIO_LIBRARY}
        ${COREFOUNDATION_LIBRARY}
        pthread
        m
    )
    target_compile_definitions(friture_audio PRIVATE __MACOSX_CORE__)
elseif(WIN32)
    # Windows: WASAPI
    target_link_libraries(friture_audio
        ole32
        winmm
        ksuser
        mfplat
        mfuuid
        wmcodecdspuuid
    )
    target_compile_definitions(friture_audio PRIVATE __WINDOWS_WASAPI__)
else()
    # Fallback - just link math library
    target_link_libraries(friture_audio m)
endif()
