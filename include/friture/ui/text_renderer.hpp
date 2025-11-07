/**
 * @file text_renderer.hpp
 * @brief SDL2_ttf text rendering utility for UI overlays
 *
 * This class provides convenient text rendering for the Friture application,
 * including FPS counters, status messages, help text, and axis labels.
 *
 * Features:
 * - Simple text rendering with SDL2_ttf
 * - Multiple font sizes
 * - Configurable colors
 * - Cached font handles for performance
 * - Fallback to system fonts if custom fonts unavailable
 *
 * @author Friture C++ Port
 * @date 2025-11-06
 */

#ifndef FRITURE_TEXT_RENDERER_HPP
#define FRITURE_TEXT_RENDERER_HPP

#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <string>
#include <memory>
#include <unordered_map>

namespace friture {

/**
 * @brief Text rendering utility using SDL2_ttf
 *
 * This class manages TTF font loading and provides convenient methods
 * for rendering text to SDL surfaces and textures.
 *
 * Usage:
 * @code
 * TextRenderer text(renderer);
 * text.renderText("FPS: 60", 10, 10, {255, 255, 255, 255}, 16);
 * text.renderText("Paused", 100, 100, {255, 0, 0, 255}, 24);
 * @endcode
 *
 * Thread Safety: Not thread-safe. Use from main/rendering thread only.
 */
class TextRenderer {
public:
    /**
     * @brief Construct TextRenderer with SDL renderer
     * @param renderer SDL renderer to render text to
     * @throws std::runtime_error if SDL_ttf initialization fails
     */
    explicit TextRenderer(SDL_Renderer* renderer);

    /**
     * @brief Destructor - cleans up fonts and SDL_ttf
     */
    ~TextRenderer();

    /**
     * @brief Render text at specified position
     * @param text Text string to render
     * @param x X position (pixels from left)
     * @param y Y position (pixels from top)
     * @param color Text color (RGBA)
     * @param font_size Font size in points (default: 16)
     * @return true if rendered successfully, false on error
     *
     * Renders text directly to the renderer's current target.
     */
    bool renderText(const std::string& text, int x, int y,
                   SDL_Color color, int font_size = 16);

    /**
     * @brief Render text with shadow for better visibility
     * @param text Text string to render
     * @param x X position (pixels from left)
     * @param y Y position (pixels from top)
     * @param color Text color (RGBA)
     * @param shadow_color Shadow color (RGBA)
     * @param font_size Font size in points
     * @param shadow_offset Shadow offset in pixels (default: 1)
     * @return true if rendered successfully, false on error
     *
     * Renders text with a shadow offset for better readability
     * on busy backgrounds.
     */
    bool renderTextWithShadow(const std::string& text, int x, int y,
                             SDL_Color color, SDL_Color shadow_color,
                             int font_size = 16, int shadow_offset = 1);

    /**
     * @brief Render text right-aligned
     * @param text Text string to render
     * @param x Right edge X position
     * @param y Y position (pixels from top)
     * @param color Text color (RGBA)
     * @param font_size Font size in points
     * @return true if rendered successfully, false on error
     */
    bool renderTextRightAlign(const std::string& text, int x, int y,
                             SDL_Color color, int font_size = 16);

    /**
     * @brief Render text center-aligned
     * @param text Text string to render
     * @param x Center X position
     * @param y Y position (pixels from top)
     * @param color Text color (RGBA)
     * @param font_size Font size in points
     * @return true if rendered successfully, false on error
     */
    bool renderTextCentered(const std::string& text, int x, int y,
                           SDL_Color color, int font_size = 16);

    /**
     * @brief Get text dimensions without rendering
     * @param text Text string to measure
     * @param font_size Font size in points
     * @param width Output: text width in pixels
     * @param height Output: text height in pixels
     * @return true if successful, false on error
     */
    bool getTextSize(const std::string& text, int font_size,
                    int& width, int& height);

    /**
     * @brief Get last error message
     * @return Error string, or empty if no error
     */
    const std::string& getError() const { return error_; }

    /**
     * @brief Check if TextRenderer is initialized
     * @return true if ready to render, false if initialization failed
     */
    bool isValid() const { return initialized_; }

private:
    /**
     * @brief Initialize SDL_ttf library
     * @return true on success, false on failure
     */
    bool initializeTTF();

    /**
     * @brief Load font at specified size
     * @param font_size Font size in points
     * @return Font handle, or nullptr on error
     *
     * Fonts are cached - subsequent calls with same size return cached font.
     */
    TTF_Font* loadFont(int font_size);

    /**
     * @brief Find system font to use
     * @return Path to font file, or empty string if not found
     *
     * Searches for common system fonts:
     * - Ubuntu: /usr/share/fonts/truetype/dejavu/DejaVuSans.ttf
     * - Debian: /usr/share/fonts/truetype/liberation/LiberationSans-Regular.ttf
     * - Generic: /usr/share/fonts/truetype/liberation2/LiberationSans-Regular.ttf
     */
    std::string findSystemFont() const;

    /**
     * @brief Set error message
     * @param message Error description
     */
    void setError(const std::string& message);

    /**
     * @brief Render text to surface
     * @param text Text to render
     * @param font Font to use
     * @param color Text color
     * @return SDL_Surface with rendered text, or nullptr on error
     */
    SDL_Surface* renderTextToSurface(const std::string& text,
                                    TTF_Font* font, SDL_Color color);

    SDL_Renderer* renderer_;                ///< SDL renderer
    std::string font_path_;                  ///< Path to font file
    std::unordered_map<int, TTF_Font*> fonts_; ///< Cached fonts by size
    std::string error_;                      ///< Last error message
    bool initialized_;                       ///< Initialization status

    // Prevent copying
    TextRenderer(const TextRenderer&) = delete;
    TextRenderer& operator=(const TextRenderer&) = delete;
};

} // namespace friture

#endif // FRITURE_TEXT_RENDERER_HPP
