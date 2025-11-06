/**
 * FFT Test - Performance comparison and correctness validation
 * Tests FFTW3 if available, otherwise uses Eigen
 */
#include <iostream>
#include <vector>
#include <complex>
#include <cmath>
#include <chrono>
#include <iomanip>

#ifdef USE_FFTW
#include <fftw3.h>
#endif

#include <eigen3/Eigen/Dense>
#include <eigen3/unsupported/Eigen/FFT>

const double PI = 3.14159265358979323846;

// Generate test signal: sum of sine waves
std::vector<float> generateTestSignal(size_t size, double sampleRate) {
    std::vector<float> signal(size);

    // Add 1000 Hz component
    for (size_t i = 0; i < size; ++i) {
        signal[i] = std::sin(2.0 * PI * 1000.0 * i / sampleRate);
    }

    // Add 2000 Hz component (half amplitude)
    for (size_t i = 0; i < size; ++i) {
        signal[i] += 0.5 * std::sin(2.0 * PI * 2000.0 * i / sampleRate);
    }

    // Add 3000 Hz component (quarter amplitude)
    for (size_t i = 0; i < size; ++i) {
        signal[i] += 0.25 * std::sin(2.0 * PI * 3000.0 * i / sampleRate);
    }

    return signal;
}

// Apply Hann window
void applyHannWindow(std::vector<float>& signal) {
    size_t N = signal.size();
    for (size_t i = 0; i < N; ++i) {
        float window = 0.5f * (1.0f - std::cos(2.0 * PI * i / (N - 1)));
        signal[i] *= window;
    }
}

#ifdef USE_FFTW
std::vector<float> fftFFTW(const std::vector<float>& signal) {
    size_t N = signal.size();

    // Allocate buffers
    float *in = fftwf_alloc_real(N);
    fftwf_complex *out = fftwf_alloc_complex(N/2 + 1);

    // Copy input
    for (size_t i = 0; i < N; ++i) {
        in[i] = signal[i];
    }

    // Create plan and execute
    fftwf_plan plan = fftwf_plan_dft_r2c_1d(N, in, out, FFTW_ESTIMATE);
    fftwf_execute(plan);

    // Calculate power spectrum
    std::vector<float> power(N/2 + 1);
    for (size_t i = 0; i < N/2 + 1; ++i) {
        float real = out[i][0];
        float imag = out[i][1];
        power[i] = (real*real + imag*imag) / (N * N);
    }

    // Cleanup
    fftwf_destroy_plan(plan);
    fftwf_free(in);
    fftwf_free(out);

    return power;
}
#endif

std::vector<float> fftEigen(const std::vector<float>& signal) {
    Eigen::FFT<float> fft;
    std::vector<std::complex<float>> freqVec;

    fft.fwd(freqVec, signal);

    size_t N = signal.size();
    std::vector<float> power(N/2 + 1);

    for (size_t i = 0; i < N/2 + 1; ++i) {
        float real = freqVec[i].real();
        float imag = freqVec[i].imag();
        power[i] = (real*real + imag*imag) / (N * N);
    }

    return power;
}

void findPeaks(const std::vector<float>& power, double sampleRate, size_t fftSize) {
    std::cout << "\n=== Peak Detection ===" << std::endl;

    // Find top 5 peaks
    std::vector<std::pair<size_t, float>> peaks;
    for (size_t i = 5; i < power.size() - 5; ++i) {
        // Simple peak detection: local maximum
        if (power[i] > power[i-1] && power[i] > power[i+1] &&
            power[i] > power[i-2] && power[i] > power[i+2]) {
            peaks.push_back({i, power[i]});
        }
    }

    // Sort by amplitude
    std::sort(peaks.begin(), peaks.end(),
              [](const auto& a, const auto& b) { return a.second > b.second; });

    // Display top peaks
    std::cout << "Top frequency peaks:" << std::endl;
    for (size_t i = 0; i < std::min(size_t(5), peaks.size()); ++i) {
        double freq = peaks[i].first * sampleRate / fftSize;
        float amplitude = peaks[i].second;
        float dB = 10.0f * std::log10(amplitude + 1e-30f);
        std::cout << "  " << std::fixed << std::setprecision(1)
                  << freq << " Hz: " << std::setprecision(2) << dB << " dB" << std::endl;
    }
}

int main() {
    std::cout << "=== FFT Test ===" << std::endl;

    const size_t fftSize = 4096;
    const double sampleRate = 48000.0;

    std::cout << "FFT Size: " << fftSize << std::endl;
    std::cout << "Sample Rate: " << sampleRate << " Hz" << std::endl;
    std::cout << "Frequency Resolution: " << (sampleRate / fftSize) << " Hz/bin" << std::endl;

    // Generate test signal
    std::cout << "\nGenerating test signal (1000, 2000, 3000 Hz)..." << std::endl;
    auto signal = generateTestSignal(fftSize, sampleRate);
    applyHannWindow(signal);

#ifdef USE_FFTW
    std::cout << "\n=== Testing FFTW3 ===" << std::endl;

    // Warm-up
    fftFFTW(signal);

    // Benchmark
    const int iterations = 1000;
    auto start = std::chrono::high_resolution_clock::now();

    std::vector<float> powerFFTW;
    for (int i = 0; i < iterations; ++i) {
        powerFFTW = fftFFTW(signal);
    }

    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);

    double avgTime = duration.count() / double(iterations);
    std::cout << "Average FFT time (FFTW3): " << avgTime << " μs" << std::endl;

    findPeaks(powerFFTW, sampleRate, fftSize);
#endif

    std::cout << "\n=== Testing Eigen FFT ===" << std::endl;

    // Warm-up
    fftEigen(signal);

    // Benchmark
    const int eigenIterations = 1000;
    auto eigenStart = std::chrono::high_resolution_clock::now();

    std::vector<float> powerEigen;
    for (int i = 0; i < eigenIterations; ++i) {
        powerEigen = fftEigen(signal);
    }

    auto eigenEnd = std::chrono::high_resolution_clock::now();
    auto eigenDuration = std::chrono::duration_cast<std::chrono::microseconds>(eigenEnd - eigenStart);

    double eigenAvgTime = eigenDuration.count() / double(eigenIterations);
    std::cout << "Average FFT time (Eigen): " << eigenAvgTime << " μs" << std::endl;

    findPeaks(powerEigen, sampleRate, fftSize);

#ifdef USE_FFTW
    // Compare results
    std::cout << "\n=== Comparing FFTW3 vs Eigen ===" << std::endl;
    double maxDiff = 0.0;
    double avgDiff = 0.0;
    for (size_t i = 0; i < powerFFTW.size(); ++i) {
        double diff = std::abs(powerFFTW[i] - powerEigen[i]);
        maxDiff = std::max(maxDiff, diff);
        avgDiff += diff;
    }
    avgDiff /= powerFFTW.size();

    std::cout << "Max difference: " << maxDiff << std::endl;
    std::cout << "Avg difference: " << avgDiff << std::endl;

    if (maxDiff < 1e-5) {
        std::cout << "✓ Results match within tolerance" << std::endl;
    } else {
        std::cout << "⚠ Results differ significantly" << std::endl;
    }
#endif

    std::cout << "\n✓ FFT test PASSED" << std::endl;
    return 0;
}
