/**
 * @file frequency_resampler.cpp
 * @brief Implementation of FrequencyResampler
 *
 * @author Friture C++ Port
 * @date 2025-11-06
 */

#include <friture/frequency_resampler.hpp>
#include <cmath>
#include <stdexcept>
#include <sstream>

namespace friture {

// ============================================================================
// Constructor
// ============================================================================

FrequencyResampler::FrequencyResampler(
    FrequencyScale scale,
    float min_freq,
    float max_freq,
    float sample_rate,
    size_t fft_size,
    size_t output_height
)
    : scale_(scale),
      min_freq_(min_freq),
      max_freq_(max_freq),
      sample_rate_(sample_rate),
      fft_size_(fft_size),
      output_height_(output_height),
      freq_mapping_(output_height)
{
    validate();
    computeMapping();
}

// ============================================================================
// Main Processing
// ============================================================================

void FrequencyResampler::resample(const float* input, float* output) const {
    const size_t num_bins = fft_size_ / 2 + 1;

    for (size_t i = 0; i < output_height_; ++i) {
        float bin_idx = freq_mapping_[i];

        // Clamp to valid range
        bin_idx = std::max(0.0f, std::min(bin_idx, static_cast<float>(num_bins - 1)));

        // Get integer and fractional parts
        size_t bin0 = static_cast<size_t>(bin_idx);
        size_t bin1 = std::min(bin0 + 1, num_bins - 1);
        float frac = bin_idx - static_cast<float>(bin0);

        // Linear interpolation
        output[i] = input[bin0] * (1.0f - frac) + input[bin1] * frac;
    }
}

// ============================================================================
// Configuration Methods
// ============================================================================

void FrequencyResampler::setScale(FrequencyScale scale) {
    if (scale != scale_) {
        scale_ = scale;
        computeMapping();
    }
}

void FrequencyResampler::setFrequencyRange(float min_freq, float max_freq) {
    min_freq_ = min_freq;
    max_freq_ = max_freq;
    validate();
    computeMapping();
}

void FrequencyResampler::setOutputHeight(size_t height) {
    if (height == 0) {
        throw std::invalid_argument("Output height must be > 0");
    }

    if (height != output_height_) {
        output_height_ = height;
        freq_mapping_.resize(height);
        computeMapping();
    }
}

// ============================================================================
// Private Methods
// ============================================================================

void FrequencyResampler::validate() const {
    if (min_freq_ <= 0.0f) {
        throw std::invalid_argument("Minimum frequency must be > 0");
    }

    if (max_freq_ <= min_freq_) {
        std::ostringstream oss;
        oss << "Maximum frequency (" << max_freq_
            << " Hz) must be > minimum frequency (" << min_freq_ << " Hz)";
        throw std::invalid_argument(oss.str());
    }

    float nyquist = sample_rate_ / 2.0f;
    if (max_freq_ > nyquist) {
        std::ostringstream oss;
        oss << "Maximum frequency (" << max_freq_
            << " Hz) exceeds Nyquist frequency (" << nyquist << " Hz)";
        throw std::invalid_argument(oss.str());
    }

    if (output_height_ == 0) {
        throw std::invalid_argument("Output height must be > 0");
    }

    if (fft_size_ == 0 || (fft_size_ & (fft_size_ - 1)) != 0) {
        throw std::invalid_argument("FFT size must be a power of 2");
    }
}

void FrequencyResampler::computeMapping() {
    // Select transform and inverse functions based on scale
    float (*transform)(float);
    float (*inverse)(float);

    switch (scale_) {
        case FrequencyScale::Linear:
            transform = linearTransform;
            inverse = linearInverse;
            break;
        case FrequencyScale::Mel:
            transform = melTransform;
            inverse = melInverse;
            break;
        case FrequencyScale::ERB:
            transform = erbTransform;
            inverse = erbInverse;
            break;
        case FrequencyScale::Logarithmic:
            transform = logTransform;
            inverse = logInverse;
            break;
        case FrequencyScale::Octave:
            transform = octaveTransform;
            inverse = octaveInverse;
            break;
        default:
            throw std::invalid_argument("Unknown frequency scale");
    }

    // Compute transformed min and max
    float min_transformed = transform(min_freq_);
    float max_transformed = transform(max_freq_);

    // For each output pixel, compute the corresponding FFT bin index
    for (size_t i = 0; i < output_height_; ++i) {
        // Normalize position in output range [0, 1]
        float t = static_cast<float>(i) / static_cast<float>(output_height_ - 1);

        // Map to transformed frequency space
        float transformed = min_transformed + t * (max_transformed - min_transformed);

        // Apply inverse transform to get Hz
        float freq_hz = inverse(transformed);

        // Convert Hz to FFT bin index
        // bin_index = freq_hz * fft_size / sample_rate
        float bin_idx = freq_hz * static_cast<float>(fft_size_) / sample_rate_;

        // Store mapping
        freq_mapping_[i] = bin_idx;
    }
}

} // namespace friture
