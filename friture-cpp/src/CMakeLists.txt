# Source subdirectories
add_subdirectory(processing)
add_subdirectory(rendering)

# Main application executable
if(SDL2_FOUND)
    add_executable(friture
        main.cpp
        application.cpp
    )

    target_link_libraries(friture
        friture_processing
        friture_rendering
        ${SDL2_LIBRARIES}
    )

    message(STATUS "Main application 'friture' configured")
else()
    message(STATUS "SDL2 not found - skipping main application")
endif()
