/**
 * @file audio_file_loader_test.cpp
 * @brief Unit tests for AudioFileLoader
 */

#define _USE_MATH_DEFINES  // For M_PI on Windows/MSVC
#include <friture/audio/audio_file_loader.hpp>
#include <gtest/gtest.h>
#include <cstdio>
#include <cstring>
#include <cmath>

using namespace friture;

// ============================================================================
// WAV File Generation Helpers
// ============================================================================

/**
 * @brief Write a simple WAV file header
 */
class WavWriter {
public:
    static bool writePCM16Mono(const char* filename, const std::vector<float>& samples,
                               uint32_t sample_rate) {
        FILE* fp = fopen(filename, "wb");
        if (!fp) return false;

        uint32_t num_samples = static_cast<uint32_t>(samples.size());
        uint32_t data_size = num_samples * 2; // 16-bit = 2 bytes per sample
        uint32_t file_size = 36 + data_size;

        // RIFF header
        fwrite("RIFF", 1, 4, fp);
        fwrite(&file_size, 4, 1, fp);
        fwrite("WAVE", 1, 4, fp);

        // fmt chunk
        fwrite("fmt ", 1, 4, fp);
        uint32_t fmt_size = 16;
        fwrite(&fmt_size, 4, 1, fp);
        uint16_t audio_format = 1; // PCM
        uint16_t num_channels = 1; // Mono
        uint16_t bits_per_sample = 16;
        uint32_t byte_rate = sample_rate * num_channels * bits_per_sample / 8;
        uint16_t block_align = num_channels * bits_per_sample / 8;

        fwrite(&audio_format, 2, 1, fp);
        fwrite(&num_channels, 2, 1, fp);
        fwrite(&sample_rate, 4, 1, fp);
        fwrite(&byte_rate, 4, 1, fp);
        fwrite(&block_align, 2, 1, fp);
        fwrite(&bits_per_sample, 2, 1, fp);

        // data chunk
        fwrite("data", 1, 4, fp);
        fwrite(&data_size, 4, 1, fp);

        // Write audio data
        for (float sample : samples) {
            int16_t pcm_sample = static_cast<int16_t>(sample * 32767.0f);
            fwrite(&pcm_sample, 2, 1, fp);
        }

        fclose(fp);
        return true;
    }

    static bool writePCM16Stereo(const char* filename, const std::vector<float>& samples,
                                 uint32_t sample_rate) {
        FILE* fp = fopen(filename, "wb");
        if (!fp) return false;

        uint32_t num_frames = static_cast<uint32_t>(samples.size() / 2);
        uint32_t data_size = num_frames * 2 * 2; // 2 channels * 2 bytes
        uint32_t file_size = 36 + data_size;

        // RIFF header
        fwrite("RIFF", 1, 4, fp);
        fwrite(&file_size, 4, 1, fp);
        fwrite("WAVE", 1, 4, fp);

        // fmt chunk
        fwrite("fmt ", 1, 4, fp);
        uint32_t fmt_size = 16;
        fwrite(&fmt_size, 4, 1, fp);
        uint16_t audio_format = 1; // PCM
        uint16_t num_channels = 2; // Stereo
        uint16_t bits_per_sample = 16;
        uint32_t byte_rate = sample_rate * num_channels * bits_per_sample / 8;
        uint16_t block_align = num_channels * bits_per_sample / 8;

        fwrite(&audio_format, 2, 1, fp);
        fwrite(&num_channels, 2, 1, fp);
        fwrite(&sample_rate, 4, 1, fp);
        fwrite(&byte_rate, 4, 1, fp);
        fwrite(&block_align, 2, 1, fp);
        fwrite(&bits_per_sample, 2, 1, fp);

        // data chunk
        fwrite("data", 1, 4, fp);
        fwrite(&data_size, 4, 1, fp);

        // Write audio data (interleaved L, R)
        for (float sample : samples) {
            int16_t pcm_sample = static_cast<int16_t>(sample * 32767.0f);
            fwrite(&pcm_sample, 2, 1, fp);
        }

        fclose(fp);
        return true;
    }

    static bool writeFloat32Mono(const char* filename, const std::vector<float>& samples,
                                 uint32_t sample_rate) {
        FILE* fp = fopen(filename, "wb");
        if (!fp) return false;

        uint32_t num_samples = static_cast<uint32_t>(samples.size());
        uint32_t data_size = num_samples * 4; // float32 = 4 bytes
        uint32_t file_size = 36 + data_size;

        // RIFF header
        fwrite("RIFF", 1, 4, fp);
        fwrite(&file_size, 4, 1, fp);
        fwrite("WAVE", 1, 4, fp);

        // fmt chunk
        fwrite("fmt ", 1, 4, fp);
        uint32_t fmt_size = 16;
        fwrite(&fmt_size, 4, 1, fp);
        uint16_t audio_format = 3; // IEEE Float
        uint16_t num_channels = 1; // Mono
        uint16_t bits_per_sample = 32;
        uint32_t byte_rate = sample_rate * num_channels * bits_per_sample / 8;
        uint16_t block_align = num_channels * bits_per_sample / 8;

        fwrite(&audio_format, 2, 1, fp);
        fwrite(&num_channels, 2, 1, fp);
        fwrite(&sample_rate, 4, 1, fp);
        fwrite(&byte_rate, 4, 1, fp);
        fwrite(&block_align, 2, 1, fp);
        fwrite(&bits_per_sample, 2, 1, fp);

        // data chunk
        fwrite("data", 1, 4, fp);
        fwrite(&data_size, 4, 1, fp);

        // Write audio data
        fwrite(samples.data(), sizeof(float), samples.size(), fp);

        fclose(fp);
        return true;
    }

    static bool writePCM24Mono(const char* filename, const std::vector<float>& samples,
                               uint32_t sample_rate) {
        FILE* fp = fopen(filename, "wb");
        if (!fp) return false;

        uint32_t num_samples = static_cast<uint32_t>(samples.size());
        uint32_t data_size = num_samples * 3; // 24-bit = 3 bytes
        uint32_t file_size = 36 + data_size;

        // RIFF header
        fwrite("RIFF", 1, 4, fp);
        fwrite(&file_size, 4, 1, fp);
        fwrite("WAVE", 1, 4, fp);

        // fmt chunk
        fwrite("fmt ", 1, 4, fp);
        uint32_t fmt_size = 16;
        fwrite(&fmt_size, 4, 1, fp);
        uint16_t audio_format = 1; // PCM
        uint16_t num_channels = 1; // Mono
        uint16_t bits_per_sample = 24;
        uint32_t byte_rate = sample_rate * num_channels * bits_per_sample / 8;
        uint16_t block_align = num_channels * bits_per_sample / 8;

        fwrite(&audio_format, 2, 1, fp);
        fwrite(&num_channels, 2, 1, fp);
        fwrite(&sample_rate, 4, 1, fp);
        fwrite(&byte_rate, 4, 1, fp);
        fwrite(&block_align, 2, 1, fp);
        fwrite(&bits_per_sample, 2, 1, fp);

        // data chunk
        fwrite("data", 1, 4, fp);
        fwrite(&data_size, 4, 1, fp);

        // Write audio data (24-bit PCM)
        for (float sample : samples) {
            int32_t pcm_sample = static_cast<int32_t>(sample * 8388607.0f);
            uint8_t bytes[3];
            bytes[0] = static_cast<uint8_t>(pcm_sample & 0xFF);
            bytes[1] = static_cast<uint8_t>((pcm_sample >> 8) & 0xFF);
            bytes[2] = static_cast<uint8_t>((pcm_sample >> 16) & 0xFF);
            fwrite(bytes, 1, 3, fp);
        }

        fclose(fp);
        return true;
    }
};

// ============================================================================
// Test Fixtures
// ============================================================================

class AudioFileLoaderTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Generate test signals
        generateSineWave(1000.0f, 48000, 0.5f, sine_wave_);
    }

    void TearDown() override {
        // Clean up test files
        remove("/tmp/test_pcm16_mono.wav");
        remove("/tmp/test_pcm16_stereo.wav");
        remove("/tmp/test_pcm24_mono.wav");
        remove("/tmp/test_float32_mono.wav");
        remove("/tmp/test_invalid.wav");
    }

    void generateSineWave(float frequency, uint32_t sample_rate,
                          float duration, std::vector<float>& output) {
        size_t num_samples = static_cast<size_t>(duration * sample_rate);
        output.resize(num_samples);

        for (size_t i = 0; i < num_samples; ++i) {
            float t = static_cast<float>(i) / sample_rate;
            output[i] = 0.5f * std::sin(2.0f * M_PI * frequency * t);
        }
    }

    std::vector<float> sine_wave_;
};

// ============================================================================
// Basic Functionality Tests
// ============================================================================

TEST_F(AudioFileLoaderTest, LoadPCM16Mono) {
    // Create test file
    ASSERT_TRUE(WavWriter::writePCM16Mono("/tmp/test_pcm16_mono.wav", sine_wave_, 48000));

    // Load file
    AudioFileLoader loader;
    std::vector<float> samples;
    float sample_rate;

    EXPECT_TRUE(loader.load("/tmp/test_pcm16_mono.wav", samples, sample_rate));
    EXPECT_FLOAT_EQ(sample_rate, 48000.0f);
    EXPECT_EQ(samples.size(), sine_wave_.size());

    // Check metadata
    const WavInfo& info = loader.getInfo();
    EXPECT_EQ(info.sample_rate, 48000u);
    EXPECT_EQ(info.channels, 1);
    EXPECT_EQ(info.bits_per_sample, 16);
    EXPECT_EQ(info.audio_format, 1); // PCM

    // Verify samples are similar (allow for quantization error)
    for (size_t i = 0; i < std::min(samples.size(), sine_wave_.size()); ++i) {
        EXPECT_NEAR(samples[i], sine_wave_[i], 0.001f) << "at index " << i;
    }
}

TEST_F(AudioFileLoaderTest, LoadPCM16Stereo) {
    // Create stereo test file (duplicate mono to L and R)
    std::vector<float> stereo_data;
    for (float sample : sine_wave_) {
        stereo_data.push_back(sample); // Left
        stereo_data.push_back(sample); // Right
    }

    ASSERT_TRUE(WavWriter::writePCM16Stereo("/tmp/test_pcm16_stereo.wav", stereo_data, 48000));

    // Load file
    AudioFileLoader loader;
    std::vector<float> samples;
    float sample_rate;

    EXPECT_TRUE(loader.load("/tmp/test_pcm16_stereo.wav", samples, sample_rate));
    EXPECT_FLOAT_EQ(sample_rate, 48000.0f);
    EXPECT_EQ(samples.size(), sine_wave_.size()); // Converted to mono

    // Check metadata
    const WavInfo& info = loader.getInfo();
    EXPECT_EQ(info.channels, 2); // Original was stereo
    EXPECT_EQ(info.bits_per_sample, 16);

    // Verify conversion to mono (should average L and R, which are identical)
    for (size_t i = 0; i < std::min(samples.size(), sine_wave_.size()); ++i) {
        EXPECT_NEAR(samples[i], sine_wave_[i], 0.001f);
    }
}

TEST_F(AudioFileLoaderTest, LoadFloat32Mono) {
    // Create test file
    ASSERT_TRUE(WavWriter::writeFloat32Mono("/tmp/test_float32_mono.wav", sine_wave_, 48000));

    // Load file
    AudioFileLoader loader;
    std::vector<float> samples;
    float sample_rate;

    EXPECT_TRUE(loader.load("/tmp/test_float32_mono.wav", samples, sample_rate));
    EXPECT_FLOAT_EQ(sample_rate, 48000.0f);
    EXPECT_EQ(samples.size(), sine_wave_.size());

    // Check metadata
    const WavInfo& info = loader.getInfo();
    EXPECT_EQ(info.audio_format, 3); // IEEE Float

    // Verify samples are exact (no quantization for float)
    for (size_t i = 0; i < std::min(samples.size(), sine_wave_.size()); ++i) {
        EXPECT_FLOAT_EQ(samples[i], sine_wave_[i]);
    }
}

TEST_F(AudioFileLoaderTest, LoadPCM24Mono) {
    // Create test file
    ASSERT_TRUE(WavWriter::writePCM24Mono("/tmp/test_pcm24_mono.wav", sine_wave_, 48000));

    // Load file
    AudioFileLoader loader;
    std::vector<float> samples;
    float sample_rate;

    EXPECT_TRUE(loader.load("/tmp/test_pcm24_mono.wav", samples, sample_rate));
    EXPECT_FLOAT_EQ(sample_rate, 48000.0f);
    EXPECT_EQ(samples.size(), sine_wave_.size());

    // Check metadata
    const WavInfo& info = loader.getInfo();
    EXPECT_EQ(info.bits_per_sample, 24);

    // Verify samples (allow for minimal quantization error)
    for (size_t i = 0; i < std::min(samples.size(), sine_wave_.size()); ++i) {
        EXPECT_NEAR(samples[i], sine_wave_[i], 0.0001f);
    }
}

// ============================================================================
// Error Handling Tests
// ============================================================================

TEST_F(AudioFileLoaderTest, FileNotFound) {
    AudioFileLoader loader;
    std::vector<float> samples;
    float sample_rate;

    EXPECT_FALSE(loader.load("/nonexistent/file.wav", samples, sample_rate));
    EXPECT_FALSE(loader.getError().empty());
}

TEST_F(AudioFileLoaderTest, InvalidFormat) {
    // Create a file that's not a WAV
    FILE* fp = fopen("/tmp/test_invalid.wav", "wb");
    ASSERT_NE(fp, nullptr);
    fwrite("NOT A WAV FILE", 1, 14, fp);
    fclose(fp);

    AudioFileLoader loader;
    std::vector<float> samples;
    float sample_rate;

    EXPECT_FALSE(loader.load("/tmp/test_invalid.wav", samples, sample_rate));
    EXPECT_FALSE(loader.getError().empty());
}

// ============================================================================
// Metadata Tests
// ============================================================================

TEST_F(AudioFileLoaderTest, WavInfoDescription) {
    WavInfo info;
    info.audio_format = 1; // PCM
    info.channels = 2;
    info.sample_rate = 44100;
    info.bits_per_sample = 16;
    info.num_samples = 44100;
    info.duration_sec = 1.0f;

    std::string desc = info.getFormatDescription();
    EXPECT_NE(desc.find("PCM 16-bit"), std::string::npos);
    EXPECT_NE(desc.find("Stereo"), std::string::npos);
    EXPECT_NE(desc.find("44100 Hz"), std::string::npos);
}

TEST_F(AudioFileLoaderTest, WavInfoValid) {
    WavInfo info;
    EXPECT_FALSE(info.isValid());

    info.sample_rate = 48000;
    info.channels = 1;
    info.bits_per_sample = 16;
    info.num_samples = 1000;
    EXPECT_TRUE(info.isValid());
}
