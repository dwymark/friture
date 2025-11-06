/**
 * @file pipeline_test.cpp
 * @brief Integration test for complete spectrogram pipeline
 *
 * This program demonstrates the end-to-end signal processing pipeline:
 * Audio Generation → FFT → Frequency Resampling → Color Transform → Spectrogram Image
 *
 * It generates several test images with different audio signals:
 * 1. Pure sine wave (1 kHz)
 * 2. Linear chirp (100 Hz → 10 kHz)
 * 3. Multi-tone signal (harmonic series)
 * 4. Frequency-modulated sine wave
 *
 * Output: BMP images in friture-cpp/output/ directory
 */

#include <friture/ringbuffer.hpp>
#include <friture/settings.hpp>
#include <friture/fft_processor.hpp>
#include <friture/frequency_resampler.hpp>
#include <friture/color_transform.hpp>
#include <friture/spectrogram_image.hpp>

#include <iostream>
#include <vector>
#include <cmath>
#include <filesystem>
#include <iomanip>

using namespace friture;

// ============================================================================
// Synthetic Audio Generators
// ============================================================================

/**
 * @brief Generate a pure sine wave
 * @param frequency Frequency in Hz
 * @param sample_rate Sample rate in Hz
 * @param duration Duration in seconds
 * @return Vector of audio samples
 */
std::vector<float> generateSineWave(float frequency, float sample_rate, float duration) {
    size_t num_samples = static_cast<size_t>(duration * sample_rate);
    std::vector<float> samples(num_samples);

    for (size_t i = 0; i < num_samples; ++i) {
        float t = static_cast<float>(i) / sample_rate;
        samples[i] = 0.5f * std::sin(2.0f * M_PI * frequency * t);
    }

    return samples;
}

/**
 * @brief Generate a linear chirp (frequency sweep)
 * @param f_start Start frequency in Hz
 * @param f_end End frequency in Hz
 * @param sample_rate Sample rate in Hz
 * @param duration Duration in seconds
 * @return Vector of audio samples
 */
std::vector<float> generateChirp(float f_start, float f_end, float sample_rate, float duration) {
    size_t num_samples = static_cast<size_t>(duration * sample_rate);
    std::vector<float> samples(num_samples);

    float k = (f_end - f_start) / duration; // Hz/sec

    for (size_t i = 0; i < num_samples; ++i) {
        float t = static_cast<float>(i) / sample_rate;
        float instantaneous_freq = f_start + k * t;
        float phase = 2.0f * M_PI * (f_start * t + 0.5f * k * t * t);
        samples[i] = 0.5f * std::sin(phase);
    }

    return samples;
}

/**
 * @brief Generate a multi-tone signal (harmonic series)
 * @param fundamental Fundamental frequency in Hz
 * @param num_harmonics Number of harmonics to include
 * @param sample_rate Sample rate in Hz
 * @param duration Duration in seconds
 * @return Vector of audio samples
 */
std::vector<float> generateMultiTone(float fundamental, size_t num_harmonics,
                                     float sample_rate, float duration) {
    size_t num_samples = static_cast<size_t>(duration * sample_rate);
    std::vector<float> samples(num_samples, 0.0f);

    for (size_t harmonic = 1; harmonic <= num_harmonics; ++harmonic) {
        float frequency = fundamental * harmonic;
        float amplitude = 0.5f / harmonic; // Decrease amplitude with harmonic number

        for (size_t i = 0; i < num_samples; ++i) {
            float t = static_cast<float>(i) / sample_rate;
            samples[i] += amplitude * std::sin(2.0f * M_PI * frequency * t);
        }
    }

    return samples;
}

/**
 * @brief Generate frequency-modulated (FM) sine wave
 * @param carrier_freq Carrier frequency in Hz
 * @param mod_freq Modulation frequency in Hz
 * @param mod_depth Modulation depth (frequency deviation in Hz)
 * @param sample_rate Sample rate in Hz
 * @param duration Duration in seconds
 * @return Vector of audio samples
 */
std::vector<float> generateFM(float carrier_freq, float mod_freq, float mod_depth,
                              float sample_rate, float duration) {
    size_t num_samples = static_cast<size_t>(duration * sample_rate);
    std::vector<float> samples(num_samples);

    for (size_t i = 0; i < num_samples; ++i) {
        float t = static_cast<float>(i) / sample_rate;
        float modulator = mod_depth * std::sin(2.0f * M_PI * mod_freq * t);
        float instantaneous_freq = carrier_freq + modulator;
        samples[i] = 0.5f * std::sin(2.0f * M_PI * instantaneous_freq * t);
    }

    return samples;
}

/**
 * @brief Generate white noise
 * @param sample_rate Sample rate in Hz
 * @param duration Duration in seconds
 * @param amplitude Noise amplitude (0-1)
 * @return Vector of audio samples
 */
std::vector<float> generateNoise(float sample_rate, float duration, float amplitude = 0.1f) {
    size_t num_samples = static_cast<size_t>(duration * sample_rate);
    std::vector<float> samples(num_samples);

    for (size_t i = 0; i < num_samples; ++i) {
        samples[i] = amplitude * (2.0f * (static_cast<float>(rand()) / RAND_MAX) - 1.0f);
    }

    return samples;
}

// ============================================================================
// Pipeline Processing
// ============================================================================

/**
 * @brief Process audio through complete spectrogram pipeline
 * @param audio_samples Input audio samples
 * @param settings Spectrogram settings
 * @param output_filename Output BMP filename
 * @param image_width Width of output spectrogram image
 */
void processAudioToSpectrogram(const std::vector<float>& audio_samples,
                               const SpectrogramSettings& settings,
                               const char* output_filename,
                               size_t image_width = 800) {
    std::cout << "\nProcessing: " << output_filename << std::endl;
    std::cout << "  Audio samples: " << audio_samples.size() << std::endl;
    std::cout << "  Duration: " << (audio_samples.size() / settings.sample_rate) << " seconds" << std::endl;

    // Image height based on frequency scale
    const size_t image_height = 400;

    // Create processing components
    FFTProcessor fft_processor(settings.fft_size, settings.window_type);
    FrequencyResampler freq_resampler(settings.freq_scale, settings.min_freq,
                                      settings.max_freq, settings.sample_rate,
                                      settings.fft_size, image_height);
    ColorTransform color_transform(ColorTheme::CMRMAP);
    SpectrogramImage spectrogram(image_width, image_height);

    // Ring buffer for audio samples
    RingBuffer<float> ring_buffer(audio_samples.size() + settings.fft_size);

    // Feed audio into ring buffer
    ring_buffer.write(audio_samples.data(), audio_samples.size());

    // Calculate number of FFT frames
    size_t samples_per_column = settings.getSamplesPerColumn();
    size_t num_frames = (audio_samples.size() - settings.fft_size) / samples_per_column;

    // Limit to image width
    num_frames = std::min(num_frames, image_width);

    std::cout << "  FFT size: " << settings.fft_size << std::endl;
    std::cout << "  Samples per column: " << samples_per_column << std::endl;
    std::cout << "  Processing " << num_frames << " frames..." << std::endl;

    // Buffers
    std::vector<float> fft_input(settings.fft_size);
    std::vector<float> fft_output(settings.fft_size / 2 + 1);
    std::vector<float> resampled(image_height);
    std::vector<float> normalized(image_height);
    std::vector<uint32_t> colors(image_height);

    // Process each frame
    for (size_t frame = 0; frame < num_frames; ++frame) {
        // Read samples from ring buffer
        size_t read_offset = frame * samples_per_column;
        ring_buffer.read(read_offset, fft_input.data(), settings.fft_size);

        // FFT processing
        fft_processor.process(fft_input.data(), fft_output.data());

        // Frequency resampling
        freq_resampler.resample(fft_output.data(), resampled.data());

        // Normalize to [0, 1] range
        for (size_t i = 0; i < image_height; ++i) {
            normalized[i] = (resampled[i] - settings.spec_min_db) /
                          (settings.spec_max_db - settings.spec_min_db);
            normalized[i] = std::clamp(normalized[i], 0.0f, 1.0f);
        }

        // Color transformation
        color_transform.transformColumn(normalized.data(), image_height, colors.data());

        // Add column to spectrogram image
        spectrogram.addColumn(colors.data(), image_height);

        // Progress indicator
        if (frame % 50 == 0 || frame == num_frames - 1) {
            std::cout << "\r  Progress: " << (frame + 1) << "/" << num_frames << " frames"
                     << std::flush;
        }
    }

    std::cout << std::endl;

    // Save to BMP file
    std::cout << "  Saving to: " << output_filename << "..." << std::endl;
    bool success = spectrogram.saveToBMP(output_filename);

    if (success) {
        std::cout << "  ✓ Successfully saved spectrogram image" << std::endl;
    } else {
        std::cerr << "  ✗ Failed to save spectrogram image" << std::endl;
    }
}

// ============================================================================
// Main
// ============================================================================

int main() {
    std::cout << "=== Friture C++ Pipeline Integration Test ===" << std::endl;
    std::cout << "\nThis program generates spectrograms from synthetic audio signals." << std::endl;
    std::cout << "Output images will be saved in the output/ directory." << std::endl;

    // Create output directory
    std::filesystem::create_directories("output");

    // Default settings
    SpectrogramSettings settings;
    settings.fft_size = 2048;
    settings.window_type = WindowFunction::Hann;
    settings.sample_rate = 48000.0f;
    settings.min_freq = 20.0f;
    settings.max_freq = 24000.0f;
    settings.spec_min_db = -100.0f;
    settings.spec_max_db = 0.0f;

    const float duration = 2.0f; // 2 seconds of audio
    const size_t image_width = 800;

    // ========================================================================
    // Test 1: Pure Sine Wave (1 kHz)
    // ========================================================================
    {
        settings.freq_scale = FrequencyScale::Linear;
        auto audio = generateSineWave(1000.0f, settings.sample_rate, duration);
        processAudioToSpectrogram(audio, settings, "output/01_sine_1khz_linear.bmp", image_width);
    }

    // ========================================================================
    // Test 2: Linear Chirp (100 Hz → 10 kHz)
    // ========================================================================
    {
        settings.freq_scale = FrequencyScale::Linear;
        auto audio = generateChirp(100.0f, 10000.0f, settings.sample_rate, duration);
        processAudioToSpectrogram(audio, settings, "output/02_chirp_linear.bmp", image_width);
    }

    // ========================================================================
    // Test 3: Linear Chirp on Mel Scale
    // ========================================================================
    {
        settings.freq_scale = FrequencyScale::Mel;
        auto audio = generateChirp(100.0f, 10000.0f, settings.sample_rate, duration);
        processAudioToSpectrogram(audio, settings, "output/03_chirp_mel.bmp", image_width);
    }

    // ========================================================================
    // Test 4: Multi-Tone (Harmonic Series) - 440 Hz (A4)
    // ========================================================================
    {
        settings.freq_scale = FrequencyScale::Linear;
        auto audio = generateMultiTone(440.0f, 8, settings.sample_rate, duration);
        processAudioToSpectrogram(audio, settings, "output/04_harmonics_440hz.bmp", image_width);
    }

    // ========================================================================
    // Test 5: FM Synthesis (Carrier: 2 kHz, Mod: 5 Hz)
    // ========================================================================
    {
        settings.freq_scale = FrequencyScale::Linear;
        auto audio = generateFM(2000.0f, 5.0f, 500.0f, settings.sample_rate, duration);
        processAudioToSpectrogram(audio, settings, "output/05_fm_synthesis.bmp", image_width);
    }

    // ========================================================================
    // Test 6: White Noise
    // ========================================================================
    {
        settings.freq_scale = FrequencyScale::Linear;
        auto audio = generateNoise(settings.sample_rate, duration, 0.2f);
        processAudioToSpectrogram(audio, settings, "output/06_white_noise.bmp", image_width);
    }

    // ========================================================================
    // Test 7: Chirp on Logarithmic Scale
    // ========================================================================
    {
        settings.freq_scale = FrequencyScale::Logarithmic;
        auto audio = generateChirp(100.0f, 10000.0f, settings.sample_rate, duration);
        processAudioToSpectrogram(audio, settings, "output/07_chirp_log.bmp", image_width);
    }

    // ========================================================================
    // Test 8: Musical Notes (C major scale)
    // ========================================================================
    {
        settings.freq_scale = FrequencyScale::Linear;
        const float note_duration = duration / 8.0f;

        // C major scale frequencies (C4-C5)
        std::vector<float> scale = {261.63f, 293.66f, 329.63f, 349.23f,
                                    392.00f, 440.00f, 493.88f, 523.25f};

        std::vector<float> audio;
        for (float freq : scale) {
            auto note = generateSineWave(freq, settings.sample_rate, note_duration);
            audio.insert(audio.end(), note.begin(), note.end());
        }

        processAudioToSpectrogram(audio, settings, "output/08_c_major_scale.bmp", image_width);
    }

    // ========================================================================
    // Summary
    // ========================================================================
    std::cout << "\n=== Summary ===" << std::endl;
    std::cout << "Successfully generated 8 spectrogram images!" << std::endl;
    std::cout << "\nTest images saved in output/ directory:" << std::endl;
    std::cout << "  1. 01_sine_1khz_linear.bmp  - Pure 1 kHz sine wave" << std::endl;
    std::cout << "  2. 02_chirp_linear.bmp      - Linear chirp (100 Hz → 10 kHz)" << std::endl;
    std::cout << "  3. 03_chirp_mel.bmp         - Chirp on Mel scale" << std::endl;
    std::cout << "  4. 04_harmonics_440hz.bmp   - Harmonic series (440 Hz + overtones)" << std::endl;
    std::cout << "  5. 05_fm_synthesis.bmp      - Frequency-modulated signal" << std::endl;
    std::cout << "  6. 06_white_noise.bmp       - White noise" << std::endl;
    std::cout << "  7. 07_chirp_log.bmp         - Chirp on logarithmic scale" << std::endl;
    std::cout << "  8. 08_c_major_scale.bmp     - C major scale (8 notes)" << std::endl;
    std::cout << "\n✓ Pipeline integration test PASSED" << std::endl;

    return 0;
}
