/**
 * @file color_transform_test.cpp
 * @brief Comprehensive unit tests for ColorTransform
 *
 * Tests cover:
 * - Construction and initialization
 * - CMRMAP colormap accuracy
 * - Grayscale theme accuracy
 * - Monotonic luminance
 * - Edge cases (NaN, Inf, out-of-range)
 * - Batch transformation
 * - Theme switching
 * - Performance benchmarks
 */

#include <gtest/gtest.h>
#include <friture/color_transform.hpp>
#include <vector>
#include <cmath>
#include <chrono>
#include <iostream>
#include <iomanip>
#include <limits>

using namespace friture;

// ============================================================================
// Test Fixtures
// ============================================================================

class ColorTransformTest : public ::testing::Test {
protected:
    // Helper to extract RGB channels from RGBA
    struct RGB {
        uint8_t r, g, b;
    };

    static RGB extractRGB(uint32_t color) {
        RGB result;
        result.r = color & 0xFF;
        result.g = (color >> 8) & 0xFF;
        result.b = (color >> 16) & 0xFF;
        return result;
    }

    // Helper to compute luminance
    static float computeLuminance(uint32_t color) {
        return ColorTransform::getLuminance(color);
    }

    // Helper to check if luminance is monotonic
    static bool isMonotonicLuminance(const std::vector<uint32_t>& colors) {
        for (size_t i = 1; i < colors.size(); ++i) {
            float prev_lum = computeLuminance(colors[i-1]);
            float curr_lum = computeLuminance(colors[i]);
            if (curr_lum < prev_lum - 1.0f) { // Allow small floating point error
                return false;
            }
        }
        return true;
    }

    // Helper to print color info (headless-compatible)
    static void printColorInfo(const char* label, float value, uint32_t color) {
        RGB rgb = extractRGB(color);
        float lum = computeLuminance(color);
        std::cout << label << " (t=" << std::fixed << std::setprecision(2) << value << "): "
                  << "RGB(" << std::setw(3) << (int)rgb.r << ", "
                  << std::setw(3) << (int)rgb.g << ", "
                  << std::setw(3) << (int)rgb.b << ") "
                  << "Lum=" << std::fixed << std::setprecision(1) << lum << "\n";
    }
};

// ============================================================================
// Construction and Initialization Tests
// ============================================================================

TEST_F(ColorTransformTest, ConstructionDefault) {
    EXPECT_NO_THROW({
        ColorTransform transformer;
    });
}

TEST_F(ColorTransformTest, ConstructionCMRMAP) {
    EXPECT_NO_THROW({
        ColorTransform transformer(ColorTheme::CMRMAP);
        EXPECT_EQ(transformer.getTheme(), ColorTheme::CMRMAP);
    });
}

TEST_F(ColorTransformTest, ConstructionGrayscale) {
    EXPECT_NO_THROW({
        ColorTransform transformer(ColorTheme::Grayscale);
        EXPECT_EQ(transformer.getTheme(), ColorTheme::Grayscale);
    });
}

// ============================================================================
// CMRMAP Color Accuracy Tests
// ============================================================================

TEST_F(ColorTransformTest, CMRMAP_BlackAtZero) {
    ColorTransform transformer(ColorTheme::CMRMAP);
    uint32_t color = transformer.valueToColor(0.0f);
    RGB rgb = extractRGB(color);

    // At t=0, CMRMAP should be near-black (all channels close to 0)
    EXPECT_LT(rgb.r, 5) << "Red channel should be near 0";
    EXPECT_LT(rgb.g, 5) << "Green channel should be near 0";
    EXPECT_LT(rgb.b, 5) << "Blue channel should be near 0";

    printColorInfo("CMRMAP Black", 0.0f, color);
}

TEST_F(ColorTransformTest, CMRMAP_WhiteAtOne) {
    ColorTransform transformer(ColorTheme::CMRMAP);
    uint32_t color = transformer.valueToColor(1.0f);
    RGB rgb = extractRGB(color);

    // At t=1, CMRMAP should be near-white (all channels close to 255)
    EXPECT_GT(rgb.r, 250) << "Red channel should be near 255";
    EXPECT_GT(rgb.g, 250) << "Green channel should be near 255";
    EXPECT_GT(rgb.b, 250) << "Blue channel should be near 255";

    printColorInfo("CMRMAP White", 1.0f, color);
}

TEST_F(ColorTransformTest, CMRMAP_PurplePhase) {
    ColorTransform transformer(ColorTheme::CMRMAP);

    // Around t=0.25, CMRMAP transitions through purple
    // Purple has more blue than red/green
    uint32_t color = transformer.valueToColor(0.25f);
    RGB rgb = extractRGB(color);

    // Blue should dominate in purple phase
    EXPECT_GT(rgb.b, rgb.r) << "Blue should be greater than red in purple phase";
    EXPECT_GT(rgb.b, rgb.g) << "Blue should be greater than green in purple phase";

    printColorInfo("CMRMAP Purple", 0.25f, color);
}

TEST_F(ColorTransformTest, CMRMAP_RedPhase) {
    ColorTransform transformer(ColorTheme::CMRMAP);

    // Around t=0.5, CMRMAP transitions through red
    uint32_t color = transformer.valueToColor(0.5f);
    RGB rgb = extractRGB(color);

    // Red should dominate in red phase
    EXPECT_GT(rgb.r, rgb.g) << "Red should be greater than green in red phase";

    printColorInfo("CMRMAP Red", 0.5f, color);
}

TEST_F(ColorTransformTest, CMRMAP_YellowPhase) {
    ColorTransform transformer(ColorTheme::CMRMAP);

    // Around t=0.75, CMRMAP transitions through yellow
    // Yellow has high red and green, low blue
    uint32_t color = transformer.valueToColor(0.75f);
    RGB rgb = extractRGB(color);

    // Red and green should both be high
    EXPECT_GT(rgb.r, 150) << "Red should be high in yellow phase";
    EXPECT_GT(rgb.g, 100) << "Green should be significant in yellow phase";

    printColorInfo("CMRMAP Yellow", 0.75f, color);
}

TEST_F(ColorTransformTest, CMRMAP_AlphaChannel) {
    ColorTransform transformer(ColorTheme::CMRMAP);

    // Test several values
    for (float t = 0.0f; t <= 1.0f; t += 0.1f) {
        uint32_t color = transformer.valueToColor(t);
        uint8_t alpha = (color >> 24) & 0xFF;
        EXPECT_EQ(alpha, 255) << "Alpha should always be 255 at t=" << t;
    }
}

// ============================================================================
// Grayscale Theme Tests
// ============================================================================

TEST_F(ColorTransformTest, Grayscale_BlackAtZero) {
    ColorTransform transformer(ColorTheme::Grayscale);
    uint32_t color = transformer.valueToColor(0.0f);
    RGB rgb = extractRGB(color);

    EXPECT_EQ(rgb.r, 0) << "Red should be 0 for black";
    EXPECT_EQ(rgb.g, 0) << "Green should be 0 for black";
    EXPECT_EQ(rgb.b, 0) << "Blue should be 0 for black";

    printColorInfo("Grayscale Black", 0.0f, color);
}

TEST_F(ColorTransformTest, Grayscale_WhiteAtOne) {
    ColorTransform transformer(ColorTheme::Grayscale);
    uint32_t color = transformer.valueToColor(1.0f);
    RGB rgb = extractRGB(color);

    EXPECT_EQ(rgb.r, 255) << "Red should be 255 for white";
    EXPECT_EQ(rgb.g, 255) << "Green should be 255 for white";
    EXPECT_EQ(rgb.b, 255) << "Blue should be 255 for white";

    printColorInfo("Grayscale White", 1.0f, color);
}

TEST_F(ColorTransformTest, Grayscale_MidGray) {
    ColorTransform transformer(ColorTheme::Grayscale);
    uint32_t color = transformer.valueToColor(0.5f);
    RGB rgb = extractRGB(color);

    // At t=0.5, should be mid-gray (around 127-128)
    EXPECT_EQ(rgb.r, rgb.g) << "R should equal G for grayscale";
    EXPECT_EQ(rgb.g, rgb.b) << "G should equal B for grayscale";
    EXPECT_NEAR(rgb.r, 127, 1) << "Mid-gray should be around 127";

    printColorInfo("Grayscale Mid", 0.5f, color);
}

TEST_F(ColorTransformTest, Grayscale_LinearProgression) {
    ColorTransform transformer(ColorTheme::Grayscale);

    // Test at several points
    std::vector<float> test_values = {0.0f, 0.25f, 0.5f, 0.75f, 1.0f};

    for (float t : test_values) {
        uint32_t color = transformer.valueToColor(t);
        RGB rgb = extractRGB(color);

        // All channels should be equal (grayscale)
        EXPECT_EQ(rgb.r, rgb.g) << "R should equal G at t=" << t;
        EXPECT_EQ(rgb.g, rgb.b) << "G should equal B at t=" << t;

        // Value should be approximately t * 255
        int expected = static_cast<int>(t * 255.0f);
        EXPECT_NEAR(rgb.r, expected, 1) << "Grayscale value incorrect at t=" << t;
    }
}

// ============================================================================
// Monotonic Luminance Tests
// ============================================================================

TEST_F(ColorTransformTest, CMRMAP_MonotonicLuminance) {
    ColorTransform transformer(ColorTheme::CMRMAP);

    // Sample 256 points and check luminance increases
    std::vector<uint32_t> colors;
    for (int i = 0; i < 256; ++i) {
        float t = i / 255.0f;
        colors.push_back(transformer.valueToColor(t));
    }

    EXPECT_TRUE(isMonotonicLuminance(colors))
        << "CMRMAP luminance should increase monotonically";

    // Print luminance progression
    std::cout << "\n=== CMRMAP Luminance Progression ===\n";
    for (size_t i = 0; i < colors.size(); i += 32) {
        float t = i / 255.0f;
        float lum = computeLuminance(colors[i]);
        std::cout << "t=" << std::fixed << std::setprecision(2) << t
                  << " -> Lum=" << std::fixed << std::setprecision(1) << lum << "\n";
    }
}

TEST_F(ColorTransformTest, Grayscale_MonotonicLuminance) {
    ColorTransform transformer(ColorTheme::Grayscale);

    std::vector<uint32_t> colors;
    for (int i = 0; i < 256; ++i) {
        float t = i / 255.0f;
        colors.push_back(transformer.valueToColor(t));
    }

    EXPECT_TRUE(isMonotonicLuminance(colors))
        << "Grayscale luminance should increase monotonically";
}

// ============================================================================
// Edge Case Tests
// ============================================================================

TEST_F(ColorTransformTest, ClampNegativeValue) {
    ColorTransform transformer(ColorTheme::Grayscale);

    // Negative values should be clamped to 0
    uint32_t color_neg = transformer.valueToColor(-1.0f);
    uint32_t color_zero = transformer.valueToColor(0.0f);

    EXPECT_EQ(color_neg, color_zero) << "Negative values should clamp to 0";
}

TEST_F(ColorTransformTest, ClampLargeValue) {
    ColorTransform transformer(ColorTheme::Grayscale);

    // Values > 1 should be clamped to 1
    uint32_t color_large = transformer.valueToColor(10.0f);
    uint32_t color_one = transformer.valueToColor(1.0f);

    EXPECT_EQ(color_large, color_one) << "Values > 1 should clamp to 1";
}

TEST_F(ColorTransformTest, HandleNaN) {
    ColorTransform transformer(ColorTheme::Grayscale);

    // NaN should be handled gracefully (clamped to 0)
    float nan_val = std::numeric_limits<float>::quiet_NaN();
    uint32_t color = transformer.valueToColor(nan_val);
    RGB rgb = extractRGB(color);

    // NaN should result in black (clamped to 0)
    EXPECT_EQ(rgb.r, 0) << "NaN should result in black (r=0)";
    EXPECT_EQ(rgb.g, 0) << "NaN should result in black (g=0)";
    EXPECT_EQ(rgb.b, 0) << "NaN should result in black (b=0)";
}

TEST_F(ColorTransformTest, HandleInfinity) {
    ColorTransform transformer(ColorTheme::Grayscale);

    // +Inf should clamp to white
    float inf_val = std::numeric_limits<float>::infinity();
    uint32_t color = transformer.valueToColor(inf_val);
    RGB rgb = extractRGB(color);

    EXPECT_EQ(rgb.r, 255) << "+Inf should result in white (r=255)";
    EXPECT_EQ(rgb.g, 255) << "+Inf should result in white (g=255)";
    EXPECT_EQ(rgb.b, 255) << "+Inf should result in white (b=255)";
}

TEST_F(ColorTransformTest, HandleNegativeInfinity) {
    ColorTransform transformer(ColorTheme::Grayscale);

    // -Inf should clamp to black
    float neg_inf = -std::numeric_limits<float>::infinity();
    uint32_t color = transformer.valueToColor(neg_inf);
    RGB rgb = extractRGB(color);

    EXPECT_EQ(rgb.r, 0) << "-Inf should result in black (r=0)";
    EXPECT_EQ(rgb.g, 0) << "-Inf should result in black (g=0)";
    EXPECT_EQ(rgb.b, 0) << "-Inf should result in black (b=0)";
}

// ============================================================================
// Batch Transformation Tests
// ============================================================================

TEST_F(ColorTransformTest, TransformColumn_FlatSpectrum) {
    ColorTransform transformer(ColorTheme::Grayscale);

    // Create flat spectrum at 0.5
    std::vector<float> input(1080, 0.5f);
    std::vector<uint32_t> output(1080);

    transformer.transformColumn(input.data(), 1080, output.data());

    // All outputs should be mid-gray
    RGB expected = extractRGB(transformer.valueToColor(0.5f));
    for (size_t i = 0; i < output.size(); ++i) {
        RGB rgb = extractRGB(output[i]);
        EXPECT_EQ(rgb.r, expected.r) << "Mismatch at index " << i;
        EXPECT_EQ(rgb.g, expected.g) << "Mismatch at index " << i;
        EXPECT_EQ(rgb.b, expected.b) << "Mismatch at index " << i;
    }
}

TEST_F(ColorTransformTest, TransformColumn_GradientSpectrum) {
    ColorTransform transformer(ColorTheme::Grayscale);

    // Create gradient from 0 to 1
    size_t height = 1080;
    std::vector<float> input(height);
    std::vector<uint32_t> output(height);

    for (size_t i = 0; i < height; ++i) {
        input[i] = static_cast<float>(i) / static_cast<float>(height - 1);
    }

    transformer.transformColumn(input.data(), height, output.data());

    // Check gradient progresses correctly
    for (size_t i = 1; i < height; ++i) {
        float lum_prev = computeLuminance(output[i-1]);
        float lum_curr = computeLuminance(output[i]);
        EXPECT_GE(lum_curr, lum_prev - 1.0f)
            << "Luminance should increase or stay same in gradient";
    }
}

TEST_F(ColorTransformTest, TransformColumn_MatchesSingleValue) {
    ColorTransform transformer(ColorTheme::CMRMAP);

    // Batch and single transformations should match
    std::vector<float> input = {0.0f, 0.25f, 0.5f, 0.75f, 1.0f};
    std::vector<uint32_t> output_batch(input.size());
    std::vector<uint32_t> output_single(input.size());

    // Batch transformation
    transformer.transformColumn(input.data(), input.size(), output_batch.data());

    // Single transformations
    for (size_t i = 0; i < input.size(); ++i) {
        output_single[i] = transformer.valueToColor(input[i]);
    }

    // Compare
    for (size_t i = 0; i < input.size(); ++i) {
        EXPECT_EQ(output_batch[i], output_single[i])
            << "Batch and single transformations should match at index " << i;
    }
}

// ============================================================================
// Theme Switching Tests
// ============================================================================

TEST_F(ColorTransformTest, SetTheme_CMRMAP_to_Grayscale) {
    ColorTransform transformer(ColorTheme::CMRMAP);

    uint32_t color_cmrmap = transformer.valueToColor(0.5f);

    transformer.setTheme(ColorTheme::Grayscale);
    EXPECT_EQ(transformer.getTheme(), ColorTheme::Grayscale);

    uint32_t color_gray = transformer.valueToColor(0.5f);

    // Colors should be different after theme change
    EXPECT_NE(color_cmrmap, color_gray)
        << "CMRMAP and Grayscale should produce different colors";
}

TEST_F(ColorTransformTest, SetTheme_Idempotent) {
    ColorTransform transformer(ColorTheme::Grayscale);

    uint32_t color_before = transformer.valueToColor(0.5f);

    // Setting same theme should be no-op
    transformer.setTheme(ColorTheme::Grayscale);

    uint32_t color_after = transformer.valueToColor(0.5f);

    EXPECT_EQ(color_before, color_after)
        << "Setting same theme should not change colors";
}

// ============================================================================
// Performance Benchmarks
// ============================================================================

TEST_F(ColorTransformTest, Performance_SingleLookup) {
    ColorTransform transformer(ColorTheme::CMRMAP);

    const int iterations = 1000000;
    volatile uint32_t result = 0; // Prevent optimization

    auto start = std::chrono::high_resolution_clock::now();

    for (int i = 0; i < iterations; ++i) {
        float t = (i % 256) / 255.0f;
        result = transformer.valueToColor(t);
    }

    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start);

    float avg_ns = duration.count() / static_cast<float>(iterations);

    std::cout << "\n=== Performance: Single Lookup ===\n";
    std::cout << "Iterations: " << iterations << "\n";
    std::cout << "Average: " << std::fixed << std::setprecision(2) << avg_ns << " ns\n";
    std::cout << "Target: < 10 ns\n";

    // Should be < 10 ns (essentially array access)
    EXPECT_LT(avg_ns, 10.0f) << "Single lookup should be < 10 ns";
}

TEST_F(ColorTransformTest, Performance_ColumnTransformation) {
    ColorTransform transformer(ColorTheme::CMRMAP);

    const size_t height = 1080;
    const int iterations = 10000;

    std::vector<float> input(height, 0.5f);
    std::vector<uint32_t> output(height);

    auto start = std::chrono::high_resolution_clock::now();

    for (int i = 0; i < iterations; ++i) {
        transformer.transformColumn(input.data(), height, output.data());
    }

    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);

    float avg_us = duration.count() / static_cast<float>(iterations);

    std::cout << "\n=== Performance: Column Transformation (1080 pixels) ===\n";
    std::cout << "Iterations: " << iterations << "\n";
    std::cout << "Average: " << std::fixed << std::setprecision(2) << avg_us << " μs\n";
    std::cout << "Target: < 1 μs\n";

    // Should be < 1 μs for 1080 pixels
    EXPECT_LT(avg_us, 1.0f) << "Column transformation should be < 1 μs";
}

TEST_F(ColorTransformTest, Performance_Throughput) {
    ColorTransform transformer(ColorTheme::CMRMAP);

    const size_t height = 1080;
    const int columns = 10000;

    std::vector<float> input(height);
    std::vector<uint32_t> output(height);

    // Create gradient
    for (size_t i = 0; i < height; ++i) {
        input[i] = i / static_cast<float>(height - 1);
    }

    auto start = std::chrono::high_resolution_clock::now();

    for (int c = 0; c < columns; ++c) {
        transformer.transformColumn(input.data(), height, output.data());
    }

    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);

    float columns_per_sec = (columns * 1000.0f) / duration.count();
    float mpixels_per_sec = (columns * height * 1.0f) / (duration.count() * 1000.0f);

    std::cout << "\n=== Performance: Throughput ===\n";
    std::cout << "Processed: " << columns << " columns × " << height << " pixels\n";
    std::cout << "Time: " << duration.count() << " ms\n";
    std::cout << "Throughput: " << std::fixed << std::setprecision(1)
              << columns_per_sec << " columns/sec\n";
    std::cout << "Throughput: " << std::fixed << std::setprecision(1)
              << mpixels_per_sec << " Mpixels/sec\n";

    // Should handle > 1000 columns/sec (1080p @ 60 FPS needs ~60 columns/sec)
    EXPECT_GT(columns_per_sec, 1000.0f)
        << "Should process > 1000 columns/sec";
}

// ============================================================================
// Main
// ============================================================================

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
