/**
 * @file text_renderer.cpp
 * @brief Implementation of SDL2_ttf text rendering utility
 */

#include <friture/ui/text_renderer.hpp>
#include <iostream>
#include <fstream>

namespace friture {

// ============================================================================
// Constructor / Destructor
// ============================================================================

TextRenderer::TextRenderer(SDL_Renderer* renderer)
    : renderer_(renderer),
      initialized_(false)
{
    if (!renderer_) {
        setError("SDL renderer is null");
        return;
    }

    if (!initializeTTF()) {
        return;
    }

    // Find system font
    font_path_ = findSystemFont();
    if (font_path_.empty()) {
        setError("Could not find system font");
        return;
    }

    initialized_ = true;
}

TextRenderer::~TextRenderer() {
    // Free all cached fonts
    for (auto& pair : fonts_) {
        if (pair.second) {
            TTF_CloseFont(pair.second);
        }
    }
    fonts_.clear();

    // Quit SDL_ttf if it was initialized
    if (initialized_) {
        TTF_Quit();
    }
}

// ============================================================================
// Initialization
// ============================================================================

bool TextRenderer::initializeTTF() {
    if (TTF_Init() < 0) {
        setError(std::string("SDL_ttf initialization failed: ") + TTF_GetError());
        return false;
    }
    return true;
}

std::string TextRenderer::findSystemFont() const {
    // Common font paths on Linux systems
    const char* font_paths[] = {
        "/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf",
        "/usr/share/fonts/truetype/liberation/LiberationSans-Regular.ttf",
        "/usr/share/fonts/truetype/liberation2/LiberationSans-Regular.ttf",
        "/usr/share/fonts/TTF/DejaVuSans.ttf",
        "/usr/share/fonts/liberation-sans/LiberationSans-Regular.ttf",
        // Fallback to any TrueType font
        "/usr/share/fonts/truetype/freefont/FreeSans.ttf",
        nullptr
    };

    for (size_t i = 0; font_paths[i] != nullptr; ++i) {
        std::ifstream file(font_paths[i]);
        if (file.good()) {
            return font_paths[i];
        }
    }

    return "";
}

// ============================================================================
// Font Loading
// ============================================================================

TTF_Font* TextRenderer::loadFont(int font_size) {
    // Check if font already cached
    auto it = fonts_.find(font_size);
    if (it != fonts_.end()) {
        return it->second;
    }

    // Load new font at this size
    TTF_Font* font = TTF_OpenFont(font_path_.c_str(), font_size);
    if (!font) {
        setError(std::string("Failed to load font: ") + TTF_GetError());
        return nullptr;
    }

    // Cache for future use
    fonts_[font_size] = font;
    return font;
}

// ============================================================================
// Text Rendering
// ============================================================================

SDL_Surface* TextRenderer::renderTextToSurface(const std::string& text,
                                              TTF_Font* font,
                                              SDL_Color color) {
    if (!font || text.empty()) {
        return nullptr;
    }

    // Render text to surface (blended for better quality)
    SDL_Surface* surface = TTF_RenderText_Blended(font, text.c_str(), color);
    if (!surface) {
        setError(std::string("Text rendering failed: ") + TTF_GetError());
        return nullptr;
    }

    return surface;
}

bool TextRenderer::renderText(const std::string& text, int x, int y,
                              SDL_Color color, int font_size) {
    if (!initialized_ || text.empty()) {
        return false;
    }

    TTF_Font* font = loadFont(font_size);
    if (!font) {
        return false;
    }

    SDL_Surface* surface = renderTextToSurface(text, font, color);
    if (!surface) {
        return false;
    }

    // Create texture from surface
    SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer_, surface);
    if (!texture) {
        SDL_FreeSurface(surface);
        setError(std::string("Texture creation failed: ") + SDL_GetError());
        return false;
    }

    // Render texture
    SDL_Rect dst_rect = {x, y, surface->w, surface->h};
    SDL_RenderCopy(renderer_, texture, nullptr, &dst_rect);

    // Cleanup
    SDL_DestroyTexture(texture);
    SDL_FreeSurface(surface);

    return true;
}

bool TextRenderer::renderTextWithShadow(const std::string& text, int x, int y,
                                       SDL_Color color, SDL_Color shadow_color,
                                       int font_size, int shadow_offset) {
    // Render shadow first (offset)
    if (!renderText(text, x + shadow_offset, y + shadow_offset,
                   shadow_color, font_size)) {
        return false;
    }

    // Render main text on top
    return renderText(text, x, y, color, font_size);
}

bool TextRenderer::renderTextRightAlign(const std::string& text, int x, int y,
                                       SDL_Color color, int font_size) {
    int width, height;
    if (!getTextSize(text, font_size, width, height)) {
        return false;
    }

    // Adjust x position to right-align
    return renderText(text, x - width, y, color, font_size);
}

bool TextRenderer::renderTextCentered(const std::string& text, int x, int y,
                                     SDL_Color color, int font_size) {
    int width, height;
    if (!getTextSize(text, font_size, width, height)) {
        return false;
    }

    // Adjust x position to center
    return renderText(text, x - width / 2, y, color, font_size);
}

// ============================================================================
// Utility Methods
// ============================================================================

bool TextRenderer::getTextSize(const std::string& text, int font_size,
                               int& width, int& height) {
    if (!initialized_ || text.empty()) {
        return false;
    }

    TTF_Font* font = loadFont(font_size);
    if (!font) {
        return false;
    }

    if (TTF_SizeText(font, text.c_str(), &width, &height) < 0) {
        setError(std::string("Failed to get text size: ") + TTF_GetError());
        return false;
    }

    return true;
}

void TextRenderer::setError(const std::string& message) {
    error_ = message;
    std::cerr << "TextRenderer error: " << message << std::endl;
}

} // namespace friture
