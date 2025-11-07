# Source subdirectories
add_subdirectory(audio)
add_subdirectory(processing)
add_subdirectory(rendering)
add_subdirectory(ui)

# Main application executable
if(SDL2_FOUND)
    add_executable(friture
        main.cpp
        application.cpp
    )

    target_link_libraries(friture
        friture_audio
        friture_processing
        friture_rendering
        friture_ui
        ${SDL2_LIBRARIES}
        ${SDL2_TTF_LIBRARY}
    )

    message(STATUS "Main application 'friture' configured")
else()
    message(STATUS "SDL2 not found - skipping main application")
endif()
