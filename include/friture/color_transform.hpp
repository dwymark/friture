/**
 * @file color_transform.hpp
 * @brief Color transformation for spectrogram visualization
 *
 * This class converts normalized amplitude values [0,1] to RGB colors
 * using various color themes. Supports CMRMAP (monochrome-compatible)
 * and Grayscale themes for different visualization preferences.
 *
 * @author Friture C++ Port
 * @date 2025-11-06
 */

#ifndef FRITURE_COLOR_TRANSFORM_HPP
#define FRITURE_COLOR_TRANSFORM_HPP

#include <cstdint>
#include <array>
#include <algorithm>
#include <cmath>

namespace friture {

/**
 * @brief Color theme types for spectrogram visualization
 *
 * Different themes provide different perceptual characteristics:
 * - CMRMAP: Black→Purple→Red→Yellow→White (monochrome-compatible)
 * - Grayscale: Black (quiet) → White (dense/loud)
 */
enum class ColorTheme {
    CMRMAP,     ///< CMRMAP colormap (black-purple-red-yellow-white)
    Grayscale   ///< Grayscale (black=quiet, white=loud)
};

/**
 * @brief Fast color transformation using lookup tables
 *
 * The ColorTransform class provides efficient conversion from normalized
 * amplitude values [0,1] to RGBA colors using pre-computed lookup tables.
 * It supports multiple color themes optimized for different use cases.
 *
 * Thread Safety: Not thread-safe for theme changes. Create separate instances
 * for concurrent use or use external synchronization when changing themes.
 *
 * Performance:
 * - Single lookup: <1 ns (array access)
 * - Column transformation (1080 pixels): <1 μs (cache-friendly loop)
 * - Memory: ~1 KB per theme (256 × 4 bytes)
 *
 * Color Format:
 * - uint32_t RGBA in little-endian format: 0xAABBGGRR
 * - Bits 0-7:   Red channel
 * - Bits 8-15:  Green channel
 * - Bits 16-23: Blue channel
 * - Bits 24-31: Alpha channel (always 0xFF)
 *
 * Typical Usage:
 * @code
 * // Create transformer with CMRMAP theme
 * ColorTransform transformer(ColorTheme::CMRMAP);
 *
 * // In rendering loop:
 * std::vector<float> spectrum_db(1080);  // Normalized to [0,1]
 * std::vector<uint32_t> colors(1080);
 *
 * transformer.transformColumn(spectrum_db.data(), 1080, colors.data());
 * // colors now contains RGBA values ready for GPU upload
 * @endcode
 */
class ColorTransform {
public:
    /**
     * @brief Construct color transformer with specified theme
     * @param theme Color theme (CMRMAP or Grayscale)
     *
     * This constructor pre-computes the color lookup table for the
     * selected theme. The LUT generation is fast (~10 μs) and only
     * happens once per theme selection.
     */
    explicit ColorTransform(ColorTheme theme = ColorTheme::CMRMAP);

    /**
     * @brief Convert single normalized value to RGBA color
     * @param normalized_value Input value in range [0,1]
     * @return RGBA color as uint32_t (0xAABBGGRR format)
     *
     * Values outside [0,1] are automatically clamped.
     *
     * Performance: <1 ns (pure array lookup)
     *
     * Example:
     * @code
     * float amplitude = 0.75f;  // 75% of max
     * uint32_t color = transformer.valueToColor(amplitude);
     * uint8_t r = color & 0xFF;
     * uint8_t g = (color >> 8) & 0xFF;
     * uint8_t b = (color >> 16) & 0xFF;
     * @endcode
     */
    uint32_t valueToColor(float normalized_value) const;

    /**
     * @brief Transform column of values to colors (batch operation)
     * @param input Input values (must be normalized to [0,1])
     * @param height Number of values/pixels in column
     * @param output Output RGBA colors (must be pre-allocated)
     *
     * This is the primary method for spectrogram rendering. It processes
     * an entire column of frequency bins in one call with optimal cache
     * behavior.
     *
     * Values outside [0,1] are automatically clamped.
     *
     * Performance: <1 μs for 1080 pixels (sequential memory access)
     *
     * Note: Input and output buffers must not overlap.
     *
     * Example:
     * @code
     * // After FFT and frequency resampling:
     * std::vector<float> normalized(1080);
     * for (size_t i = 0; i < 1080; ++i) {
     *     // Normalize dB values to [0,1]
     *     normalized[i] = (spectrum_db[i] + 140.0f) / 140.0f;
     *     normalized[i] = std::clamp(normalized[i], 0.0f, 1.0f);
     * }
     *
     * std::vector<uint32_t> colors(1080);
     * transformer.transformColumn(normalized.data(), 1080, colors.data());
     * @endcode
     */
    void transformColumn(const float* input, size_t height,
                        uint32_t* output) const;

    /**
     * @brief Change color theme
     * @param theme New color theme
     *
     * This regenerates the lookup table for the new theme.
     * Relatively cheap operation (~10 μs).
     *
     * Note: Not thread-safe if called while transformColumn() is running.
     */
    void setTheme(ColorTheme theme);

    /**
     * @brief Get current color theme
     * @return Current theme
     */
    ColorTheme getTheme() const { return current_theme_; }

    /**
     * @brief Get luminance of a color
     * @param color RGBA color value
     * @return Luminance in range [0, 255]
     *
     * Uses standard RGB luminance formula:
     * L = 0.299*R + 0.587*G + 0.114*B
     *
     * Useful for testing monotonicity of colormaps.
     */
    static float getLuminance(uint32_t color);

private:
    /**
     * @brief Generate lookup table for current theme
     *
     * This method fills color_lut_ with 256 RGBA values corresponding
     * to normalized input values [0, 1/255, 2/255, ..., 255/255].
     */
    void generateLUT();

    /**
     * @brief Generate CMRMAP lookup table
     *
     * CMRMAP (Color Map for Monochrome Rendering) varies from:
     * Black → Purple → Red → Yellow → White
     *
     * This is a perceptually linear colormap that remains readable
     * when converted to grayscale. Based on research by Rappaport (2002).
     *
     * Reference:
     * [1] Rappaport, C. 2002: "A Color Map for Effective Black-and-White
     *     Rendering of Color Scale Images", IEEE Antenna's and Propagation
     *     Magazine, Vol.44, No.3, pp.94-96 (June).
     */
    void generateCMRMAP();

    /**
     * @brief Generate grayscale lookup table
     *
     * Simple linear grayscale: Black (0,0,0) → White (255,255,255)
     * - Black represents quiet/low amplitude
     * - White represents loud/high amplitude
     *
     * This theme is preferred by users who want minimal visual distraction
     * or need to match scientific publication standards.
     */
    void generateGrayscale();

    /**
     * @brief Convert RGB [0,1] to RGBA uint32_t
     * @param r Red channel [0,1]
     * @param g Green channel [0,1]
     * @param b Blue channel [0,1]
     * @return RGBA color (alpha=255)
     */
    static uint32_t toRGBA(float r, float g, float b);

    // Current theme
    ColorTheme current_theme_;

    // Lookup table: 256 RGBA colors
    // Index 0 = value 0.0, Index 255 = value 1.0
    std::array<uint32_t, 256> color_lut_;

    // Prevent copying (would need deep copy of LUT, but it's trivial to copy)
    // Actually, we can allow copying since it's just a small array
    // ColorTransform(const ColorTransform&) = default;
    // ColorTransform& operator=(const ColorTransform&) = default;
};

/**
 * @brief Convert ColorTheme enum to string
 * @param theme Color theme type
 * @return Human-readable string representation
 */
inline const char* toString(ColorTheme theme) {
    switch (theme) {
        case ColorTheme::CMRMAP:    return "CMRMAP";
        case ColorTheme::Grayscale: return "Grayscale";
        default:                    return "Unknown";
    }
}

} // namespace friture

#endif // FRITURE_COLOR_TRANSFORM_HPP
