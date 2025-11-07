/**
 * @file spectrogram_image.hpp
 * @brief Spectrogram image buffer with ring buffer for scrolling display
 *
 * This class maintains a double-buffered ring buffer of spectrogram pixels
 * for smooth scrolling visualization. It stores columns of frequency data
 * as RGB colors and manages the scrolling window.
 *
 * @author Friture C++ Port
 * @date 2025-11-06
 */

#ifndef FRITURE_SPECTROGRAM_IMAGE_HPP
#define FRITURE_SPECTROGRAM_IMAGE_HPP

#include <cstdint>
#include <vector>
#include <cstring>
#include <algorithm>

namespace friture {

/**
 * @brief Ring buffer image storage for scrolling spectrogram
 *
 * The SpectrogramImage maintains a double-buffered pixel array (2× screen width)
 * to enable smooth horizontal scrolling. New columns are written sequentially,
 * and the read offset tracks where rendering should begin.
 *
 * Memory Layout:
 * - Total pixels: 2 × width × height
 * - Storage: Contiguous uint32_t array (RGBA format: 0xAABBGGRR)
 * - Columns are vertical (height pixels each)
 * - Wraps around at width boundary
 *
 * Thread Safety: Not thread-safe. Should be accessed from single thread
 * or protected with external synchronization.
 *
 * Performance:
 * - Column write: O(height) - simple memcpy
 * - Memory: 8 × width × height bytes (double buffered)
 * - Resize: Expensive, allocates new buffer
 *
 * Example:
 * @code
 * SpectrogramImage image(1920, 1080);
 *
 * // In processing loop:
 * std::vector<uint32_t> column(1080);
 * // ... fill column with colors ...
 * image.addColumn(column.data(), 1080);
 *
 * // For rendering:
 * const uint32_t* pixels = image.getPixelData();
 * size_t offset = image.getReadOffset();
 * // ... upload to GPU texture ...
 * @endcode
 */
class SpectrogramImage {
public:
    /**
     * @brief Construct spectrogram image with fixed dimensions
     * @param width Display width in pixels (columns)
     * @param height Display height in pixels (rows/frequency bins)
     *
     * Allocates 2 × width × height × 4 bytes for double buffering.
     * All pixels are initialized to black (0x00000000).
     *
     * Example: 1920×1080 display uses ~16.6 MB
     */
    SpectrogramImage(size_t width, size_t height);

    /**
     * @brief Add new column of pixels to the spectrogram
     * @param column_data Array of RGBA colors (must have 'height' elements)
     * @param column_height Number of pixels in column (must equal height)
     *
     * This method copies the column data into the ring buffer at the current
     * write position and advances the write pointer. When reaching the end,
     * it wraps around to the beginning.
     *
     * Performance: ~1-2 μs for 1080 pixels (simple memcpy)
     *
     * Thread Safety: Not thread-safe with concurrent addColumn() or getPixelData() calls.
     *
     * @throws std::invalid_argument if column_height doesn't match height
     */
    void addColumn(const uint32_t* column_data, size_t column_height);

    /**
     * @brief Get pointer to pixel data for rendering
     * @return Pointer to RGBA pixel array (2 × width × height elements)
     *
     * The returned pointer is valid until the next resize() call.
     * Pixels are stored in row-major order with double buffering.
     *
     * To render correctly with scrolling:
     * @code
     * const uint32_t* pixels = image.getPixelData();
     * size_t offset = image.getReadOffset();
     * // Render from column 'offset' to 'offset + width' (with wrapping)
     * @endcode
     *
     * Thread Safety: Not thread-safe with concurrent addColumn() calls.
     */
    const uint32_t* getPixelData() const { return pixels_.data(); }

    /**
     * @brief Get non-const pointer to pixel data (for direct manipulation)
     * @return Pointer to RGBA pixel array
     *
     * Use with caution. Prefer addColumn() for normal operation.
     */
    uint32_t* getPixelDataMutable() { return pixels_.data(); }

    /**
     * @brief Get current read offset for scrolling rendering
     * @return Column index where rendering should start [0, width)
     *
     * The read offset points to the oldest column in the ring buffer.
     * For correct scrolling, render columns from [offset, offset+width)
     * with wrapping at the buffer edge.
     */
    size_t getReadOffset() const { return read_offset_; }

    /**
     * @brief Get current write position
     * @return Column index where next column will be written [0, width)
     *
     * Useful for debugging and synchronization.
     */
    size_t getWriteOffset() const { return write_offset_; }

    /**
     * @brief Get display width (number of columns visible)
     * @return Width in pixels
     */
    size_t getWidth() const { return width_; }

    /**
     * @brief Get display height (number of frequency bins)
     * @return Height in pixels
     */
    size_t getHeight() const { return height_; }

    /**
     * @brief Get total pixel count in buffer
     * @return Total pixels (2 × width × height)
     */
    size_t getTotalPixels() const { return pixels_.size(); }

    /**
     * @brief Clear entire image to black
     *
     * Sets all pixels to 0x00000000 (transparent black).
     * Resets write offset to 0.
     *
     * Performance: O(width × height) - memset
     */
    void clear();

    /**
     * @brief Resize image buffer
     * @param new_width New display width
     * @param new_height New display height
     *
     * This reallocates the buffer and clears all content.
     * Relatively expensive operation; avoid calling frequently.
     *
     * Performance: O(new_width × new_height) - allocation + memset
     *
     * @throws std::invalid_argument if width or height is 0
     */
    void resize(size_t new_width, size_t new_height);

    /**
     * @brief Get memory usage in bytes
     * @return Memory used by pixel buffer
     */
    size_t getMemoryUsage() const {
        return pixels_.size() * sizeof(uint32_t);
    }

    /**
     * @brief Save image to BMP file (for debugging)
     * @param filename Output filename (e.g., "spectrogram.bmp")
     * @return true if successful, false on error
     *
     * Saves the current visible portion (width × height) to a BMP file.
     * Handles scrolling offset automatically.
     *
     * Note: This is a convenience method for debugging. For production,
     * use a proper image library (SDL_Surface, stb_image_write, etc.)
     */
    bool saveToBMP(const char* filename) const;

private:
    /**
     * @brief Update read offset after writing a column
     *
     * The read offset follows the write offset with a delay equal to
     * the display width, ensuring we always show the most recent data.
     */
    void updateReadOffset();

    size_t width_;              ///< Display width (columns visible on screen)
    size_t height_;             ///< Display height (frequency bins / rows)
    size_t write_offset_;       ///< Current column write position [0, 2*width)
    size_t read_offset_;        ///< Current column read position [0, 2*width)
    size_t columns_written_;    ///< Total number of columns written (for tracking wrap)

    /**
     * @brief Pixel storage: 2 × width × height RGBA values
     *
     * Layout: Row-major with double buffering
     * - Each column has 'height' pixels
     * - Total columns: 2 × width
     * - Pixel format: 0xAABBGGRR (little-endian RGBA)
     *
     * Access pattern: pixels_[column * height + row]
     */
    std::vector<uint32_t> pixels_;

    // Prevent copying (would be expensive)
    SpectrogramImage(const SpectrogramImage&) = delete;
    SpectrogramImage& operator=(const SpectrogramImage&) = delete;

    // Move operations could be added if needed
    SpectrogramImage(SpectrogramImage&&) = default;
    SpectrogramImage& operator=(SpectrogramImage&&) = default;
};

} // namespace friture

#endif // FRITURE_SPECTROGRAM_IMAGE_HPP
