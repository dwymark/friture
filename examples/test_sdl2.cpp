/**
 * SDL2 Test - Create window and render triangle
 * Run with xpra for headless testing
 */
#include <SDL2/SDL.h>
#include <iostream>
#include <cmath>

bool renderFrame(SDL_Renderer* renderer, int frame) {
    // Clear to black
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_RenderClear(renderer);

    // Draw a colorful gradient triangle
    int centerX = 400;
    int centerY = 300;
    int radius = 100 + 50 * std::sin(frame * 0.05);

    // Draw filled triangle
    for (int y = -radius; y <= radius; ++y) {
        for (int x = -radius; x <= radius; ++x) {
            // Simple triangle check
            if (y > 0 && std::abs(x) < radius - y) {
                int pixelX = centerX + x;
                int pixelY = centerY + y;

                // Color based on position
                Uint8 r = static_cast<Uint8>(128 + 127 * std::sin(frame * 0.1 + x * 0.1));
                Uint8 g = static_cast<Uint8>(128 + 127 * std::cos(frame * 0.1 + y * 0.1));
                Uint8 b = static_cast<Uint8>(128 + 127 * std::sin(frame * 0.1));

                SDL_SetRenderDrawColor(renderer, r, g, b, 255);
                SDL_RenderDrawPoint(renderer, pixelX, pixelY);
            }
        }
    }

    // Draw frame counter
    char text[32];
    snprintf(text, sizeof(text), "Frame: %d", frame);

    SDL_RenderPresent(renderer);
    return true;
}

int main(int argc, char* argv[]) {
    std::cout << "=== SDL2 Test ===" << std::endl;

    // Initialize SDL
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        std::cerr << "Failed to initialize SDL: " << SDL_GetError() << std::endl;
        return 1;
    }

    SDL_version compiled;
    SDL_version linked;
    SDL_VERSION(&compiled);
    SDL_GetVersion(&linked);

    std::cout << "SDL Compiled version: " << (int)compiled.major << "."
              << (int)compiled.minor << "." << (int)compiled.patch << std::endl;
    std::cout << "SDL Linked version: " << (int)linked.major << "."
              << (int)linked.minor << "." << (int)linked.patch << std::endl;

    // Create window
    SDL_Window* window = SDL_CreateWindow(
        "Friture C++ - SDL2 Test",
        SDL_WINDOWPOS_CENTERED,
        SDL_WINDOWPOS_CENTERED,
        800, 600,
        SDL_WINDOW_SHOWN
    );

    if (!window) {
        std::cerr << "Failed to create window: " << SDL_GetError() << std::endl;
        SDL_Quit();
        return 1;
    }

    std::cout << "Window created successfully" << std::endl;

    // Create renderer
    SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    if (!renderer) {
        std::cerr << "Failed to create renderer: " << SDL_GetError() << std::endl;
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }

    std::cout << "Renderer created successfully" << std::endl;

    // Render loop
    bool quit = false;
    SDL_Event event;
    int frame = 0;
    const int maxFrames = 60; // Render 60 frames (~2 seconds at 30fps)

    std::cout << "Rendering " << maxFrames << " frames..." << std::endl;

    while (!quit && frame < maxFrames) {
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) {
                quit = true;
            }
        }

        renderFrame(renderer, frame);
        frame++;

        SDL_Delay(33); // ~30 FPS
    }

    std::cout << "Rendered " << frame << " frames" << std::endl;

    // Save final frame as BMP for verification
    SDL_Surface* surface = SDL_CreateRGBSurface(0, 800, 600, 32, 0, 0, 0, 0);
    if (surface) {
        SDL_RenderReadPixels(renderer, nullptr, surface->format->format,
                            surface->pixels, surface->pitch);
        const char* filename = "sdl2_test_output.bmp";
        if (SDL_SaveBMP(surface, filename) == 0) {
            std::cout << "Screenshot saved to: " << filename << std::endl;
        }
        SDL_FreeSurface(surface);
    }

    // Cleanup
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();

    std::cout << "\n✓ SDL2 test PASSED" << std::endl;
    return 0;
}
