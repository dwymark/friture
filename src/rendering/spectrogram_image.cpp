/**
 * @file spectrogram_image.cpp
 * @brief Implementation of SpectrogramImage class
 */

#include <friture/spectrogram_image.hpp>
#include <stdexcept>
#include <fstream>

namespace friture {

SpectrogramImage::SpectrogramImage(size_t width, size_t height)
    : width_(width),
      height_(height),
      write_offset_(0),
      read_offset_(0),
      columns_written_(0),
      pixels_(2 * width * height, 0x00000000) {

    if (width == 0 || height == 0) {
        throw std::invalid_argument("Width and height must be > 0");
    }
}

void SpectrogramImage::addColumn(const uint32_t* column_data, size_t column_height) {
    if (column_height != height_) {
        throw std::invalid_argument("Column height must match image height");
    }

    // Calculate destination offset in pixel buffer
    // Layout: pixels are stored column-major within the ring buffer
    // Each column occupies 'height_' consecutive pixels
    size_t dest_offset = write_offset_ * height_;

    // Copy column data into buffer
    std::memcpy(pixels_.data() + dest_offset, column_data, height_ * sizeof(uint32_t));

    // Increment total columns written
    columns_written_++;

    // Advance write offset with wrapping
    write_offset_ = (write_offset_ + 1) % (2 * width_);

    // Update read offset to maintain proper display window
    updateReadOffset();
}

void SpectrogramImage::updateReadOffset() {
    // The read offset should trail the write offset by 'width_' columns
    // This ensures we always display the most recent 'width_' columns

    // During initial fill (less than 'width_' columns written), read from start
    if (columns_written_ <= width_) {
        read_offset_ = 0;
    } else {
        // After initial fill, read_offset trails write_offset by 'width_' in circular buffer
        // write_offset points to where we'll write NEXT
        // We want to read from (write_offset - width), wrapping correctly
        if (write_offset_ >= width_) {
            read_offset_ = write_offset_ - width_;
        } else {
            // write_offset has wrapped, so read_offset should be in second half
            read_offset_ = write_offset_ + width_;
        }
    }
}

void SpectrogramImage::clear() {
    std::fill(pixels_.begin(), pixels_.end(), 0x00000000);
    write_offset_ = 0;
    read_offset_ = 0;
    columns_written_ = 0;
}

void SpectrogramImage::resize(size_t new_width, size_t new_height) {
    if (new_width == 0 || new_height == 0) {
        throw std::invalid_argument("Width and height must be > 0");
    }

    width_ = new_width;
    height_ = new_height;
    write_offset_ = 0;
    read_offset_ = 0;
    columns_written_ = 0;

    // Reallocate buffer
    pixels_.clear();
    pixels_.resize(2 * width_ * height_, 0x00000000);
}

bool SpectrogramImage::saveToBMP(const char* filename) const {
    // BMP format parameters
    const uint32_t file_header_size = 14;
    const uint32_t info_header_size = 40;
    const uint32_t header_size = file_header_size + info_header_size;
    const uint32_t row_size = width_ * 4; // RGBA: 4 bytes per pixel
    const uint32_t pixel_data_size = row_size * height_;
    const uint32_t file_size = header_size + pixel_data_size;

    std::ofstream file(filename, std::ios::binary);
    if (!file) {
        return false;
    }

    // BMP File Header (14 bytes)
    uint8_t file_header[14] = {
        'B', 'M',                              // Signature
        static_cast<uint8_t>(file_size),       // File size (little-endian)
        static_cast<uint8_t>(file_size >> 8),
        static_cast<uint8_t>(file_size >> 16),
        static_cast<uint8_t>(file_size >> 24),
        0, 0, 0, 0,                            // Reserved
        static_cast<uint8_t>(header_size),     // Pixel data offset
        static_cast<uint8_t>(header_size >> 8),
        static_cast<uint8_t>(header_size >> 16),
        static_cast<uint8_t>(header_size >> 24)
    };
    file.write(reinterpret_cast<const char*>(file_header), file_header_size);

    // BMP Info Header (40 bytes)
    uint8_t info_header[40] = {
        40, 0, 0, 0,                                    // Header size
        static_cast<uint8_t>(width_),                   // Width (little-endian)
        static_cast<uint8_t>(width_ >> 8),
        static_cast<uint8_t>(width_ >> 16),
        static_cast<uint8_t>(width_ >> 24),
        static_cast<uint8_t>(height_),                  // Height (little-endian)
        static_cast<uint8_t>(height_ >> 8),
        static_cast<uint8_t>(height_ >> 16),
        static_cast<uint8_t>(height_ >> 24),
        1, 0,                                           // Planes
        32, 0,                                          // Bits per pixel (32-bit RGBA)
        0, 0, 0, 0,                                     // Compression (none)
        static_cast<uint8_t>(pixel_data_size),         // Image size
        static_cast<uint8_t>(pixel_data_size >> 8),
        static_cast<uint8_t>(pixel_data_size >> 16),
        static_cast<uint8_t>(pixel_data_size >> 24),
        0, 0, 0, 0,                                     // X pixels per meter
        0, 0, 0, 0,                                     // Y pixels per meter
        0, 0, 0, 0,                                     // Colors in palette
        0, 0, 0, 0                                      // Important colors
    };
    file.write(reinterpret_cast<const char*>(info_header), info_header_size);

    // Write pixel data (BMP stores bottom-to-top, left-to-right)
    // We need to handle the ring buffer wrap-around and flip vertically
    std::vector<uint32_t> row_buffer(width_);

    for (int y = static_cast<int>(height_) - 1; y >= 0; --y) {
        // Extract one row across all visible columns, handling wrap-around
        for (size_t x = 0; x < width_; ++x) {
            size_t column_idx = (read_offset_ + x) % (2 * width_);
            size_t pixel_idx = column_idx * height_ + y;
            row_buffer[x] = pixels_[pixel_idx];
        }

        // Write row to file
        file.write(reinterpret_cast<const char*>(row_buffer.data()), row_size);
    }

    file.close();
    return file.good();
}

} // namespace friture
