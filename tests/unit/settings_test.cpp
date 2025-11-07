/**
 * @file settings_test.cpp
 * @brief Unit tests for SpectrogramSettings
 *
 * Tests cover:
 * - Default values
 * - Validation methods
 * - Setters with valid/invalid inputs
 * - Edge cases
 * - Helper methods
 */

#include <gtest/gtest.h>
#include <friture/settings.hpp>
#include <friture/types.hpp>

using namespace friture;

// ============================================================================
// Default Values Tests
// ============================================================================

TEST(SpectrogramSettingsTest, DefaultValues) {
    SpectrogramSettings settings;

    EXPECT_EQ(settings.fft_size, 4096);
    EXPECT_EQ(settings.window_type, WindowFunction::Hann);
    EXPECT_EQ(settings.freq_scale, FrequencyScale::Mel);
    EXPECT_FLOAT_EQ(settings.min_freq, 20.0f);
    EXPECT_FLOAT_EQ(settings.max_freq, 24000.0f);
    EXPECT_FLOAT_EQ(settings.spec_min_db, -140.0f);
    EXPECT_FLOAT_EQ(settings.spec_max_db, 0.0f);
    EXPECT_FLOAT_EQ(settings.time_range, 10.0f);
    EXPECT_EQ(settings.weighting, WeightingType::None);
    EXPECT_FLOAT_EQ(settings.sample_rate, 48000.0f);
}

TEST(SpectrogramSettingsTest, DefaultsAreValid) {
    SpectrogramSettings settings;
    EXPECT_TRUE(settings.isValid());
}

// ============================================================================
// FFT Size Validation Tests
// ============================================================================

TEST(SpectrogramSettingsTest, ValidFFTSizes) {
    SpectrogramSettings settings;

    // Test all valid FFT sizes
    std::vector<size_t> valid_sizes = {32, 64, 128, 256, 512, 1024, 2048, 4096, 8192, 16384};

    for (size_t size : valid_sizes) {
        EXPECT_TRUE(settings.setFFTSize(size)) << "Size " << size << " should be valid";
        EXPECT_EQ(settings.fft_size, size);
        EXPECT_TRUE(settings.isValid());
    }
}

TEST(SpectrogramSettingsTest, InvalidFFTSizes) {
    SpectrogramSettings settings;

    // Test invalid FFT sizes
    std::vector<size_t> invalid_sizes = {
        0, 1, 16, 31, 33, 100, 1000, 3000, 5000, 32768, 65536
    };

    for (size_t size : invalid_sizes) {
        size_t original = settings.fft_size;
        EXPECT_FALSE(settings.setFFTSize(size)) << "Size " << size << " should be invalid";
        EXPECT_EQ(settings.fft_size, original) << "FFT size should not change on invalid input";
    }
}

// ============================================================================
// Frequency Range Validation Tests
// ============================================================================

TEST(SpectrogramSettingsTest, ValidFrequencyRanges) {
    SpectrogramSettings settings;

    EXPECT_TRUE(settings.setFrequencyRange(20.0f, 20000.0f));
    EXPECT_FLOAT_EQ(settings.min_freq, 20.0f);
    EXPECT_FLOAT_EQ(settings.max_freq, 20000.0f);

    EXPECT_TRUE(settings.setFrequencyRange(50.0f, 15000.0f));
    EXPECT_FLOAT_EQ(settings.min_freq, 50.0f);
    EXPECT_FLOAT_EQ(settings.max_freq, 15000.0f);

    EXPECT_TRUE(settings.setFrequencyRange(10.0f, 24000.0f));
    EXPECT_FLOAT_EQ(settings.min_freq, 10.0f);
    EXPECT_FLOAT_EQ(settings.max_freq, 24000.0f);
}

TEST(SpectrogramSettingsTest, InvalidFrequencyRanges) {
    SpectrogramSettings settings;

    // Min >= Max
    EXPECT_FALSE(settings.setFrequencyRange(1000.0f, 1000.0f));
    EXPECT_FALSE(settings.setFrequencyRange(1000.0f, 500.0f));

    // Negative or zero frequencies
    EXPECT_FALSE(settings.setFrequencyRange(-100.0f, 1000.0f));
    EXPECT_FALSE(settings.setFrequencyRange(0.0f, 1000.0f));
    EXPECT_FALSE(settings.setFrequencyRange(100.0f, 0.0f));

    // Exceeds Nyquist (48000 / 2 = 24000 Hz)
    EXPECT_FALSE(settings.setFrequencyRange(100.0f, 25000.0f));
    EXPECT_FALSE(settings.setFrequencyRange(100.0f, 30000.0f));
}

TEST(SpectrogramSettingsTest, FrequencyRangeWithDifferentSampleRates) {
    SpectrogramSettings settings;

    // Set sample rate to 44100 Hz (Nyquist = 22050 Hz)
    settings.setSampleRate(44100.0f);

    // This should work (below Nyquist)
    EXPECT_TRUE(settings.setFrequencyRange(20.0f, 20000.0f));

    // This should fail (exceeds Nyquist)
    EXPECT_FALSE(settings.setFrequencyRange(20.0f, 24000.0f));

    // Set higher sample rate (96000 Hz, Nyquist = 48000 Hz)
    settings.setSampleRate(96000.0f);

    // Now this should work
    EXPECT_TRUE(settings.setFrequencyRange(20.0f, 40000.0f));
}

// ============================================================================
// Amplitude Range Validation Tests
// ============================================================================

TEST(SpectrogramSettingsTest, ValidAmplitudeRanges) {
    SpectrogramSettings settings;

    EXPECT_TRUE(settings.setAmplitudeRange(-140.0f, 0.0f));
    EXPECT_FLOAT_EQ(settings.spec_min_db, -140.0f);
    EXPECT_FLOAT_EQ(settings.spec_max_db, 0.0f);

    EXPECT_TRUE(settings.setAmplitudeRange(-200.0f, 200.0f));
    EXPECT_FLOAT_EQ(settings.spec_min_db, -200.0f);
    EXPECT_FLOAT_EQ(settings.spec_max_db, 200.0f);

    EXPECT_TRUE(settings.setAmplitudeRange(-80.0f, -20.0f));
    EXPECT_FLOAT_EQ(settings.spec_min_db, -80.0f);
    EXPECT_FLOAT_EQ(settings.spec_max_db, -20.0f);
}

TEST(SpectrogramSettingsTest, InvalidAmplitudeRanges) {
    SpectrogramSettings settings;

    // Min >= Max
    EXPECT_FALSE(settings.setAmplitudeRange(-60.0f, -60.0f));
    EXPECT_FALSE(settings.setAmplitudeRange(-60.0f, -80.0f));

    // Out of range [-200, +200]
    EXPECT_FALSE(settings.setAmplitudeRange(-250.0f, 0.0f));
    EXPECT_FALSE(settings.setAmplitudeRange(-100.0f, 250.0f));
}

// ============================================================================
// Time Range Validation Tests
// ============================================================================

TEST(SpectrogramSettingsTest, ValidTimeRanges) {
    SpectrogramSettings settings;

    EXPECT_TRUE(settings.setTimeRange(0.1f));
    EXPECT_FLOAT_EQ(settings.time_range, 0.1f);

    EXPECT_TRUE(settings.setTimeRange(10.0f));
    EXPECT_FLOAT_EQ(settings.time_range, 10.0f);

    EXPECT_TRUE(settings.setTimeRange(1000.0f));
    EXPECT_FLOAT_EQ(settings.time_range, 1000.0f);
}

TEST(SpectrogramSettingsTest, InvalidTimeRanges) {
    SpectrogramSettings settings;

    // Zero or negative
    EXPECT_FALSE(settings.setTimeRange(0.0f));
    EXPECT_FALSE(settings.setTimeRange(-1.0f));

    // Too large
    EXPECT_FALSE(settings.setTimeRange(1001.0f));
    EXPECT_FALSE(settings.setTimeRange(10000.0f));

    // Below minimum
    EXPECT_FALSE(settings.setTimeRange(0.05f));
}

// ============================================================================
// Sample Rate Tests
// ============================================================================

TEST(SpectrogramSettingsTest, ValidSampleRates) {
    SpectrogramSettings settings;

    EXPECT_TRUE(settings.setSampleRate(44100.0f));
    EXPECT_FLOAT_EQ(settings.sample_rate, 44100.0f);

    EXPECT_TRUE(settings.setSampleRate(48000.0f));
    EXPECT_FLOAT_EQ(settings.sample_rate, 48000.0f);

    EXPECT_TRUE(settings.setSampleRate(96000.0f));
    EXPECT_FLOAT_EQ(settings.sample_rate, 96000.0f);
}

TEST(SpectrogramSettingsTest, InvalidSampleRates) {
    SpectrogramSettings settings;

    EXPECT_FALSE(settings.setSampleRate(0.0f));
    EXPECT_FALSE(settings.setSampleRate(-1000.0f));
}

TEST(SpectrogramSettingsTest, SampleRateAdjustsMaxFrequency) {
    SpectrogramSettings settings;

    // Set max frequency to 24000 Hz (valid for 48000 Hz sample rate)
    settings.setFrequencyRange(20.0f, 24000.0f);
    EXPECT_FLOAT_EQ(settings.max_freq, 24000.0f);

    // Lower sample rate to 44100 Hz (Nyquist = 22050 Hz)
    // max_freq should be automatically adjusted
    settings.setSampleRate(44100.0f);
    EXPECT_LE(settings.max_freq, 22050.0f);
}

// ============================================================================
// Helper Methods Tests
// ============================================================================

TEST(SpectrogramSettingsTest, GetNyquistFrequency) {
    SpectrogramSettings settings;

    settings.setSampleRate(48000.0f);
    EXPECT_FLOAT_EQ(settings.getNyquistFrequency(), 24000.0f);

    settings.setSampleRate(44100.0f);
    EXPECT_FLOAT_EQ(settings.getNyquistFrequency(), 22050.0f);

    settings.setSampleRate(96000.0f);
    EXPECT_FLOAT_EQ(settings.getNyquistFrequency(), 48000.0f);
}

TEST(SpectrogramSettingsTest, GetSamplesPerColumn) {
    SpectrogramSettings settings;

    // FFT size 4096, 75% overlap = 25% hop = 1024 samples
    settings.setFFTSize(4096);
    EXPECT_EQ(settings.getSamplesPerColumn(), 1024);

    // FFT size 2048, 75% overlap = 25% hop = 512 samples
    settings.setFFTSize(2048);
    EXPECT_EQ(settings.getSamplesPerColumn(), 512);

    // FFT size 8192, 75% overlap = 25% hop = 2048 samples
    settings.setFFTSize(8192);
    EXPECT_EQ(settings.getSamplesPerColumn(), 2048);
}

TEST(SpectrogramSettingsTest, GetTimePerColumn) {
    SpectrogramSettings settings;

    settings.setSampleRate(48000.0f);
    settings.setFFTSize(4096);

    // 1024 samples at 48000 Hz = 21.333... ms
    float expected_time = 1024.0f / 48000.0f;
    EXPECT_NEAR(settings.getTimePerColumn(), expected_time, 1e-6);

    settings.setFFTSize(2048);
    // 512 samples at 48000 Hz = 10.666... ms
    expected_time = 512.0f / 48000.0f;
    EXPECT_NEAR(settings.getTimePerColumn(), expected_time, 1e-6);
}

// ============================================================================
// Overall Validation Tests
// ============================================================================

TEST(SpectrogramSettingsTest, ValidationDetectsInvalidFFTSize) {
    SpectrogramSettings settings;

    // Force invalid FFT size (bypassing setter)
    settings.fft_size = 100;
    EXPECT_FALSE(settings.isValid());
}

TEST(SpectrogramSettingsTest, ValidationDetectsInvalidFrequencyRange) {
    SpectrogramSettings settings;

    // Min >= Max
    settings.min_freq = 1000.0f;
    settings.max_freq = 500.0f;
    EXPECT_FALSE(settings.isValid());

    // Exceeds Nyquist
    settings.min_freq = 100.0f;
    settings.max_freq = 30000.0f;  // Exceeds 24000 Hz Nyquist
    EXPECT_FALSE(settings.isValid());
}

TEST(SpectrogramSettingsTest, ValidationDetectsInvalidAmplitudeRange) {
    SpectrogramSettings settings;

    // Min >= Max
    settings.spec_min_db = -50.0f;
    settings.spec_max_db = -100.0f;
    EXPECT_FALSE(settings.isValid());

    // Out of range
    settings.spec_min_db = -250.0f;
    settings.spec_max_db = 0.0f;
    EXPECT_FALSE(settings.isValid());
}

TEST(SpectrogramSettingsTest, ValidationDetectsInvalidTimeRange) {
    SpectrogramSettings settings;

    settings.time_range = -1.0f;
    EXPECT_FALSE(settings.isValid());

    settings.time_range = 0.0f;
    EXPECT_FALSE(settings.isValid());

    settings.time_range = 2000.0f;
    EXPECT_FALSE(settings.isValid());
}

// ============================================================================
// Type toString Tests
// ============================================================================

TEST(TypesTest, WindowFunctionToString) {
    EXPECT_STREQ(toString(WindowFunction::Hann), "Hann");
    EXPECT_STREQ(toString(WindowFunction::Hamming), "Hamming");
}

TEST(TypesTest, FrequencyScaleToString) {
    EXPECT_STREQ(toString(FrequencyScale::Linear), "Linear");
    EXPECT_STREQ(toString(FrequencyScale::Logarithmic), "Logarithmic");
    EXPECT_STREQ(toString(FrequencyScale::Mel), "Mel");
    EXPECT_STREQ(toString(FrequencyScale::ERB), "ERB");
    EXPECT_STREQ(toString(FrequencyScale::Octave), "Octave");
}

TEST(TypesTest, WeightingTypeToString) {
    EXPECT_STREQ(toString(WeightingType::None), "None");
    EXPECT_STREQ(toString(WeightingType::A), "A-weighting");
    EXPECT_STREQ(toString(WeightingType::B), "B-weighting");
    EXPECT_STREQ(toString(WeightingType::C), "C-weighting");
}

// ============================================================================
// Integration Tests
// ============================================================================

TEST(SpectrogramSettingsTest, CompleteSettingsConfiguration) {
    SpectrogramSettings settings;

    // Configure all settings
    EXPECT_TRUE(settings.setFFTSize(8192));
    settings.window_type = WindowFunction::Hamming;
    settings.freq_scale = FrequencyScale::Logarithmic;
    EXPECT_TRUE(settings.setFrequencyRange(50.0f, 20000.0f));
    EXPECT_TRUE(settings.setAmplitudeRange(-120.0f, -20.0f));
    EXPECT_TRUE(settings.setTimeRange(5.0f));
    settings.weighting = WeightingType::A;
    EXPECT_TRUE(settings.setSampleRate(44100.0f));

    // Verify all values
    EXPECT_EQ(settings.fft_size, 8192);
    EXPECT_EQ(settings.window_type, WindowFunction::Hamming);
    EXPECT_EQ(settings.freq_scale, FrequencyScale::Logarithmic);
    EXPECT_FLOAT_EQ(settings.min_freq, 50.0f);
    EXPECT_FLOAT_EQ(settings.max_freq, 20000.0f);
    EXPECT_FLOAT_EQ(settings.spec_min_db, -120.0f);
    EXPECT_FLOAT_EQ(settings.spec_max_db, -20.0f);
    EXPECT_FLOAT_EQ(settings.time_range, 5.0f);
    EXPECT_EQ(settings.weighting, WeightingType::A);
    EXPECT_FLOAT_EQ(settings.sample_rate, 44100.0f);

    // Verify overall validity
    EXPECT_TRUE(settings.isValid());
}

// ============================================================================
// Main
// ============================================================================

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
