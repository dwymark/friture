/**
 * @file settings.hpp
 * @brief Spectrogram settings management with validation
 *
 * This file contains the SpectrogramSettings structure which holds all
 * configuration parameters for the spectrogram visualization and audio
 * processing pipeline.
 *
 * @author Friture C++ Port
 * @date 2025-11-06
 */

#ifndef FRITURE_SETTINGS_HPP
#define FRITURE_SETTINGS_HPP

#include <friture/types.hpp>
#include <cstddef>
#include <cmath>
#include <algorithm>

namespace friture {

/**
 * @brief Settings for spectrogram visualization and audio processing
 *
 * This structure holds all configurable parameters for the Friture
 * spectrogram. All settings include validation to ensure they remain
 * within acceptable ranges.
 *
 * Thread Safety: Not thread-safe. Settings should be modified from
 * the UI thread only, or protected by appropriate synchronization.
 *
 * Example:
 * @code
 * SpectrogramSettings settings;
 * settings.setFFTSize(8192);
 * settings.setFrequencyRange(50.0f, 20000.0f);
 * if (settings.isValid()) {
 *     // Use settings...
 * }
 * @endcode
 */
struct SpectrogramSettings {
    // ========================================================================
    // FFT Settings
    // ========================================================================

    /**
     * @brief FFT size (must be power of 2 × 32)
     *
     * Valid range: 32, 64, 128, 256, 512, 1024, 2048, 4096, 8192, 16384
     * Larger sizes provide better frequency resolution but higher latency.
     */
    size_t fft_size = 4096;

    /**
     * @brief Window function for FFT processing
     *
     * Default: Hann window (good general-purpose choice)
     */
    WindowFunction window_type = WindowFunction::Hann;

    /**
     * @brief FFT overlap percentage (fixed at 75%)
     *
     * Overlap determines how much consecutive FFT frames share samples.
     * 75% overlap provides good time resolution without excessive computation.
     *
     * Note: Currently fixed; may become configurable in future versions.
     */
    static constexpr float overlap_percent = 75.0f;

    // ========================================================================
    // Frequency Settings
    // ========================================================================

    /**
     * @brief Frequency scale for Y-axis display
     *
     * Default: Mel scale (perceptually linear)
     */
    FrequencyScale freq_scale = FrequencyScale::Mel;

    /**
     * @brief Minimum frequency to display (Hz)
     *
     * Valid range: 10 Hz to Nyquist frequency
     * Must be less than max_freq
     */
    float min_freq = 20.0f;

    /**
     * @brief Maximum frequency to display (Hz)
     *
     * Valid range: min_freq to Nyquist frequency (sample_rate / 2)
     * Default assumes 48 kHz sample rate
     */
    float max_freq = 24000.0f;

    // ========================================================================
    // Amplitude Settings
    // ========================================================================

    /**
     * @brief Minimum amplitude for color mapping (dB)
     *
     * Valid range: -200 dB to spec_max_db
     * Values below this are clipped to black in the colormap
     */
    float spec_min_db = -140.0f;

    /**
     * @brief Maximum amplitude for color mapping (dB)
     *
     * Valid range: spec_min_db to +200 dB
     * Values above this are clipped to white in the colormap
     */
    float spec_max_db = 0.0f;

    // ========================================================================
    // Time Settings
    // ========================================================================

    /**
     * @brief Time range to display (seconds)
     *
     * Valid range: 0.1 to 1000 seconds
     * Determines how much spectrogram history is shown on screen
     */
    float time_range = 10.0f;

    // ========================================================================
    // Audio Processing Settings
    // ========================================================================

    /**
     * @brief Psychoacoustic weighting type
     *
     * Default: None (flat frequency response)
     */
    WeightingType weighting = WeightingType::None;

    /**
     * @brief Sample rate (Hz)
     *
     * This is typically set by the audio engine based on the input device.
     * Common values: 44100, 48000, 96000
     */
    float sample_rate = 48000.0f;

    // ========================================================================
    // Validation Methods
    // ========================================================================

    /**
     * @brief Check if all settings are valid
     * @return true if all settings are within acceptable ranges
     *
     * Checks:
     * - FFT size is a valid power-of-2 multiple of 32
     * - Frequency range is valid (min < max, both > 0)
     * - Amplitude range is valid (min < max)
     * - Time range is positive
     * - Frequencies don't exceed Nyquist limit
     */
    bool isValid() const {
        // Check FFT size is valid power of 2 × 32
        if (!isValidFFTSize(fft_size)) {
            return false;
        }

        // Check frequency range
        if (min_freq <= 0.0f || max_freq <= 0.0f) {
            return false;
        }
        if (min_freq >= max_freq) {
            return false;
        }

        // Check frequencies don't exceed Nyquist
        float nyquist = sample_rate / 2.0f;
        if (max_freq > nyquist) {
            return false;
        }

        // Check amplitude range
        if (spec_min_db >= spec_max_db) {
            return false;
        }
        if (spec_min_db < -200.0f || spec_max_db > 200.0f) {
            return false;
        }

        // Check time range (minimum 0.1 seconds)
        if (time_range < 0.1f || time_range > 1000.0f) {
            return false;
        }

        return true;
    }

    /**
     * @brief Set FFT size with validation
     * @param size Desired FFT size
     * @return true if size was valid and set, false otherwise
     *
     * Valid sizes: 32, 64, 128, 256, 512, 1024, 2048, 4096, 8192, 16384
     */
    bool setFFTSize(size_t size) {
        if (!isValidFFTSize(size)) {
            return false;
        }
        fft_size = size;
        return true;
    }

    /**
     * @brief Set frequency range with validation
     * @param min Minimum frequency (Hz)
     * @param max Maximum frequency (Hz)
     * @return true if range was valid and set, false otherwise
     *
     * Constraints:
     * - min must be > 0 and < max
     * - max must not exceed Nyquist frequency
     */
    bool setFrequencyRange(float min, float max) {
        if (min <= 0.0f || max <= 0.0f || min >= max) {
            return false;
        }

        float nyquist = sample_rate / 2.0f;
        if (max > nyquist) {
            return false;
        }

        min_freq = min;
        max_freq = max;
        return true;
    }

    /**
     * @brief Set amplitude range with validation
     * @param min Minimum amplitude (dB)
     * @param max Maximum amplitude (dB)
     * @return true if range was valid and set, false otherwise
     *
     * Constraints:
     * - min must be < max
     * - Both must be in range [-200, +200] dB
     */
    bool setAmplitudeRange(float min, float max) {
        if (min >= max) {
            return false;
        }
        if (min < -200.0f || max > 200.0f) {
            return false;
        }

        spec_min_db = min;
        spec_max_db = max;
        return true;
    }

    /**
     * @brief Set time range with validation
     * @param range Time range in seconds
     * @return true if range was valid and set, false otherwise
     *
     * Constraints: Must be in range [0.1, 1000] seconds
     */
    bool setTimeRange(float range) {
        if (range < 0.1f || range > 1000.0f) {
            return false;
        }

        time_range = range;
        return true;
    }

    /**
     * @brief Set sample rate and adjust frequency limits if needed
     * @param rate Sample rate in Hz
     * @return true if rate was valid and set, false otherwise
     *
     * If max_freq exceeds the new Nyquist frequency, it will be
     * automatically adjusted downward.
     */
    bool setSampleRate(float rate) {
        if (rate <= 0.0f) {
            return false;
        }

        sample_rate = rate;

        // Adjust max frequency if it exceeds new Nyquist limit
        float nyquist = rate / 2.0f;
        if (max_freq > nyquist) {
            max_freq = nyquist;
        }

        return true;
    }

    /**
     * @brief Get Nyquist frequency (sample_rate / 2)
     * @return Nyquist frequency in Hz
     */
    float getNyquistFrequency() const {
        return sample_rate / 2.0f;
    }

    /**
     * @brief Get number of samples per FFT column (based on overlap)
     * @return Number of samples between consecutive FFT frames
     */
    size_t getSamplesPerColumn() const {
        return static_cast<size_t>(fft_size * (1.0f - overlap_percent / 100.0f));
    }

    /**
     * @brief Get time per FFT column in seconds
     * @return Time duration of one FFT column
     */
    float getTimePerColumn() const {
        return getSamplesPerColumn() / sample_rate;
    }

private:
    /**
     * @brief Check if FFT size is valid
     * @param size FFT size to check
     * @return true if size is a valid power-of-2 multiple of 32
     *
     * Valid sizes: 32, 64, 128, 256, 512, 1024, 2048, 4096, 8192, 16384
     */
    static bool isValidFFTSize(size_t size) {
        // Must be >= 32 and <= 16384
        if (size < 32 || size > 16384) {
            return false;
        }

        // Must be a power of 2
        if ((size & (size - 1)) != 0) {
            return false;
        }

        return true;
    }
};

} // namespace friture

#endif // FRITURE_SETTINGS_HPP
