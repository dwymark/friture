/**
 * @file fft_processor_test.cpp
 * @brief Comprehensive unit tests for FFTProcessor
 *
 * Tests cover:
 * - Window functions (Hann, Hamming)
 * - FFT processing accuracy
 * - Dynamic reconfiguration
 * - Performance benchmarks
 */

#include <gtest/gtest.h>
#include <friture/fft_processor.hpp>
#include <vector>
#include <cmath>
#include <chrono>
#include <iostream>
#include <iomanip>

using namespace friture;

// ============================================================================
// Test Fixtures
// ============================================================================

class FFTProcessorTest : public ::testing::Test {
protected:
    static constexpr float SAMPLE_RATE = 48000.0f;
    static constexpr float PI = 3.14159265358979323846f;

    // Helper to generate sine wave
    std::vector<float> generateSine(size_t length, float freq_hz, float amplitude = 1.0f) {
        std::vector<float> signal(length);
        for (size_t i = 0; i < length; ++i) {
            signal[i] = amplitude * std::sin(2.0f * PI * freq_hz * i / SAMPLE_RATE);
        }
        return signal;
    }

    // Helper to find peak bin
    size_t findPeakBin(const std::vector<float>& spectrum) {
        return std::distance(spectrum.begin(),
                           std::max_element(spectrum.begin(), spectrum.end()));
    }
};

// ============================================================================
// Construction and Configuration Tests
// ============================================================================

TEST_F(FFTProcessorTest, Construction) {
    EXPECT_NO_THROW({
        FFTProcessor processor(4096, WindowFunction::Hann);
    });
}

TEST_F(FFTProcessorTest, InvalidFFTSize) {
    EXPECT_THROW(FFTProcessor(0, WindowFunction::Hann), std::invalid_argument);
    EXPECT_THROW(FFTProcessor(100, WindowFunction::Hann), std::invalid_argument);
    EXPECT_THROW(FFTProcessor(32768, WindowFunction::Hann), std::invalid_argument);
}

TEST_F(FFTProcessorTest, ValidFFTSizes) {
    std::vector<size_t> valid_sizes = {32, 64, 128, 256, 512, 1024, 2048, 4096, 8192, 16384};
    for (size_t size : valid_sizes) {
        EXPECT_NO_THROW(FFTProcessor(size, WindowFunction::Hann));
    }
}

// ============================================================================
// Window Function Tests
// ============================================================================

TEST_F(FFTProcessorTest, HannWindow) {
    FFTProcessor processor(1024, WindowFunction::Hann);

    // Process zero signal
    std::vector<float> zeros(1024, 0.0f);
    std::vector<float> output(513);
    processor.process(zeros.data(), output.data());

    // All outputs should be very low (noise floor)
    for (float val : output) {
        EXPECT_LT(val, -100.0f);
    }
}

TEST_F(FFTProcessorTest, HammingWindow) {
    FFTProcessor processor(1024, WindowFunction::Hamming);

    std::vector<float> zeros(1024, 0.0f);
    std::vector<float> output(513);
    processor.process(zeros.data(), output.data());

    for (float val : output) {
        EXPECT_LT(val, -100.0f);
    }
}

// ============================================================================
// FFT Accuracy Tests
// ============================================================================

TEST_F(FFTProcessorTest, SineWave1kHz) {
    FFTProcessor processor(4096, WindowFunction::Hann);

    auto signal = generateSine(4096, 1000.0f);
    std::vector<float> spectrum(2049);
    processor.process(signal.data(), spectrum.data());

    // Find peak
    size_t peak_bin = findPeakBin(spectrum);
    float peak_freq = peak_bin * SAMPLE_RATE / 4096.0f;

    // Peak should be near 1000 Hz (within a few bins)
    EXPECT_NEAR(peak_freq, 1000.0f, 50.0f);

    // Peak should be strong
    // Note: Hann window has coherent gain of 0.5 (-6 dB loss)
    // For unit amplitude sine, expect peak around -6 to -8 dB
    EXPECT_GT(spectrum[peak_bin], -15.0f);
}

TEST_F(FFTProcessorTest, MultipleFrequencies) {
    FFTProcessor processor(4096, WindowFunction::Hann);

    // Signal with 440 Hz and 880 Hz components
    auto signal1 = generateSine(4096, 440.0f, 0.5f);
    auto signal2 = generateSine(4096, 880.0f, 0.5f);
    std::vector<float> signal(4096);
    for (size_t i = 0; i < 4096; ++i) {
        signal[i] = signal1[i] + signal2[i];
    }

    std::vector<float> spectrum(2049);
    processor.process(signal.data(), spectrum.data());

    // Find expected bins
    size_t bin440 = static_cast<size_t>(440.0f * 4096 / SAMPLE_RATE);
    size_t bin880 = static_cast<size_t>(880.0f * 4096 / SAMPLE_RATE);

    // Both frequencies should have peaks
    // Note: Each component is 0.5 amplitude (-6 dB) + Hann window loss (-6 dB) = -12 dB typical
    EXPECT_GT(spectrum[bin440], -22.0f);
    EXPECT_GT(spectrum[bin880], -22.0f);
}

// ============================================================================
// Dynamic Reconfiguration Tests
// ============================================================================

TEST_F(FFTProcessorTest, ChangeFFTSize) {
    FFTProcessor processor(1024, WindowFunction::Hann);
    EXPECT_EQ(processor.getFFTSize(), 1024);

    processor.setFFTSize(2048);
    EXPECT_EQ(processor.getFFTSize(), 2048);
    EXPECT_EQ(processor.getNumBins(), 1025);

    // Should still work after resize
    std::vector<float> signal(2048, 0.0f);
    std::vector<float> output(1025);
    EXPECT_NO_THROW(processor.process(signal.data(), output.data()));
}

TEST_F(FFTProcessorTest, ChangeWindowFunction) {
    FFTProcessor processor(1024, WindowFunction::Hann);

    processor.setWindowFunction(WindowFunction::Hamming);

    // Should still work
    std::vector<float> signal(1024, 0.0f);
    std::vector<float> output(513);
    EXPECT_NO_THROW(processor.process(signal.data(), output.data()));
}

// ============================================================================
// Performance Tests
// ============================================================================

TEST_F(FFTProcessorTest, Performance4096) {
    FFTProcessor processor(4096, WindowFunction::Hann);

    std::vector<float> signal(4096, 1.0f);
    std::vector<float> output(2049);

    // Warm-up
    for (int i = 0; i < 100; ++i) {
        processor.process(signal.data(), output.data());
    }

    // Benchmark
    const int iterations = 10000;
    auto start = std::chrono::high_resolution_clock::now();

    for (int i = 0; i < iterations; ++i) {
        processor.process(signal.data(), output.data());
    }

    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
    float avg_us = duration.count() / static_cast<float>(iterations);

    std::cout << "\nFFT 4096 average: " << avg_us << " μs (target: <100 μs)\n";
    EXPECT_LT(avg_us, 100.0f);
}

TEST_F(FFTProcessorTest, PerformanceAllSizes) {
    std::vector<size_t> sizes = {512, 1024, 2048, 4096, 8192};
    const int iterations = 10000;

    std::cout << "\n=== FFT Performance ===\n";
    std::cout << std::setw(10) << "Size" << std::setw(15) << "Avg Time (μs)" << "\n";
    std::cout << std::string(25, '-') << "\n";

    for (size_t size : sizes) {
        FFTProcessor processor(size, WindowFunction::Hann);

        std::vector<float> signal(size, 1.0f);
        std::vector<float> output(size / 2 + 1);

        // Warm-up
        for (int i = 0; i < 100; ++i) {
            processor.process(signal.data(), output.data());
        }

        // Benchmark
        auto start = std::chrono::high_resolution_clock::now();
        for (int i = 0; i < iterations; ++i) {
            processor.process(signal.data(), output.data());
        }
        auto end = std::chrono::high_resolution_clock::now();

        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
        float avg_us = duration.count() / static_cast<float>(iterations);

        std::cout << std::setw(10) << size
                  << std::setw(15) << std::fixed << std::setprecision(3) << avg_us
                  << "\n";
    }
    std::cout << "\n";
}
