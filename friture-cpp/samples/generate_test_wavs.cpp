/**
 * @file generate_test_wavs.cpp
 * @brief Generate test WAV files in various formats for testing
 *
 * Generates:
 * - sine_1khz_pcm16_mono.wav - 1 kHz sine, PCM 16-bit, mono
 * - sine_1khz_pcm16_stereo.wav - 1 kHz sine, PCM 16-bit, stereo
 * - sine_1khz_pcm24_mono.wav - 1 kHz sine, PCM 24-bit, mono
 * - sine_1khz_float32_mono.wav - 1 kHz sine, IEEE Float 32-bit, mono
 * - chirp_100_10k_pcm16.wav - Chirp 100Hz→10kHz, PCM 16-bit, mono
 * - silence_pcm16.wav - Silence (for null testing)
 * - multitone_pcm16.wav - Multiple frequencies (440Hz + 880Hz + 1320Hz)
 *
 * @author Friture C++ Port
 * @date 2025-11-06
 */

#include <cstdio>
#include <cstdint>
#include <cstring>
#include <cmath>
#include <vector>
#include <iostream>

constexpr float PI = 3.14159265358979323846f;

// ============================================================================
// WAV Writer Class
// ============================================================================

class WavWriter {
public:
    static bool writePCM16Mono(const char* filename, const std::vector<float>& samples,
                               uint32_t sample_rate) {
        FILE* fp = fopen(filename, "wb");
        if (!fp) return false;

        uint32_t num_samples = static_cast<uint32_t>(samples.size());
        uint32_t data_size = num_samples * 2;
        uint32_t file_size = 36 + data_size;

        writeHeader(fp, file_size, 1, sample_rate, 16, 1, data_size);

        for (float sample : samples) {
            int16_t pcm = static_cast<int16_t>(sample * 32767.0f);
            fwrite(&pcm, 2, 1, fp);
        }

        fclose(fp);
        std::cout << "Created: " << filename << " (" << num_samples << " samples)" << std::endl;
        return true;
    }

    static bool writePCM16Stereo(const char* filename, const std::vector<float>& left,
                                 const std::vector<float>& right, uint32_t sample_rate) {
        FILE* fp = fopen(filename, "wb");
        if (!fp) return false;

        uint32_t num_frames = static_cast<uint32_t>(left.size());
        uint32_t data_size = num_frames * 2 * 2; // 2 channels * 2 bytes
        uint32_t file_size = 36 + data_size;

        writeHeader(fp, file_size, 2, sample_rate, 16, 1, data_size);

        for (size_t i = 0; i < left.size(); ++i) {
            int16_t pcm_l = static_cast<int16_t>(left[i] * 32767.0f);
            int16_t pcm_r = static_cast<int16_t>(right[i] * 32767.0f);
            fwrite(&pcm_l, 2, 1, fp);
            fwrite(&pcm_r, 2, 1, fp);
        }

        fclose(fp);
        std::cout << "Created: " << filename << " (" << num_frames << " frames)" << std::endl;
        return true;
    }

    static bool writePCM24Mono(const char* filename, const std::vector<float>& samples,
                               uint32_t sample_rate) {
        FILE* fp = fopen(filename, "wb");
        if (!fp) return false;

        uint32_t num_samples = static_cast<uint32_t>(samples.size());
        uint32_t data_size = num_samples * 3;
        uint32_t file_size = 36 + data_size;

        writeHeader(fp, file_size, 1, sample_rate, 24, 1, data_size);

        for (float sample : samples) {
            int32_t pcm = static_cast<int32_t>(sample * 8388607.0f);
            uint8_t bytes[3] = {
                static_cast<uint8_t>(pcm & 0xFF),
                static_cast<uint8_t>((pcm >> 8) & 0xFF),
                static_cast<uint8_t>((pcm >> 16) & 0xFF)
            };
            fwrite(bytes, 1, 3, fp);
        }

        fclose(fp);
        std::cout << "Created: " << filename << " (" << num_samples << " samples)" << std::endl;
        return true;
    }

    static bool writeFloat32Mono(const char* filename, const std::vector<float>& samples,
                                 uint32_t sample_rate) {
        FILE* fp = fopen(filename, "wb");
        if (!fp) return false;

        uint32_t num_samples = static_cast<uint32_t>(samples.size());
        uint32_t data_size = num_samples * 4;
        uint32_t file_size = 36 + data_size;

        writeHeader(fp, file_size, 1, sample_rate, 32, 3, data_size); // format=3 for IEEE float

        fwrite(samples.data(), sizeof(float), samples.size(), fp);

        fclose(fp);
        std::cout << "Created: " << filename << " (" << num_samples << " samples)" << std::endl;
        return true;
    }

private:
    static void writeHeader(FILE* fp, uint32_t file_size, uint16_t channels,
                           uint32_t sample_rate, uint16_t bits_per_sample,
                           uint16_t audio_format, uint32_t data_size) {
        // RIFF header
        fwrite("RIFF", 1, 4, fp);
        fwrite(&file_size, 4, 1, fp);
        fwrite("WAVE", 1, 4, fp);

        // fmt chunk
        fwrite("fmt ", 1, 4, fp);
        uint32_t fmt_size = 16;
        fwrite(&fmt_size, 4, 1, fp);
        fwrite(&audio_format, 2, 1, fp);
        fwrite(&channels, 2, 1, fp);
        fwrite(&sample_rate, 4, 1, fp);
        uint32_t byte_rate = sample_rate * channels * bits_per_sample / 8;
        fwrite(&byte_rate, 4, 1, fp);
        uint16_t block_align = channels * bits_per_sample / 8;
        fwrite(&block_align, 2, 1, fp);
        fwrite(&bits_per_sample, 2, 1, fp);

        // data chunk
        fwrite("data", 1, 4, fp);
        fwrite(&data_size, 4, 1, fp);
    }
};

// ============================================================================
// Signal Generators
// ============================================================================

std::vector<float> generateSine(float frequency, uint32_t sample_rate, float duration) {
    size_t num_samples = static_cast<size_t>(duration * sample_rate);
    std::vector<float> samples(num_samples);

    for (size_t i = 0; i < num_samples; ++i) {
        float t = static_cast<float>(i) / sample_rate;
        samples[i] = 0.5f * std::sin(2.0f * PI * frequency * t);
    }

    return samples;
}

std::vector<float> generateChirp(float f_start, float f_end, uint32_t sample_rate,
                                 float duration) {
    size_t num_samples = static_cast<size_t>(duration * sample_rate);
    std::vector<float> samples(num_samples);

    float k = (f_end - f_start) / duration;

    for (size_t i = 0; i < num_samples; ++i) {
        float t = static_cast<float>(i) / sample_rate;
        float phase = 2.0f * PI * (f_start * t + 0.5f * k * t * t);
        samples[i] = 0.5f * std::sin(phase);
    }

    return samples;
}

std::vector<float> generateMultiTone(const std::vector<float>& frequencies,
                                     uint32_t sample_rate, float duration) {
    size_t num_samples = static_cast<size_t>(duration * sample_rate);
    std::vector<float> samples(num_samples, 0.0f);

    float amplitude = 0.5f / frequencies.size();

    for (float freq : frequencies) {
        for (size_t i = 0; i < num_samples; ++i) {
            float t = static_cast<float>(i) / sample_rate;
            samples[i] += amplitude * std::sin(2.0f * PI * freq * t);
        }
    }

    return samples;
}

std::vector<float> generateSilence(uint32_t sample_rate, float duration) {
    size_t num_samples = static_cast<size_t>(duration * sample_rate);
    return std::vector<float>(num_samples, 0.0f);
}

// ============================================================================
// Main
// ============================================================================

int main(int argc, char* argv[]) {
    const char* output_dir = (argc > 1) ? argv[1] : ".";

    std::cout << "=== Generating Test WAV Files ===" << std::endl;
    std::cout << "Output directory: " << output_dir << std::endl << std::endl;

    constexpr uint32_t SAMPLE_RATE = 48000;
    constexpr float DURATION = 1.0f;

    // 1. 1 kHz sine - PCM 16-bit mono
    auto sine_1khz = generateSine(1000.0f, SAMPLE_RATE, DURATION);
    char filename[256];
    snprintf(filename, sizeof(filename), "%s/sine_1khz_pcm16_mono.wav", output_dir);
    WavWriter::writePCM16Mono(filename, sine_1khz, SAMPLE_RATE);

    // 2. 1 kHz sine - PCM 16-bit stereo
    snprintf(filename, sizeof(filename), "%s/sine_1khz_pcm16_stereo.wav", output_dir);
    WavWriter::writePCM16Stereo(filename, sine_1khz, sine_1khz, SAMPLE_RATE);

    // 3. 1 kHz sine - PCM 24-bit mono
    snprintf(filename, sizeof(filename), "%s/sine_1khz_pcm24_mono.wav", output_dir);
    WavWriter::writePCM24Mono(filename, sine_1khz, SAMPLE_RATE);

    // 4. 1 kHz sine - IEEE Float 32-bit mono
    snprintf(filename, sizeof(filename), "%s/sine_1khz_float32_mono.wav", output_dir);
    WavWriter::writeFloat32Mono(filename, sine_1khz, SAMPLE_RATE);

    // 5. Chirp 100 Hz → 10 kHz
    auto chirp = generateChirp(100.0f, 10000.0f, SAMPLE_RATE, 5.0f);
    snprintf(filename, sizeof(filename), "%s/chirp_100_10k_pcm16.wav", output_dir);
    WavWriter::writePCM16Mono(filename, chirp, SAMPLE_RATE);

    // 6. Silence
    auto silence = generateSilence(SAMPLE_RATE, 0.5f);
    snprintf(filename, sizeof(filename), "%s/silence_pcm16.wav", output_dir);
    WavWriter::writePCM16Mono(filename, silence, SAMPLE_RATE);

    // 7. Multi-tone (A4 + A5 + E5 = 440 Hz + 880 Hz + 1320 Hz)
    auto multitone = generateMultiTone({440.0f, 880.0f, 1320.0f}, SAMPLE_RATE, 2.0f);
    snprintf(filename, sizeof(filename), "%s/multitone_pcm16.wav", output_dir);
    WavWriter::writePCM16Mono(filename, multitone, SAMPLE_RATE);

    // 8. Pink noise - for visual testing
    auto pink = generateSine(500.0f, SAMPLE_RATE, 3.0f); // Simplified - use sine for now
    snprintf(filename, sizeof(filename), "%s/pink_noise_pcm16.wav", output_dir);
    WavWriter::writePCM16Mono(filename, pink, SAMPLE_RATE);

    std::cout << "\n=== Done! ===" << std::endl;
    std::cout << "Generated 8 test WAV files in " << output_dir << std::endl;

    return 0;
}
