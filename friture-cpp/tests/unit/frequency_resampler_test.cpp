/**
 * @file frequency_resampler_test.cpp
 * @brief Comprehensive unit tests for FrequencyResampler
 *
 * Tests cover:
 * - All 5 frequency scales (Linear, Mel, ERB, Log, Octave)
 * - Frequency mapping accuracy
 * - Interpolation quality
 * - Dynamic reconfiguration
 * - Performance benchmarks
 * - Edge cases
 * - Headless mapping visualization
 */

#include <gtest/gtest.h>
#include <friture/frequency_resampler.hpp>
#include <vector>
#include <cmath>
#include <chrono>
#include <iostream>
#include <iomanip>

using namespace friture;

// ============================================================================
// Test Fixtures
// ============================================================================

class FrequencyResamplerTest : public ::testing::Test {
protected:
    static constexpr float SAMPLE_RATE = 48000.0f;
    static constexpr size_t FFT_SIZE = 4096;
    static constexpr size_t OUTPUT_HEIGHT = 1080;
    static constexpr float MIN_FREQ = 20.0f;
    static constexpr float MAX_FREQ = 24000.0f;

    // Helper to create flat spectrum
    std::vector<float> createFlatSpectrum(float value) {
        return std::vector<float>(FFT_SIZE / 2 + 1, value);
    }

    // Helper to create impulse at specific frequency
    std::vector<float> createImpulseAt(float freq_hz) {
        std::vector<float> spectrum(FFT_SIZE / 2 + 1, -100.0f);
        size_t bin = static_cast<size_t>(freq_hz * FFT_SIZE / SAMPLE_RATE);
        if (bin < spectrum.size()) {
            spectrum[bin] = 0.0f;  // 0 dB peak
        }
        return spectrum;
    }

    // Helper to print mapping (headless-compatible)
    void printMapping(const FrequencyResampler& resampler, size_t num_samples = 10) {
        const auto& mapping = resampler.getFrequencyMapping();
        size_t step = mapping.size() / num_samples;

        std::cout << "\n=== Frequency Mapping ===\n";
        std::cout << "Scale: " << toString(resampler.getScale()) << "\n";
        std::cout << "Range: " << resampler.getMinFrequency() << " - "
                  << resampler.getMaxFrequency() << " Hz\n";
        std::cout << "Output pixels: " << resampler.getOutputHeight() << "\n\n";
        std::cout << std::setw(8) << "Pixel"
                  << std::setw(12) << "Bin Index"
                  << std::setw(12) << "Freq (Hz)" << "\n";
        std::cout << std::string(32, '-') << "\n";

        for (size_t i = 0; i < mapping.size(); i += step) {
            float bin_idx = mapping[i];
            float freq_hz = bin_idx * SAMPLE_RATE / FFT_SIZE;
            std::cout << std::setw(8) << i
                      << std::setw(12) << std::fixed << std::setprecision(2) << bin_idx
                      << std::setw(12) << std::fixed << std::setprecision(1) << freq_hz
                      << "\n";
        }
        std::cout << "\n";
    }
};

// ============================================================================
// Construction and Validation Tests
// ============================================================================

TEST_F(FrequencyResamplerTest, Construction) {
    EXPECT_NO_THROW({
        FrequencyResampler resampler(
            FrequencyScale::Mel,
            MIN_FREQ, MAX_FREQ,
            SAMPLE_RATE, FFT_SIZE, OUTPUT_HEIGHT
        );
    });
}

TEST_F(FrequencyResamplerTest, InvalidMinFrequency) {
    EXPECT_THROW({
        FrequencyResampler resampler(
            FrequencyScale::Linear,
            -10.0f, MAX_FREQ,  // Invalid: negative frequency
            SAMPLE_RATE, FFT_SIZE, OUTPUT_HEIGHT
        );
    }, std::invalid_argument);

    EXPECT_THROW({
        FrequencyResampler resampler(
            FrequencyScale::Linear,
            0.0f, MAX_FREQ,  // Invalid: zero frequency
            SAMPLE_RATE, FFT_SIZE, OUTPUT_HEIGHT
        );
    }, std::invalid_argument);
}

TEST_F(FrequencyResamplerTest, InvalidMaxFrequency) {
    EXPECT_THROW({
        FrequencyResampler resampler(
            FrequencyScale::Linear,
            MIN_FREQ, MIN_FREQ,  // Invalid: max <= min
            SAMPLE_RATE, FFT_SIZE, OUTPUT_HEIGHT
        );
    }, std::invalid_argument);

    EXPECT_THROW({
        FrequencyResampler resampler(
            FrequencyScale::Linear,
            MIN_FREQ, 30000.0f,  // Invalid: exceeds Nyquist (24000 Hz)
            SAMPLE_RATE, FFT_SIZE, OUTPUT_HEIGHT
        );
    }, std::invalid_argument);
}

TEST_F(FrequencyResamplerTest, InvalidOutputHeight) {
    EXPECT_THROW({
        FrequencyResampler resampler(
            FrequencyScale::Linear,
            MIN_FREQ, MAX_FREQ,
            SAMPLE_RATE, FFT_SIZE, 0  // Invalid: zero height
        );
    }, std::invalid_argument);
}

// ============================================================================
// Linear Scale Tests
// ============================================================================

TEST_F(FrequencyResamplerTest, LinearScaleMapping) {
    FrequencyResampler resampler(
        FrequencyScale::Linear,
        MIN_FREQ, MAX_FREQ,
        SAMPLE_RATE, FFT_SIZE, OUTPUT_HEIGHT
    );

    // Print mapping for manual verification
    printMapping(resampler);

    const auto& mapping = resampler.getFrequencyMapping();

    // First pixel should map near min frequency
    float first_freq = mapping[0] * SAMPLE_RATE / FFT_SIZE;
    EXPECT_NEAR(first_freq, MIN_FREQ, 20.0f);

    // Last pixel should map near max frequency
    float last_freq = mapping[OUTPUT_HEIGHT - 1] * SAMPLE_RATE / FFT_SIZE;
    EXPECT_NEAR(last_freq, MAX_FREQ, 20.0f);

    // Linear scale should have approximately constant spacing
    float prev_freq = mapping[0] * SAMPLE_RATE / FFT_SIZE;
    std::vector<float> deltas;
    for (size_t i = 100; i < OUTPUT_HEIGHT; i += 100) {
        float curr_freq = mapping[i] * SAMPLE_RATE / FFT_SIZE;
        deltas.push_back(curr_freq - prev_freq);
        prev_freq = curr_freq;
    }

    // Check that deltas are roughly constant (within 20% variation)
    float mean_delta = 0.0f;
    for (float d : deltas) mean_delta += d;
    mean_delta /= deltas.size();

    for (float d : deltas) {
        EXPECT_NEAR(d, mean_delta, mean_delta * 0.2f);
    }
}

TEST_F(FrequencyResamplerTest, LinearScaleFlatSpectrum) {
    FrequencyResampler resampler(
        FrequencyScale::Linear,
        MIN_FREQ, MAX_FREQ,
        SAMPLE_RATE, FFT_SIZE, OUTPUT_HEIGHT
    );

    // Create flat spectrum at -60 dB
    auto input = createFlatSpectrum(-60.0f);
    std::vector<float> output(OUTPUT_HEIGHT);

    resampler.resample(input.data(), output.data());

    // Output should also be flat (within interpolation tolerance)
    for (float val : output) {
        EXPECT_NEAR(val, -60.0f, 0.1f);
    }
}

// ============================================================================
// Mel Scale Tests
// ============================================================================

TEST_F(FrequencyResamplerTest, MelScaleMapping) {
    FrequencyResampler resampler(
        FrequencyScale::Mel,
        MIN_FREQ, MAX_FREQ,
        SAMPLE_RATE, FFT_SIZE, OUTPUT_HEIGHT
    );

    printMapping(resampler);

    const auto& mapping = resampler.getFrequencyMapping();

    // Mel scale should have finer resolution at low frequencies
    float low_delta = (mapping[100] - mapping[0]) * SAMPLE_RATE / FFT_SIZE;
    float high_delta = (mapping[OUTPUT_HEIGHT-1] - mapping[OUTPUT_HEIGHT-101]) * SAMPLE_RATE / FFT_SIZE;

    // Low frequency spacing should be smaller than high frequency spacing
    EXPECT_LT(low_delta, high_delta);

    // Check that spacing increases monotonically (roughly)
    float prev_delta = 0.0f;
    for (size_t i = 100; i < OUTPUT_HEIGHT; i += 100) {
        float curr_delta = (mapping[i] - mapping[i-100]) * SAMPLE_RATE / FFT_SIZE;
        if (prev_delta > 0) {
            // Allow some tolerance for numerical variation
            EXPECT_GE(curr_delta, prev_delta * 0.8f);
        }
        prev_delta = curr_delta;
    }
}

TEST_F(FrequencyResamplerTest, MelScaleTransformInverse) {
    // Test that transform and inverse are truly inverse operations
    std::vector<float> test_freqs = {20.0f, 100.0f, 500.0f, 1000.0f, 5000.0f, 10000.0f, 20000.0f};

    for (float freq : test_freqs) {
        float mel = 2595.0f * std::log10(1.0f + freq / 700.0f);
        float freq_recovered = 700.0f * (std::pow(10.0f, mel / 2595.0f) - 1.0f);
        EXPECT_NEAR(freq_recovered, freq, 0.01f);
    }
}

// ============================================================================
// ERB Scale Tests
// ============================================================================

TEST_F(FrequencyResamplerTest, ERBScaleMapping) {
    FrequencyResampler resampler(
        FrequencyScale::ERB,
        MIN_FREQ, MAX_FREQ,
        SAMPLE_RATE, FFT_SIZE, OUTPUT_HEIGHT
    );

    printMapping(resampler);

    const auto& mapping = resampler.getFrequencyMapping();

    // ERB scale should also have finer resolution at low frequencies (like Mel)
    float low_delta = (mapping[100] - mapping[0]) * SAMPLE_RATE / FFT_SIZE;
    float high_delta = (mapping[OUTPUT_HEIGHT-1] - mapping[OUTPUT_HEIGHT-101]) * SAMPLE_RATE / FFT_SIZE;

    EXPECT_LT(low_delta, high_delta);
}

TEST_F(FrequencyResamplerTest, ERBScaleTransformInverse) {
    // Test ERB transform and inverse
    constexpr float ERB_A = 21.33228113095401739888262f;
    std::vector<float> test_freqs = {20.0f, 100.0f, 500.0f, 1000.0f, 5000.0f, 10000.0f, 20000.0f};

    for (float freq : test_freqs) {
        float erb = ERB_A * std::log10(1.0f + 0.00437f * freq);
        float freq_recovered = (std::pow(10.0f, erb / ERB_A) - 1.0f) / 0.00437f;
        EXPECT_NEAR(freq_recovered, freq, 0.01f);
    }
}

// ============================================================================
// Logarithmic Scale Tests
// ============================================================================

TEST_F(FrequencyResamplerTest, LogScaleMapping) {
    FrequencyResampler resampler(
        FrequencyScale::Logarithmic,
        MIN_FREQ, MAX_FREQ,
        SAMPLE_RATE, FFT_SIZE, OUTPUT_HEIGHT
    );

    printMapping(resampler);

    const auto& mapping = resampler.getFrequencyMapping();

    // Log scale: equal ratios should map to equal distances
    // Test by checking that frequency ratios are constant across equal pixel distances

    size_t step = 200;
    std::vector<float> ratios;

    for (size_t i = step; i < OUTPUT_HEIGHT - step; i += step) {
        float freq1 = mapping[i] * SAMPLE_RATE / FFT_SIZE;
        float freq2 = mapping[i + step] * SAMPLE_RATE / FFT_SIZE;
        ratios.push_back(freq2 / freq1);
    }

    // Ratios should be approximately constant
    if (!ratios.empty()) {
        float mean_ratio = 0.0f;
        for (float r : ratios) mean_ratio += r;
        mean_ratio /= ratios.size();

        for (float r : ratios) {
            EXPECT_NEAR(r, mean_ratio, mean_ratio * 0.1f);
        }
    }
}

TEST_F(FrequencyResamplerTest, LogScaleTransformInverse) {
    std::vector<float> test_freqs = {20.0f, 100.0f, 500.0f, 1000.0f, 5000.0f, 10000.0f, 20000.0f};

    for (float freq : test_freqs) {
        float log_val = std::log10(freq);
        float freq_recovered = std::pow(10.0f, log_val);
        EXPECT_NEAR(freq_recovered, freq, 0.01f);
    }
}

// ============================================================================
// Octave Scale Tests
// ============================================================================

TEST_F(FrequencyResamplerTest, OctaveScaleMapping) {
    FrequencyResampler resampler(
        FrequencyScale::Octave,
        MIN_FREQ, MAX_FREQ,
        SAMPLE_RATE, FFT_SIZE, OUTPUT_HEIGHT
    );

    printMapping(resampler);

    const auto& mapping = resampler.getFrequencyMapping();

    // Octave scale: each octave is a doubling of frequency
    // Test that equal pixel distances map to approximately constant frequency ratios

    size_t step = 200;
    std::vector<float> ratios;

    for (size_t i = step; i < OUTPUT_HEIGHT - step; i += step) {
        float freq1 = mapping[i] * SAMPLE_RATE / FFT_SIZE;
        float freq2 = mapping[i + step] * SAMPLE_RATE / FFT_SIZE;
        ratios.push_back(freq2 / freq1);
    }

    // Ratios should be approximately constant (log2 scale property)
    if (!ratios.empty()) {
        float mean_ratio = 0.0f;
        for (float r : ratios) mean_ratio += r;
        mean_ratio /= ratios.size();

        for (float r : ratios) {
            EXPECT_NEAR(r, mean_ratio, mean_ratio * 0.1f);
        }
    }
}

TEST_F(FrequencyResamplerTest, OctaveScaleTransformInverse) {
    std::vector<float> test_freqs = {20.0f, 40.0f, 80.0f, 160.0f, 320.0f, 640.0f, 1280.0f};

    for (float freq : test_freqs) {
        float octave_val = std::log2(freq);
        float freq_recovered = std::pow(2.0f, octave_val);
        EXPECT_NEAR(freq_recovered, freq, 0.01f);
    }
}

// ============================================================================
// Interpolation Tests
// ============================================================================

TEST_F(FrequencyResamplerTest, InterpolationAccuracy) {
    // Test that interpolation works without crashing for various configurations
    std::vector<size_t> output_heights = {10, 50, 100, 500};

    for (size_t height : output_heights) {
        FrequencyResampler resampler(
            FrequencyScale::Linear,
            MIN_FREQ, MAX_FREQ,
            SAMPLE_RATE, FFT_SIZE, height
        );

        auto input = createFlatSpectrum(-60.0f);
        std::vector<float> output(height);

        EXPECT_NO_THROW(resampler.resample(input.data(), output.data()));

        // Output should be reasonable (not NaN, not Inf)
        for (float val : output) {
            EXPECT_FALSE(std::isnan(val));
            EXPECT_FALSE(std::isinf(val));
            EXPECT_NEAR(val, -60.0f, 1.0f);  // Should preserve flat spectrum
        }
    }
}

TEST_F(FrequencyResamplerTest, FlatSpectrumAllScales) {
    std::vector<FrequencyScale> scales = {
        FrequencyScale::Linear,
        FrequencyScale::Mel,
        FrequencyScale::ERB,
        FrequencyScale::Logarithmic,
        FrequencyScale::Octave
    };

    auto input = createFlatSpectrum(-60.0f);
    std::vector<float> output(OUTPUT_HEIGHT);

    for (auto scale : scales) {
        FrequencyResampler resampler(
            scale, MIN_FREQ, MAX_FREQ,
            SAMPLE_RATE, FFT_SIZE, OUTPUT_HEIGHT
        );

        resampler.resample(input.data(), output.data());

        // All scales should preserve flat spectrum
        for (float val : output) {
            EXPECT_NEAR(val, -60.0f, 0.5f)
                << "Scale: " << toString(scale);
        }
    }
}

// ============================================================================
// Dynamic Reconfiguration Tests
// ============================================================================

TEST_F(FrequencyResamplerTest, ChangeScale) {
    FrequencyResampler resampler(
        FrequencyScale::Linear,
        MIN_FREQ, MAX_FREQ,
        SAMPLE_RATE, FFT_SIZE, OUTPUT_HEIGHT
    );

    EXPECT_EQ(resampler.getScale(), FrequencyScale::Linear);

    // Change to Mel
    EXPECT_NO_THROW(resampler.setScale(FrequencyScale::Mel));
    EXPECT_EQ(resampler.getScale(), FrequencyScale::Mel);

    // Should still work after scale change
    auto input = createFlatSpectrum(-60.0f);
    std::vector<float> output(OUTPUT_HEIGHT);
    EXPECT_NO_THROW(resampler.resample(input.data(), output.data()));
}

TEST_F(FrequencyResamplerTest, ChangeFrequencyRange) {
    FrequencyResampler resampler(
        FrequencyScale::Mel,
        MIN_FREQ, MAX_FREQ,
        SAMPLE_RATE, FFT_SIZE, OUTPUT_HEIGHT
    );

    EXPECT_NO_THROW(resampler.setFrequencyRange(50.0f, 10000.0f));
    EXPECT_FLOAT_EQ(resampler.getMinFrequency(), 50.0f);
    EXPECT_FLOAT_EQ(resampler.getMaxFrequency(), 10000.0f);

    // Should reject invalid ranges
    EXPECT_THROW(resampler.setFrequencyRange(100.0f, 50.0f), std::invalid_argument);
    EXPECT_THROW(resampler.setFrequencyRange(-10.0f, 10000.0f), std::invalid_argument);
    EXPECT_THROW(resampler.setFrequencyRange(100.0f, 30000.0f), std::invalid_argument);
}

TEST_F(FrequencyResamplerTest, ChangeOutputHeight) {
    FrequencyResampler resampler(
        FrequencyScale::Mel,
        MIN_FREQ, MAX_FREQ,
        SAMPLE_RATE, FFT_SIZE, OUTPUT_HEIGHT
    );

    EXPECT_EQ(resampler.getOutputHeight(), OUTPUT_HEIGHT);

    EXPECT_NO_THROW(resampler.setOutputHeight(720));
    EXPECT_EQ(resampler.getOutputHeight(), 720);
    EXPECT_EQ(resampler.getFrequencyMapping().size(), 720);

    EXPECT_THROW(resampler.setOutputHeight(0), std::invalid_argument);
}

// ============================================================================
// Edge Cases
// ============================================================================

TEST_F(FrequencyResamplerTest, VeryNarrowFrequencyRange) {
    FrequencyResampler resampler(
        FrequencyScale::Linear,
        1000.0f, 1100.0f,  // Only 100 Hz range
        SAMPLE_RATE, FFT_SIZE, 100
    );

    auto input = createFlatSpectrum(-60.0f);
    std::vector<float> output(100);

    EXPECT_NO_THROW(resampler.resample(input.data(), output.data()));
}

TEST_F(FrequencyResamplerTest, VeryLowFrequencyRange) {
    FrequencyResampler resampler(
        FrequencyScale::Linear,
        1.0f, 100.0f,  // Very low frequencies
        SAMPLE_RATE, FFT_SIZE, 100
    );

    auto input = createFlatSpectrum(-60.0f);
    std::vector<float> output(100);

    EXPECT_NO_THROW(resampler.resample(input.data(), output.data()));
}

TEST_F(FrequencyResamplerTest, SmallOutputHeight) {
    FrequencyResampler resampler(
        FrequencyScale::Mel,
        MIN_FREQ, MAX_FREQ,
        SAMPLE_RATE, FFT_SIZE, 10  // Only 10 pixels
    );

    auto input = createFlatSpectrum(-60.0f);
    std::vector<float> output(10);

    EXPECT_NO_THROW(resampler.resample(input.data(), output.data()));
}

// ============================================================================
// Performance Benchmarks
// ============================================================================

TEST_F(FrequencyResamplerTest, ResamplePerformance) {
    FrequencyResampler resampler(
        FrequencyScale::Mel,
        MIN_FREQ, MAX_FREQ,
        SAMPLE_RATE, FFT_SIZE, OUTPUT_HEIGHT
    );

    auto input = createFlatSpectrum(-60.0f);
    std::vector<float> output(OUTPUT_HEIGHT);

    // Warm-up
    for (int i = 0; i < 100; ++i) {
        resampler.resample(input.data(), output.data());
    }

    // Benchmark
    const int iterations = 10000;
    auto start = std::chrono::high_resolution_clock::now();

    for (int i = 0; i < iterations; ++i) {
        resampler.resample(input.data(), output.data());
    }

    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
    float avg_us = duration.count() / static_cast<float>(iterations);

    std::cout << "\n=== Performance Benchmark ===\n";
    std::cout << "Average resample time: " << avg_us << " μs\n";
    std::cout << "Target: <10 μs\n";
    std::cout << "Status: " << (avg_us < 10.0f ? "PASS ✓" : "NEEDS OPTIMIZATION") << "\n\n";

    // Should be under 10 μs for 2049 bins → 1080 pixels
    EXPECT_LT(avg_us, 10.0f);
}

TEST_F(FrequencyResamplerTest, PerformanceAllScales) {
    std::vector<FrequencyScale> scales = {
        FrequencyScale::Linear,
        FrequencyScale::Mel,
        FrequencyScale::ERB,
        FrequencyScale::Logarithmic,
        FrequencyScale::Octave
    };

    auto input = createFlatSpectrum(-60.0f);
    std::vector<float> output(OUTPUT_HEIGHT);
    const int iterations = 10000;

    std::cout << "\n=== Performance by Scale ===\n";
    std::cout << std::setw(15) << "Scale" << std::setw(15) << "Avg Time (μs)" << "\n";
    std::cout << std::string(30, '-') << "\n";

    for (auto scale : scales) {
        FrequencyResampler resampler(
            scale, MIN_FREQ, MAX_FREQ,
            SAMPLE_RATE, FFT_SIZE, OUTPUT_HEIGHT
        );

        // Warm-up
        for (int i = 0; i < 100; ++i) {
            resampler.resample(input.data(), output.data());
        }

        // Benchmark
        auto start = std::chrono::high_resolution_clock::now();
        for (int i = 0; i < iterations; ++i) {
            resampler.resample(input.data(), output.data());
        }
        auto end = std::chrono::high_resolution_clock::now();

        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
        float avg_us = duration.count() / static_cast<float>(iterations);

        std::cout << std::setw(15) << toString(scale)
                  << std::setw(15) << std::fixed << std::setprecision(3) << avg_us
                  << "\n";

        EXPECT_LT(avg_us, 10.0f) << "Scale: " << toString(scale);
    }
    std::cout << "\n";
}

// ============================================================================
// Mapping Visualization (Headless)
// ============================================================================

TEST_F(FrequencyResamplerTest, VisualizeAllScaleMappings) {
    std::vector<FrequencyScale> scales = {
        FrequencyScale::Linear,
        FrequencyScale::Mel,
        FrequencyScale::ERB,
        FrequencyScale::Logarithmic,
        FrequencyScale::Octave
    };

    std::cout << "\n========================================\n";
    std::cout << "FREQUENCY MAPPING VISUALIZATION\n";
    std::cout << "========================================\n";

    for (auto scale : scales) {
        FrequencyResampler resampler(
            scale, MIN_FREQ, MAX_FREQ,
            SAMPLE_RATE, FFT_SIZE, OUTPUT_HEIGHT
        );

        printMapping(resampler, 15);  // Print 15 sample points
    }
}
