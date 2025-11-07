# Audio module - WAV file loading and real-time input

add_library(friture_audio STATIC
    audio_file_loader.cpp
    audio_engine.cpp
)

target_include_directories(friture_audio PUBLIC
    ${CMAKE_SOURCE_DIR}/include
)

# Platform-specific RtAudio configuration
if(WIN32)
    # Windows: Use vcpkg's RtAudio package
    find_package(RtAudio CONFIG REQUIRED)
    target_link_libraries(friture_audio RtAudio::rtaudio)
else()
    # Linux/macOS: Use third_party/rtaudio source files
    target_sources(friture_audio PRIVATE
        ${CMAKE_SOURCE_DIR}/third_party/rtaudio/RtAudio.cpp
    )

    target_include_directories(friture_audio PRIVATE
        ${CMAKE_SOURCE_DIR}/third_party/rtaudio
    )

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
    endif()
endif()
