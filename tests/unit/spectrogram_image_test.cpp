/**
 * @file spectrogram_image_test.cpp
 * @brief Unit tests for SpectrogramImage class
 */

#include <friture/spectrogram_image.hpp>
#include <gtest/gtest.h>
#include <vector>
#include <cstdint>
#include <fstream>
#include <chrono>

using namespace friture;

// ============================================================================
// Construction and Initialization Tests
// ============================================================================

TEST(SpectrogramImageTest, Construction) {
    SpectrogramImage image(1920, 1080);

    EXPECT_EQ(image.getWidth(), 1920u);
    EXPECT_EQ(image.getHeight(), 1080u);
    EXPECT_EQ(image.getTotalPixels(), 2u * 1920u * 1080u);
    EXPECT_EQ(image.getWriteOffset(), 0u);
    EXPECT_EQ(image.getReadOffset(), 0u);
}

TEST(SpectrogramImageTest, ConstructionInvalidDimensions) {
    EXPECT_THROW(SpectrogramImage(0, 1080), std::invalid_argument);
    EXPECT_THROW(SpectrogramImage(1920, 0), std::invalid_argument);
    EXPECT_THROW(SpectrogramImage(0, 0), std::invalid_argument);
}

TEST(SpectrogramImageTest, InitialPixelsAreBlack) {
    SpectrogramImage image(10, 10);

    const uint32_t* pixels = image.getPixelData();
    for (size_t i = 0; i < image.getTotalPixels(); ++i) {
        EXPECT_EQ(pixels[i], 0x00000000u);
    }
}

TEST(SpectrogramImageTest, SmallDimensions) {
    SpectrogramImage image(1, 1);
    EXPECT_EQ(image.getWidth(), 1u);
    EXPECT_EQ(image.getHeight(), 1u);
    EXPECT_EQ(image.getTotalPixels(), 2u);
}

TEST(SpectrogramImageTest, LargeDimensions) {
    // Test with typical full HD spectrogram size
    SpectrogramImage image(1920, 1080);

    size_t expected_memory = 2 * 1920 * 1080 * 4; // bytes
    EXPECT_EQ(image.getMemoryUsage(), expected_memory);
}

// ============================================================================
// Column Addition Tests
// ============================================================================

TEST(SpectrogramImageTest, AddSingleColumn) {
    SpectrogramImage image(10, 5);

    std::vector<uint32_t> column(5);
    for (size_t i = 0; i < 5; ++i) {
        column[i] = 0xFF0000FF; // Red
    }

    image.addColumn(column.data(), 5);

    EXPECT_EQ(image.getWriteOffset(), 1u);
    EXPECT_EQ(image.getReadOffset(), 0u);

    // Verify column was written
    const uint32_t* pixels = image.getPixelData();
    for (size_t i = 0; i < 5; ++i) {
        EXPECT_EQ(pixels[i], 0xFF0000FFu);
    }
}

TEST(SpectrogramImageTest, AddMultipleColumns) {
    SpectrogramImage image(10, 5);

    std::vector<uint32_t> column(5, 0xFF00FF00); // Green

    for (size_t col = 0; col < 3; ++col) {
        image.addColumn(column.data(), 5);
    }

    EXPECT_EQ(image.getWriteOffset(), 3u);
    EXPECT_EQ(image.getReadOffset(), 0u);

    // Verify columns were written
    const uint32_t* pixels = image.getPixelData();
    for (size_t col = 0; col < 3; ++col) {
        for (size_t row = 0; row < 5; ++row) {
            size_t idx = col * 5 + row;
            EXPECT_EQ(pixels[idx], 0xFF00FF00u);
        }
    }
}

TEST(SpectrogramImageTest, AddColumnInvalidHeight) {
    SpectrogramImage image(10, 5);

    std::vector<uint32_t> column(10); // Wrong height
    EXPECT_THROW(image.addColumn(column.data(), 10), std::invalid_argument);
}

TEST(SpectrogramImageTest, AddUniqueColumns) {
    SpectrogramImage image(5, 3);

    // Add columns with unique colors
    for (uint32_t col = 0; col < 3; ++col) {
        std::vector<uint32_t> column(3);
        for (size_t row = 0; row < 3; ++row) {
            column[row] = (col << 16) | (row << 8); // Unique color per position
        }
        image.addColumn(column.data(), 3);
    }

    // Verify each column has correct colors
    const uint32_t* pixels = image.getPixelData();
    for (uint32_t col = 0; col < 3; ++col) {
        for (size_t row = 0; row < 3; ++row) {
            size_t idx = col * 3 + row;
            uint32_t expected = (col << 16) | (row << 8);
            EXPECT_EQ(pixels[idx], expected);
        }
    }
}

// ============================================================================
// Ring Buffer Wrapping Tests
// ============================================================================

TEST(SpectrogramImageTest, WrapAroundAtWidth) {
    SpectrogramImage image(5, 3);

    std::vector<uint32_t> column(3, 0xFFFFFFFF); // White

    // Add exactly 'width' columns
    for (size_t col = 0; col < 5; ++col) {
        image.addColumn(column.data(), 3);
    }

    EXPECT_EQ(image.getWriteOffset(), 5u);
    EXPECT_EQ(image.getReadOffset(), 0u);

    // Add one more column - should NOT wrap yet (buffer is 2×width)
    image.addColumn(column.data(), 3);
    EXPECT_EQ(image.getWriteOffset(), 6u);
    EXPECT_EQ(image.getReadOffset(), 1u); // Read offset starts moving
}

TEST(SpectrogramImageTest, WrapAroundAt2xWidth) {
    SpectrogramImage image(5, 3);

    std::vector<uint32_t> column(3, 0xFFFFFFFF);

    // Add exactly 2×width columns (10 columns)
    for (size_t col = 0; col < 10; ++col) {
        image.addColumn(column.data(), 3);
    }

    // After 10 columns with width=5:
    // - write_offset wraps to 0
    // - read_offset should show the 5 most recent columns (5-9), so read_offset = 5
    EXPECT_EQ(image.getWriteOffset(), 0u); // Wrapped around
    EXPECT_EQ(image.getReadOffset(), 5u);  // Showing columns 5-9

    // Add one more (11th column) at position 0
    // - write_offset advances to 1
    // - read_offset should show columns 6-10, which are at positions 6-9,0
    // - So read_offset = 6
    image.addColumn(column.data(), 3);
    EXPECT_EQ(image.getWriteOffset(), 1u);
    EXPECT_EQ(image.getReadOffset(), 6u);
}

TEST(SpectrogramImageTest, ContinuousWrapping) {
    SpectrogramImage image(3, 2);

    std::vector<uint32_t> column(2);

    // Add many columns to test continuous wrapping
    for (size_t col = 0; col < 20; ++col) {
        column[0] = col;        // Row 0 stores column number
        column[1] = col + 100;  // Row 1 stores column number + 100
        image.addColumn(column.data(), 2);
    }

    // After 20 columns with width=3:
    // - Buffer size is 2*3 = 6
    // - write_offset = 20 % 6 = 2
    // - We're showing columns 17-19 (the 3 most recent)
    // - These are at buffer positions (17%6=5), (18%6=0), (19%6=1)
    // - So read_offset should trail write_offset by width:
    //   read_offset = (2 + 6 - 3) % 6 = 5
    EXPECT_EQ(image.getWriteOffset(), 2u);

    // Since write_offset (2) < width (3), we use: read_offset = write_offset + width = 2 + 3 = 5
    EXPECT_EQ(image.getReadOffset(), 5u);
}

// ============================================================================
// Read Offset Tracking Tests
// ============================================================================

TEST(SpectrogramImageTest, ReadOffsetInitialFill) {
    SpectrogramImage image(10, 5);

    std::vector<uint32_t> column(5, 0xFF0000FF);

    // During initial fill (< width columns), read offset stays at 0
    for (size_t col = 0; col < 9; ++col) {
        image.addColumn(column.data(), 5);
        EXPECT_EQ(image.getReadOffset(), 0u);
    }

    // After writing 'width' columns, read offset still at 0
    image.addColumn(column.data(), 5); // 10th column
    EXPECT_EQ(image.getWriteOffset(), 10u);
    EXPECT_EQ(image.getReadOffset(), 0u); // Still at 0

    // After writing width+1 columns, read offset starts moving
    image.addColumn(column.data(), 5); // 11th column
    EXPECT_EQ(image.getWriteOffset(), 11u);
    EXPECT_EQ(image.getReadOffset(), 1u); // Now moves
}

TEST(SpectrogramImageTest, ReadOffsetTrailsWrite) {
    SpectrogramImage image(5, 3);

    std::vector<uint32_t> column(3, 0xFFFFFFFF);

    // Add more than width columns
    for (size_t col = 0; col < 15; ++col) {
        image.addColumn(column.data(), 3);

        if (image.getWriteOffset() >= 5) {
            // Read should trail write by 'width' (5 columns)
            size_t expected_read = image.getWriteOffset() - 5;
            EXPECT_EQ(image.getReadOffset(), expected_read);
        }
    }
}

// ============================================================================
// Clear and Reset Tests
// ============================================================================

TEST(SpectrogramImageTest, Clear) {
    SpectrogramImage image(5, 3);

    // Add some colored columns
    std::vector<uint32_t> column(3, 0xFF00FF00);
    for (size_t col = 0; col < 3; ++col) {
        image.addColumn(column.data(), 3);
    }

    // Clear the image
    image.clear();

    EXPECT_EQ(image.getWriteOffset(), 0u);
    EXPECT_EQ(image.getReadOffset(), 0u);

    // All pixels should be black
    const uint32_t* pixels = image.getPixelData();
    for (size_t i = 0; i < image.getTotalPixels(); ++i) {
        EXPECT_EQ(pixels[i], 0x00000000u);
    }
}

TEST(SpectrogramImageTest, ClearAfterWrap) {
    SpectrogramImage image(3, 2);

    std::vector<uint32_t> column(2, 0xFFFFFFFF);

    // Add many columns to trigger wrapping
    for (size_t col = 0; col < 10; ++col) {
        image.addColumn(column.data(), 2);
    }

    image.clear();

    EXPECT_EQ(image.getWriteOffset(), 0u);
    EXPECT_EQ(image.getReadOffset(), 0u);

    const uint32_t* pixels = image.getPixelData();
    for (size_t i = 0; i < image.getTotalPixels(); ++i) {
        EXPECT_EQ(pixels[i], 0x00000000u);
    }
}

// ============================================================================
// Resize Tests
// ============================================================================

TEST(SpectrogramImageTest, Resize) {
    SpectrogramImage image(10, 5);

    // Add some data
    std::vector<uint32_t> column(5, 0xFF0000FF);
    image.addColumn(column.data(), 5);

    // Resize to larger dimensions
    image.resize(20, 10);

    EXPECT_EQ(image.getWidth(), 20u);
    EXPECT_EQ(image.getHeight(), 10u);
    EXPECT_EQ(image.getTotalPixels(), 2u * 20u * 10u);
    EXPECT_EQ(image.getWriteOffset(), 0u);
    EXPECT_EQ(image.getReadOffset(), 0u);

    // All pixels should be black after resize
    const uint32_t* pixels = image.getPixelData();
    for (size_t i = 0; i < image.getTotalPixels(); ++i) {
        EXPECT_EQ(pixels[i], 0x00000000u);
    }
}

TEST(SpectrogramImageTest, ResizeSmaller) {
    SpectrogramImage image(20, 10);

    image.resize(5, 3);

    EXPECT_EQ(image.getWidth(), 5u);
    EXPECT_EQ(image.getHeight(), 3u);
    EXPECT_EQ(image.getTotalPixels(), 2u * 5u * 3u);
}

TEST(SpectrogramImageTest, ResizeInvalidDimensions) {
    SpectrogramImage image(10, 5);

    EXPECT_THROW(image.resize(0, 5), std::invalid_argument);
    EXPECT_THROW(image.resize(10, 0), std::invalid_argument);
}

// ============================================================================
// Memory Usage Tests
// ============================================================================

TEST(SpectrogramImageTest, MemoryUsage) {
    SpectrogramImage image(100, 50);

    size_t expected = 2 * 100 * 50 * sizeof(uint32_t);
    EXPECT_EQ(image.getMemoryUsage(), expected);
}

TEST(SpectrogramImageTest, MemoryUsageAfterResize) {
    SpectrogramImage image(10, 10);

    size_t initial_memory = image.getMemoryUsage();
    EXPECT_EQ(initial_memory, 2u * 10u * 10u * 4u);

    image.resize(20, 20);

    size_t new_memory = image.getMemoryUsage();
    EXPECT_EQ(new_memory, 2u * 20u * 20u * 4u);
    EXPECT_EQ(new_memory, initial_memory * 4); // 2x width, 2x height = 4x memory
}

// ============================================================================
// BMP Save Tests
// ============================================================================

TEST(SpectrogramImageTest, SaveToBMP) {
    SpectrogramImage image(10, 10);

    // Fill with gradient pattern
    for (size_t col = 0; col < 10; ++col) {
        std::vector<uint32_t> column(10);
        for (size_t row = 0; row < 10; ++row) {
            uint8_t intensity = static_cast<uint8_t>((col * 10 + row) * 255 / 99);
            column[row] = 0xFF000000 | (intensity << 16) | (intensity << 8) | intensity;
        }
        image.addColumn(column.data(), 10);
    }

    // Save to file
    bool success = image.saveToBMP("/tmp/test_spectrogram.bmp");
    EXPECT_TRUE(success);

    // Verify file exists
    std::ifstream file("/tmp/test_spectrogram.bmp", std::ios::binary);
    EXPECT_TRUE(file.good());

    // Check BMP magic number
    char magic[2];
    file.read(magic, 2);
    EXPECT_EQ(magic[0], 'B');
    EXPECT_EQ(magic[1], 'M');
}

TEST(SpectrogramImageTest, SaveEmptyImage) {
    SpectrogramImage image(5, 5);

    bool success = image.saveToBMP("/tmp/empty_spectrogram.bmp");
    EXPECT_TRUE(success);
}

// ============================================================================
// Edge Case Tests
// ============================================================================

TEST(SpectrogramImageTest, SinglePixelImage) {
    SpectrogramImage image(1, 1);

    std::vector<uint32_t> column(1, 0xFFFF0000); // Blue
    image.addColumn(column.data(), 1);

    const uint32_t* pixels = image.getPixelData();
    EXPECT_EQ(pixels[0], 0xFFFF0000u);
}

TEST(SpectrogramImageTest, TallNarrowImage) {
    SpectrogramImage image(2, 1000);

    EXPECT_EQ(image.getWidth(), 2u);
    EXPECT_EQ(image.getHeight(), 1000u);

    std::vector<uint32_t> column(1000, 0xFF00FF00);
    image.addColumn(column.data(), 1000);

    EXPECT_EQ(image.getWriteOffset(), 1u);
}

TEST(SpectrogramImageTest, WideShortImage) {
    SpectrogramImage image(1000, 2);

    EXPECT_EQ(image.getWidth(), 1000u);
    EXPECT_EQ(image.getHeight(), 2u);

    std::vector<uint32_t> column(2, 0xFF0000FF);
    for (size_t i = 0; i < 10; ++i) {
        image.addColumn(column.data(), 2);
    }

    EXPECT_EQ(image.getWriteOffset(), 10u);
}

// ============================================================================
// Performance Hint Tests
// ============================================================================

TEST(SpectrogramImageTest, ManyColumnAdditions) {
    SpectrogramImage image(1920, 1080);

    std::vector<uint32_t> column(1080, 0xFFFFFFFF);

    // Add many columns (simulating real-time operation)
    auto start = std::chrono::high_resolution_clock::now();

    const size_t num_columns = 1000;
    for (size_t i = 0; i < num_columns; ++i) {
        image.addColumn(column.data(), 1080);
    }

    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);

    double avg_time_us = static_cast<double>(duration.count()) / num_columns;

    // Should be fast: < 10 μs per column on modern hardware
    EXPECT_LT(avg_time_us, 10.0);

    std::cout << "Average time per column: " << avg_time_us << " μs\n";
}

// ============================================================================
// Main
// ============================================================================

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
