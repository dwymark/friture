# UI module - Text rendering and UI components

add_library(friture_ui
    text_renderer.cpp
)

target_link_libraries(friture_ui
    ${SDL2_LIBRARIES}
    SDL2_ttf
)
