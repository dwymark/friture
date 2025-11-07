/**
 * @file fft_processor.cpp
 * @brief Implementation of FFTProcessor
 *
 * @author Friture C++ Port
 * @date 2025-11-06
 */

#define _USE_MATH_DEFINES  // For M_PI on Windows/MSVC
#include <friture/fft_processor.hpp>
#include <cmath>
#include <stdexcept>
#include <sstream>

namespace friture {

// ============================================================================
// Constructor & Destructor
// ============================================================================

FFTProcessor::FFTProcessor(size_t fft_size, WindowFunction window_type)
    : fft_size_(fft_size),
      window_type_(window_type),
      window_(fft_size),
      fftw_input_(nullptr),
      fftw_output_(nullptr),
      fft_plan_(nullptr)
{
    if (!isValidFFTSize(fft_size)) {
        std::ostringstream oss;
        oss << "Invalid FFT size: " << fft_size
            << ". Must be power of 2, range [32, 16384]";
        throw std::invalid_argument(oss.str());
    }

    initialize();
}

FFTProcessor::~FFTProcessor() {
    cleanup();
}

// ============================================================================
// Main Processing
// ============================================================================

void FFTProcessor::process(const float* input, float* output) {
    // Apply window function
    for (size_t i = 0; i < fft_size_; ++i) {
        fftw_input_[i] = input[i] * window_[i];
    }

    // Compute FFT
    fftwf_execute(fft_plan_);

    // Compute power spectrum and convert to dB
    const size_t num_bins = fft_size_ / 2 + 1;
    const float scale = 1.0f / (fft_size_ * fft_size_);

    for (size_t i = 0; i < num_bins; ++i) {
        float real = fftw_output_[i][0];
        float imag = fftw_output_[i][1];
        float power = (real * real + imag * imag) * scale;
        output[i] = 10.0f * std::log10(power + EPSILON);
    }
}

// ============================================================================
// Configuration Methods
// ============================================================================

void FFTProcessor::setFFTSize(size_t new_size) {
    if (!isValidFFTSize(new_size)) {
        throw std::invalid_argument("Invalid FFT size");
    }

    if (new_size != fft_size_) {
        cleanup();
        fft_size_ = new_size;
        window_.resize(fft_size_);
        initialize();
    }
}

void FFTProcessor::setWindowFunction(WindowFunction type) {
    if (type != window_type_) {
        window_type_ = type;
        computeWindow();
    }
}

// ============================================================================
// Private Methods
// ============================================================================

void FFTProcessor::initialize() {
    // Allocate FFTW buffers
    fftw_input_ = fftwf_alloc_real(fft_size_);
    fftw_output_ = fftwf_alloc_complex(fft_size_ / 2 + 1);

    if (!fftw_input_ || !fftw_output_) {
        cleanup();
        throw std::runtime_error("Failed to allocate FFTW buffers");
    }

    // Create FFT plan
    fft_plan_ = fftwf_plan_dft_r2c_1d(
        fft_size_,
        fftw_input_,
        fftw_output_,
        FFTW_MEASURE
    );

    if (!fft_plan_) {
        cleanup();
        throw std::runtime_error("Failed to create FFTW plan");
    }

    // Compute window coefficients
    computeWindow();
}

void FFTProcessor::cleanup() {
    if (fft_plan_) {
        fftwf_destroy_plan(fft_plan_);
        fft_plan_ = nullptr;
    }

    if (fftw_input_) {
        fftwf_free(fftw_input_);
        fftw_input_ = nullptr;
    }

    if (fftw_output_) {
        fftwf_free(fftw_output_);
        fftw_output_ = nullptr;
    }
}

void FFTProcessor::computeWindow() {
    const float N = static_cast<float>(fft_size_ - 1);

    switch (window_type_) {
        case WindowFunction::Hann:
            for (size_t i = 0; i < fft_size_; ++i) {
                window_[i] = 0.5f * (1.0f - std::cos(2.0f * M_PI * i / N));
            }
            break;

        case WindowFunction::Hamming:
            for (size_t i = 0; i < fft_size_; ++i) {
                window_[i] = 0.54f - 0.46f * std::cos(2.0f * M_PI * i / N);
            }
            break;

        default:
            throw std::invalid_argument("Unknown window function type");
    }
}

bool FFTProcessor::isValidFFTSize(size_t size) {
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

} // namespace friture
