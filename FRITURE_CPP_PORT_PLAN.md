# Friture C++ Spectrogram Port - Specification and Implementation Plan

**Version:** 1.0
**Date:** 2025-11-06
**Author:** Claude Code
**Purpose:** Standalone C++ port of Friture's 2D spectrogram visualization

---

## Executive Summary

This document outlines a plan for creating a standalone C++ application that replicates Friture's real-time 2D spectrogram visualization capabilities. The port will focus exclusively on spectrogram functionality, leveraging modern C++ libraries and techniques to achieve high performance and maintainability.

**Technology Stack:**
- **Graphics & Rendering:** SDL3 with GLSL shaders
- **UI Layout:** Clay (immediate-mode C layout library)
- **Audio Input:** PortAudio
- **Signal Processing:** Intel MKL for FFT (with Eigen as fallback)
- **Build System:** CMake
- **Testing:** Google Test with headless testing support

**Key Objectives:**
1. Real-time spectrogram rendering at 60+ FPS
2. Feature parity with Friture's spectrogram settings
3. Low-latency audio processing (<10ms)
4. Cross-platform support (Linux, Windows, macOS)
5. Maintainable, testable codebase

---

## 1. High-Level Specification

### 1.1 Functional Requirements

#### 1.1.1 Spectrogram Visualization
- **Time-Frequency Display:** Scrolling 2D spectrogram with time on X-axis, frequency on Y-axis, and amplitude represented by color
- **Real-time Update:** Smooth, jitter-free scrolling at display refresh rate
- **Color Mapping:** CMRMAP colormap (black→purple→red→yellow→white) with perceptually linear luminance

#### 1.1.2 Audio Input
- **Sample Rate:** 48 kHz (configurable)
- **Buffer Size:** 512 frames per buffer (configurable)
- **Channels:** Mono and stereo support (use first channel or average for stereo)
- **Input Device Selection:** List and select from available audio input devices
- **Ring Buffer:** Efficient circular buffer for continuous audio storage

#### 1.1.3 Signal Processing Settings

| Setting | Options | Default | Range/Values |
|---------|---------|---------|--------------|
| **FFT Size** | Power of 2 × 32 | 4096 | 32, 64, 128, 256, 512, 1024, 2048, 4096, 8192, 16384 |
| **Window Function** | Hann, Hamming | Hann | - |
| **Overlap** | Fixed | 75% | - |
| **Frequency Scale** | Linear, Log, Mel, ERB, Octave | Mel | - |
| **Min Frequency** | User-defined | 20 Hz | 10 Hz - Nyquist |
| **Max Frequency** | User-defined | 24000 Hz | Min Freq - Nyquist |
| **Amplitude Range Min** | dB scale | -140 dB | -200 dB to +200 dB |
| **Amplitude Range Max** | dB scale | 0 dB | -200 dB to +200 dB |
| **Time Range** | Display history | 10.0 sec | 0.1 sec - 1000 sec |
| **Weighting** | Psychoacoustic | None | None, A, B, C |

#### 1.1.4 User Interface
- **Settings Panel:**
  - Audio device selection dropdown
  - FFT size slider/dropdown
  - Frequency scale selector (radio buttons)
  - Min/Max frequency sliders
  - Amplitude range sliders (dB)
  - Time range slider
  - Weighting selector
  - Pause/Resume button
  - Reset to defaults button
- **Spectrogram Display:**
  - Resizable viewport
  - Frequency axis labels (Hz/kHz)
  - Amplitude scale bar with dB values
  - Time markers (optional)
  - FPS counter (debug mode)

#### 1.1.5 Performance Requirements
- **Frame Rate:** 60+ FPS for spectrogram rendering
- **Audio Latency:** <10ms from input to FFT processing
- **Memory Usage:** <50MB for typical operation (10s time range, 4096 FFT)
- **CPU Usage:** <25% on modern quad-core CPU (single-threaded processing)

### 1.2 Non-Functional Requirements

#### 1.2.1 Code Quality
- Modern C++17 or C++20 standards
- RAII for resource management
- Const-correctness throughout
- Minimal raw pointers (use smart pointers)
- Clear separation of concerns

#### 1.2.2 Testing
- Headless unit tests for all signal processing components
- Integration tests for audio pipeline
- Rendering tests with framebuffer capture
- Minimum 80% code coverage

#### 1.2.3 Documentation
- Doxygen-style API documentation
- Architecture diagrams
- Build instructions for all platforms
- Performance tuning guide

---

## 2. Architecture Design

### 2.1 Component Overview

```
┌─────────────────────────────────────────────────────────────┐
│                         Application                          │
├─────────────────────────────────────────────────────────────┤
│  ┌──────────────┐  ┌──────────────┐  ┌──────────────┐     │
│  │   UI Layer   │  │   Renderer   │  │ Audio Engine │     │
│  │   (Clay)     │  │  (SDL3+GLSL) │  │ (PortAudio)  │     │
│  └──────┬───────┘  └──────┬───────┘  └──────┬───────┘     │
│         │                  │                  │              │
│         │                  │                  │              │
│  ┌──────▼──────────────────▼──────────────────▼───────┐    │
│  │            Signal Processing Pipeline              │    │
│  │  ┌──────────┐  ┌──────────┐  ┌──────────┐        │    │
│  │  │   FFT    │→ │Frequency │→ │  Color   │        │    │
│  │  │ Processor│  │Resampler │  │Transform │        │    │
│  │  │  (MKL)   │  │          │  │          │        │    │
│  │  └──────────┘  └──────────┘  └──────────┘        │    │
│  └────────────────────────────────────────────────────┘    │
│                                                              │
│  ┌──────────────────────────────────────────────────────┐  │
│  │              Data Structures                          │  │
│  │  • RingBuffer    • SpectrogramImage                  │  │
│  │  • Settings      • ColorMap                          │  │
│  └──────────────────────────────────────────────────────┘  │
└─────────────────────────────────────────────────────────────┘
```

### 2.2 Core Components

#### 2.2.1 Audio Engine (`AudioEngine`)
- **Responsibilities:**
  - Initialize PortAudio
  - Manage audio input stream
  - Fill ring buffer with incoming audio data
  - Handle device enumeration and selection
  - Provide callback for real-time audio processing

- **Key Classes:**
  ```cpp
  class AudioEngine {
  public:
      AudioEngine(size_t sample_rate, size_t buffer_size);
      ~AudioEngine();

      void start();
      void stop();
      void setInputDevice(int device_id);
      std::vector<AudioDeviceInfo> getInputDevices();

      // Thread-safe access to audio data
      RingBuffer& getRingBuffer();

  private:
      static int audioCallback(const void* input, void* output,
                              unsigned long frameCount,
                              const PaStreamCallbackTimeInfo* timeInfo,
                              PaStreamCallbackFlags statusFlags,
                              void* userData);
  };
  ```

#### 2.2.2 Ring Buffer (`RingBuffer`)
- **Responsibilities:**
  - Lock-free circular buffer for audio samples
  - Thread-safe read/write operations
  - Efficient memory management with fixed allocation

- **Key Classes:**
  ```cpp
  template<typename T>
  class RingBuffer {
  public:
      RingBuffer(size_t capacity);

      // Write samples (from audio callback)
      void write(const T* data, size_t count);

      // Read samples at specific offset (for FFT)
      void read(size_t offset, T* output, size_t count) const;

      // Get current write position
      size_t getWritePosition() const;

  private:
      std::vector<T> buffer_;
      std::atomic<size_t> write_pos_;
      size_t capacity_;
  };
  ```

#### 2.2.3 FFT Processor (`FFTProcessor`)
- **Responsibilities:**
  - Apply window function (Hann, Hamming)
  - Compute real FFT using Intel MKL
  - Calculate power spectrum
  - Apply psychoacoustic weighting (A/B/C)
  - Convert to logarithmic scale (dB)

- **Key Classes:**
  ```cpp
  class FFTProcessor {
  public:
      FFTProcessor(size_t fft_size, WindowFunction window_type);
      ~FFTProcessor();

      // Process audio samples -> frequency spectrum
      void process(const float* input, float* output);

      // Update configuration
      void setFFTSize(size_t new_size);
      void setWindowFunction(WindowFunction type);
      void setWeighting(WeightingType type);

  private:
      void computeWindow();
      void computeWeighting();

      size_t fft_size_;
      std::vector<float> window_;
      std::vector<float> weighting_;

      // MKL FFT descriptor
      DFTI_DESCRIPTOR_HANDLE fft_handle_;
  };
  ```

#### 2.2.4 Frequency Resampler (`FrequencyResampler`)
- **Responsibilities:**
  - Map linear FFT bins to desired frequency scale
  - Implement multiple scale types (Linear, Log, Mel, ERB, Octave)
  - Interpolate spectrum to screen height

- **Key Classes:**
  ```cpp
  class FrequencyResampler {
  public:
      FrequencyResampler(FrequencyScale scale_type,
                        float min_freq, float max_freq,
                        size_t output_height);

      // Resample frequency axis: [fft_bins] -> [screen_height]
      void resample(const float* input, size_t input_size,
                   float* output);

      void setScale(FrequencyScale scale_type);
      void setFrequencyRange(float min_freq, float max_freq);

  private:
      void computeMapping();

      FrequencyScale scale_type_;
      std::vector<float> freq_mapping_; // Maps output index to input index
  };
  ```

#### 2.2.5 Color Transform (`ColorTransform`)
- **Responsibilities:**
  - Map normalized amplitude [0,1] to RGB color
  - Implement CMRMAP colormap
  - Fast lookup table for real-time performance

- **Key Classes:**
  ```cpp
  class ColorTransform {
  public:
      ColorTransform();

      // Convert normalized value to RGBA color
      uint32_t valueToColor(float normalized_value) const;

      // Batch conversion for entire column
      void transformColumn(const float* input, size_t height,
                          uint32_t* output);

  private:
      void generateCMRMAP();

      std::array<uint32_t, 256> color_lut_;
  };
  ```

#### 2.2.6 Spectrogram Image (`SpectrogramImage`)
- **Responsibilities:**
  - Maintain double-buffered ring buffer pixmap (2× screen width)
  - Write new spectrogram columns
  - Track read/write pointers for scrolling
  - Handle resize operations

- **Key Classes:**
  ```cpp
  class SpectrogramImage {
  public:
      SpectrogramImage(size_t width, size_t height);

      // Add new spectrogram column
      void addColumn(const uint32_t* column_data, size_t height);

      // Get pixel data for rendering
      const uint32_t* getPixelData() const;
      size_t getReadOffset() const;

      void resize(size_t new_width, size_t new_height);
      void clear();

  private:
      std::vector<uint32_t> pixels_;
      size_t width_;
      size_t height_;
      size_t write_offset_;
      float time_per_column_;
  };
  ```

#### 2.2.7 Renderer (`Renderer`)
- **Responsibilities:**
  - Initialize SDL3 and GPU context
  - Compile and manage GLSL shaders
  - Upload spectrogram texture to GPU
  - Render spectrogram with proper scrolling
  - Draw UI elements from Clay

- **Key Classes:**
  ```cpp
  class Renderer {
  public:
      Renderer(SDL_Window* window);
      ~Renderer();

      void render(const SpectrogramImage& image,
                 const Clay_RenderCommandArray& ui_commands);

      void updateSpectrogramTexture(const uint32_t* pixels,
                                    size_t width, size_t height);

  private:
      void initializeShaders();
      void renderSpectrogram();
      void renderUI(const Clay_RenderCommandArray& commands);

      SDL_GPUDevice* gpu_device_;
      SDL_GPUGraphicsPipeline* spectrogram_pipeline_;
      SDL_GPUTexture* spectrogram_texture_;
  };
  ```

#### 2.2.8 UI Layer (`UILayer`)
- **Responsibilities:**
  - Layout UI using Clay
  - Handle user input events
  - Update application settings
  - Generate Clay render commands

- **Key Classes:**
  ```cpp
  class UILayer {
  public:
      UILayer(SpectrogramSettings& settings);

      // Build UI layout and return render commands
      Clay_RenderCommandArray buildUI(int screen_width, int screen_height);

      // Handle SDL events
      void handleEvent(const SDL_Event& event);

  private:
      void buildSettingsPanel();
      void buildSpectrogramView();

      SpectrogramSettings& settings_;
      bool show_settings_;
  };
  ```

### 2.3 Data Flow

```
┌──────────────┐
│ Audio Device │
└──────┬───────┘
       │ Raw Audio (512 samples @ 48kHz)
       ▼
┌──────────────┐
│ RingBuffer   │  (Lock-free circular buffer)
└──────┬───────┘
       │ When enough samples accumulated
       ▼
┌──────────────┐
│FFTProcessor  │  (Window → FFT → Power → dB)
└──────┬───────┘
       │ Frequency spectrum [fft_size/2 + 1]
       ▼
┌──────────────┐
│FreqResampler │  (Interpolate to screen height)
└──────┬───────┘
       │ Resampled spectrum [screen_height]
       ▼
┌──────────────┐
│ColorTransform│  (Map to RGB values)
└──────┬───────┘
       │ RGBA column [screen_height]
       ▼
┌──────────────┐
│SpectrogramImg│  (Add to ringbuffer pixmap)
└──────┬───────┘
       │ 2D pixel array
       ▼
┌──────────────┐
│   Renderer   │  (Upload texture → Draw quad)
└──────────────┘
```

### 2.4 Memory Management Strategy

#### 2.4.1 Pre-Allocation
- All buffers allocated at initialization
- No dynamic allocation in audio callback or FFT processing
- Ring buffer size: `sample_rate * max_time_range`
- Spectrogram image: `2 * screen_width * screen_height * 4 bytes`

#### 2.4.2 Memory Budget (Example: 10s time range, 1920×1080 display)
```
RingBuffer:         48000 * 10 * 4 bytes        = 1.92 MB
FFT working memory: 16384 * 4 bytes * 2         = 0.13 MB
Spectrogram image:  2 * 1920 * 1080 * 4 bytes   = 16.6 MB
Window/Weighting:   16384 * 4 bytes * 2         = 0.13 MB
Frequency mapping:  1080 * 4 bytes              = 0.004 MB
Color LUT:          256 * 4 bytes               = 0.001 MB
                                        Total:   ≈ 18.8 MB
```

---

## 3. Implementation Plan (Test-Driven)

### Phase 1: Core Infrastructure (Week 1)

#### 1.1 Project Setup
- [ ] Create CMake build system
  - Configure dependencies (SDL3, PortAudio, MKL/Eigen, Clay, GTest)
  - Support Linux, Windows, macOS builds
  - Debug and Release configurations
- [ ] Set up directory structure
  ```
  friture-cpp/
  ├── src/
  │   ├── audio/
  │   ├── processing/
  │   ├── rendering/
  │   ├── ui/
  │   └── main.cpp
  ├── include/
  │   └── friture/
  ├── tests/
  │   ├── unit/
  │   └── integration/
  ├── shaders/
  │   ├── spectrogram.vert
  │   └── spectrogram.frag
  ├── third_party/
  │   └── clay/
  └── CMakeLists.txt
  ```

#### 1.2 RingBuffer Implementation
**Test-first approach:**
```cpp
// tests/unit/ringbuffer_test.cpp
TEST(RingBufferTest, WriteAndRead) {
    RingBuffer<float> buffer(1024);
    float data[512] = {1.0f, 2.0f, ...};
    buffer.write(data, 512);

    float output[512];
    buffer.read(0, output, 512);

    EXPECT_FLOAT_EQ(output[0], 1.0f);
}

TEST(RingBufferTest, WrapAround) {
    RingBuffer<float> buffer(1024);
    // Write 1500 samples, verify wrapping works
}

TEST(RingBufferTest, ThreadSafety) {
    // Concurrent write/read test
}
```

**Implementation steps:**
1. Write failing tests
2. Implement basic RingBuffer with std::vector backend
3. Add atomic operations for thread safety
4. Verify all tests pass
5. Benchmark performance (target: <1μs per 512-sample write)

#### 1.3 Settings Management
**Test-first approach:**
```cpp
TEST(SettingsTest, DefaultValues) {
    SpectrogramSettings settings;
    EXPECT_EQ(settings.fft_size, 4096);
    EXPECT_EQ(settings.freq_scale, FrequencyScale::Mel);
}

TEST(SettingsTest, Validation) {
    SpectrogramSettings settings;
    settings.setMinFrequency(100);
    settings.setMaxFrequency(50); // Invalid
    EXPECT_FALSE(settings.isValid());
}
```

**Implementation:**
```cpp
struct SpectrogramSettings {
    size_t fft_size = 4096;
    WindowFunction window_type = WindowFunction::Hann;
    FrequencyScale freq_scale = FrequencyScale::Mel;
    float min_freq = 20.0f;
    float max_freq = 24000.0f;
    float spec_min_db = -140.0f;
    float spec_max_db = 0.0f;
    float time_range = 10.0f;
    WeightingType weighting = WeightingType::None;

    bool isValid() const;
    void setFFTSize(size_t size);
    // ... other setters with validation
};
```

---

### Phase 2: Signal Processing Pipeline (Week 2-3)

#### 2.1 FFT Processor with MKL
**Test-first approach:**
```cpp
TEST(FFTProcessorTest, HannWindow) {
    FFTProcessor processor(1024, WindowFunction::Hann);

    // Verify window coefficients
    auto window = processor.getWindow();
    EXPECT_FLOAT_EQ(window[0], 0.0f);
    EXPECT_NEAR(window[512], 1.0f, 1e-5);
}

TEST(FFTProcessorTest, ImpulseResponse) {
    FFTProcessor processor(1024, WindowFunction::Hann);

    std::vector<float> impulse(1024, 0.0f);
    impulse[512] = 1.0f; // Delta function

    std::vector<float> output(513);
    processor.process(impulse.data(), output.data());

    // All frequency bins should have equal magnitude
    for (size_t i = 1; i < 513; ++i) {
        EXPECT_NEAR(output[i], output[0], 0.1f);
    }
}

TEST(FFTProcessorTest, SineWave) {
    FFTProcessor processor(4096, WindowFunction::Hann);

    // 1 kHz sine wave at 48 kHz sample rate
    std::vector<float> sine(4096);
    for (size_t i = 0; i < 4096; ++i) {
        sine[i] = std::sin(2.0f * M_PI * 1000.0f * i / 48000.0f);
    }

    std::vector<float> output(2049);
    processor.process(sine.data(), output.data());

    // Peak should be at bin closest to 1 kHz
    size_t expected_bin = 1000 * 4096 / 48000;
    auto max_it = std::max_element(output.begin(), output.end());
    size_t max_bin = std::distance(output.begin(), max_it);

    EXPECT_NEAR(max_bin, expected_bin, 2);
}

TEST(FFTProcessorTest, NoiseFloor) {
    FFTProcessor processor(4096, WindowFunction::Hann);

    std::vector<float> zeros(4096, 0.0f);
    std::vector<float> output(2049);
    processor.process(zeros.data(), output.data());

    // All bins should be below -140 dB
    for (float val : output) {
        EXPECT_LT(val, -130.0f); // In dB
    }
}
```

**Implementation steps:**
1. Integrate Intel MKL
   ```cpp
   #include <mkl_dfti.h>

   void FFTProcessor::initialize() {
       MKL_LONG status;
       status = DftiCreateDescriptor(&fft_handle_, DFTI_SINGLE,
                                     DFTI_REAL, 1, fft_size_);
       status = DftiSetValue(fft_handle_, DFTI_PLACEMENT,
                            DFTI_NOT_INPLACE);
       status = DftiCommitDescriptor(fft_handle_);
   }
   ```

2. Implement window functions
   ```cpp
   void FFTProcessor::computeHannWindow() {
       window_.resize(fft_size_);
       for (size_t i = 0; i < fft_size_; ++i) {
           window_[i] = 0.5f * (1.0f - std::cos(
               2.0f * M_PI * i / (fft_size_ - 1)));
       }
   }
   ```

3. Implement power spectrum calculation
   ```cpp
   void FFTProcessor::process(const float* input, float* output) {
       // Apply window
       std::vector<float> windowed(fft_size_);
       for (size_t i = 0; i < fft_size_; ++i) {
           windowed[i] = input[i] * window_[i];
       }

       // Compute FFT
       std::vector<float> fft_output(fft_size_ + 2);
       DftiComputeForward(fft_handle_, windowed.data(),
                         fft_output.data());

       // Power spectrum: |FFT|^2
       size_t num_bins = fft_size_ / 2 + 1;
       for (size_t i = 0; i < num_bins; ++i) {
           float real = fft_output[2*i];
           float imag = fft_output[2*i + 1];
           float power = (real*real + imag*imag) / (fft_size_ * fft_size_);

           // Convert to dB
           output[i] = 10.0f * std::log10(power + 1e-30f);
       }
   }
   ```

4. Implement psychoacoustic weighting
   ```cpp
   void FFTProcessor::computeAWeighting() {
       weighting_.resize(fft_size_ / 2 + 1);

       for (size_t i = 0; i < weighting_.size(); ++i) {
           float freq = i * sample_rate_ / fft_size_;

           // A-weighting formula
           float f2 = freq * freq;
           float num = 12194.0f * 12194.0f * f2 * f2;
           float den = (f2 + 20.6f*20.6f) *
                      std::sqrt((f2 + 107.7f*107.7f) *
                               (f2 + 737.9f*737.9f)) *
                      (f2 + 12194.0f*12194.0f);

           weighting_[i] = 20.0f * std::log10(num / den) + 2.0f;
       }
   }
   ```

#### 2.2 Frequency Resampler
**Test-first approach:**
```cpp
TEST(FrequencyResamplerTest, LinearScale) {
    FrequencyResampler resampler(FrequencyScale::Linear,
                                 20.0f, 20000.0f, 1080);

    std::vector<float> input(2049, -60.0f); // Flat spectrum
    std::vector<float> output(1080);

    resampler.resample(input.data(), input.size(), output.data());

    // Output should also be flat
    for (float val : output) {
        EXPECT_NEAR(val, -60.0f, 1.0f);
    }
}

TEST(FrequencyResamplerTest, MelScale) {
    FrequencyResampler resampler(FrequencyScale::Mel,
                                 20.0f, 20000.0f, 1080);

    // Mel scale should have more bins at low frequencies
    auto mapping = resampler.getFrequencyMapping();

    // Distance between bins should increase with frequency
    float low_delta = mapping[10] - mapping[9];
    float high_delta = mapping[1070] - mapping[1069];
    EXPECT_LT(low_delta, high_delta);
}

TEST(FrequencyResamplerTest, InterpolationAccuracy) {
    FrequencyResampler resampler(FrequencyScale::Linear,
                                 0.0f, 24000.0f, 100);

    // Create spectrum with single peak
    std::vector<float> input(2049, -100.0f);
    input[1000] = 0.0f; // Peak at ~11.7 kHz

    std::vector<float> output(100);
    resampler.resample(input.data(), input.size(), output.data());

    // Peak should be preserved after resampling
    auto max_it = std::max_element(output.begin(), output.end());
    EXPECT_NEAR(*max_it, 0.0f, 1.0f);
}
```

**Implementation:**
```cpp
class FrequencyResampler {
public:
    void resample(const float* input, size_t input_size,
                 float* output) {
        for (size_t i = 0; i < output_height_; ++i) {
            float input_idx = freq_mapping_[i];
            size_t idx0 = static_cast<size_t>(input_idx);
            size_t idx1 = std::min(idx0 + 1, input_size - 1);
            float frac = input_idx - idx0;

            // Linear interpolation
            output[i] = input[idx0] * (1.0f - frac) +
                       input[idx1] * frac;
        }
    }

private:
    void computeMelMapping() {
        auto hzToMel = [](float hz) {
            return 2595.0f * std::log10(1.0f + hz / 700.0f);
        };

        auto melToHz = [](float mel) {
            return 700.0f * (std::pow(10.0f, mel / 2595.0f) - 1.0f);
        };

        float min_mel = hzToMel(min_freq_);
        float max_mel = hzToMel(max_freq_);

        freq_mapping_.resize(output_height_);
        for (size_t i = 0; i < output_height_; ++i) {
            float mel = min_mel + (max_mel - min_mel) * i /
                       (output_height_ - 1);
            float hz = melToHz(mel);

            // Map Hz to FFT bin
            freq_mapping_[i] = hz * fft_size_ / sample_rate_;
        }
    }
};
```

#### 2.3 Color Transform
**Test-first approach:**
```cpp
TEST(ColorTransformTest, LUTGeneration) {
    ColorTransform transform;

    // Black at 0.0
    uint32_t black = transform.valueToColor(0.0f);
    EXPECT_EQ(black & 0xFF, 0); // B
    EXPECT_EQ((black >> 8) & 0xFF, 0); // G
    EXPECT_EQ((black >> 16) & 0xFF, 0); // R

    // White at 1.0
    uint32_t white = transform.valueToColor(1.0f);
    EXPECT_GT(white & 0xFF, 200); // Mostly white
}

TEST(ColorTransformTest, MonotonicLuminance) {
    ColorTransform transform;

    // Luminance should increase monotonically
    float prev_luminance = 0.0f;
    for (int i = 0; i < 256; ++i) {
        float val = i / 255.0f;
        uint32_t color = transform.valueToColor(val);

        uint8_t r = (color >> 16) & 0xFF;
        uint8_t g = (color >> 8) & 0xFF;
        uint8_t b = color & 0xFF;

        float luminance = 0.299f * r + 0.587f * g + 0.114f * b;
        EXPECT_GE(luminance, prev_luminance);
        prev_luminance = luminance;
    }
}
```

**Implementation:**
```cpp
void ColorTransform::generateCMRMAP() {
    // CMRMAP colormap coefficients
    // (Generated from matplotlib's cmrmap)

    for (size_t i = 0; i < 256; ++i) {
        float t = i / 255.0f;

        uint8_t r, g, b;
        if (t < 0.25f) {
            r = 0;
            g = 0;
            b = static_cast<uint8_t>(255 * t / 0.25f);
        } else if (t < 0.5f) {
            float local_t = (t - 0.25f) / 0.25f;
            r = static_cast<uint8_t>(128 * local_t);
            g = 0;
            b = static_cast<uint8_t>(255 * (1.0f - local_t * 0.5f));
        } else if (t < 0.75f) {
            float local_t = (t - 0.5f) / 0.25f;
            r = static_cast<uint8_t>(128 + 127 * local_t);
            g = static_cast<uint8_t>(128 * local_t);
            b = static_cast<uint8_t>(127 * (1.0f - local_t));
        } else {
            float local_t = (t - 0.75f) / 0.25f;
            r = 255;
            g = static_cast<uint8_t>(128 + 127 * local_t);
            b = static_cast<uint8_t>(255 * local_t);
        }

        color_lut_[i] = (0xFF << 24) | (r << 16) | (g << 8) | b;
    }
}
```

---

### Phase 3: Audio Engine (Week 3-4)

#### 3.1 PortAudio Integration
**Test-first approach:**
```cpp
TEST(AudioEngineTest, Initialization) {
    AudioEngine engine(48000, 512);
    EXPECT_NO_THROW(engine.start());
    EXPECT_NO_THROW(engine.stop());
}

TEST(AudioEngineTest, DeviceEnumeration) {
    AudioEngine engine(48000, 512);
    auto devices = engine.getInputDevices();
    EXPECT_GT(devices.size(), 0);
}

// Integration test with simulated audio
TEST(AudioEngineTest, SimulatedInput) {
    AudioEngine engine(48000, 512);

    // Use generator device or loopback
    engine.setInputDevice(SIMULATED_DEVICE_ID);
    engine.start();

    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    auto& buffer = engine.getRingBuffer();
    EXPECT_GT(buffer.getWritePosition(), 4800); // At least 100ms

    engine.stop();
}
```

**Implementation:**
```cpp
class AudioEngine {
public:
    AudioEngine(size_t sample_rate, size_t buffer_size)
        : sample_rate_(sample_rate),
          buffer_size_(buffer_size),
          ring_buffer_(sample_rate * 60) { // 60s buffer

        PaError err = Pa_Initialize();
        if (err != paNoError) {
            throw std::runtime_error("Failed to initialize PortAudio");
        }
    }

    void start() {
        PaStreamParameters input_params;
        input_params.device = Pa_GetDefaultInputDevice();
        input_params.channelCount = 1;
        input_params.sampleFormat = paFloat32;
        input_params.suggestedLatency =
            Pa_GetDeviceInfo(input_params.device)->
                defaultLowInputLatency;
        input_params.hostApiSpecificStreamInfo = nullptr;

        PaError err = Pa_OpenStream(
            &stream_,
            &input_params,
            nullptr, // No output
            sample_rate_,
            buffer_size_,
            paClipOff,
            &AudioEngine::audioCallback,
            this
        );

        if (err != paNoError) {
            throw std::runtime_error("Failed to open stream");
        }

        Pa_StartStream(stream_);
    }

private:
    static int audioCallback(const void* input, void* output,
                            unsigned long frameCount,
                            const PaStreamCallbackTimeInfo* timeInfo,
                            PaStreamCallbackFlags statusFlags,
                            void* userData) {
        auto* engine = static_cast<AudioEngine*>(userData);
        const float* in = static_cast<const float*>(input);

        engine->ring_buffer_.write(in, frameCount);

        return paContinue;
    }

    PaStream* stream_ = nullptr;
    size_t sample_rate_;
    size_t buffer_size_;
    RingBuffer<float> ring_buffer_;
};
```

#### 3.2 End-to-End Signal Processing Test
```cpp
TEST(PipelineIntegrationTest, SineWaveToSpectrogram) {
    // Setup
    size_t fft_size = 4096;
    size_t sample_rate = 48000;

    FFTProcessor fft_proc(fft_size, WindowFunction::Hann);
    FrequencyResampler freq_res(FrequencyScale::Linear,
                                0.0f, 24000.0f, 1080);
    ColorTransform color_trans;
    SpectrogramImage image(1920, 1080);

    // Generate 1 kHz sine wave
    std::vector<float> sine(fft_size);
    for (size_t i = 0; i < fft_size; ++i) {
        sine[i] = std::sin(2.0f * M_PI * 1000.0f * i / sample_rate);
    }

    // Process pipeline
    std::vector<float> spectrum(fft_size / 2 + 1);
    fft_proc.process(sine.data(), spectrum.data());

    std::vector<float> resampled(1080);
    freq_res.resample(spectrum.data(), spectrum.size(),
                     resampled.data());

    // Normalize to [0, 1]
    std::vector<float> normalized(1080);
    for (size_t i = 0; i < 1080; ++i) {
        normalized[i] = (resampled[i] + 140.0f) / 140.0f;
        normalized[i] = std::clamp(normalized[i], 0.0f, 1.0f);
    }

    std::vector<uint32_t> colors(1080);
    color_trans.transformColumn(normalized.data(), 1080,
                               colors.data());

    image.addColumn(colors.data(), 1080);

    // Verify: 1 kHz should map to specific pixel row
    size_t expected_row = 1000 * 1080 / 24000; // ~45

    // Check that this row has high amplitude (bright color)
    uint32_t color_at_peak = colors[expected_row];
    uint8_t r = (color_at_peak >> 16) & 0xFF;
    EXPECT_GT(r, 128); // Should be bright
}
```

---

### Phase 4: Rendering System (Week 4-5)

#### 4.1 SDL3 + GLSL Setup
**Shaders:**

`shaders/spectrogram.vert`
```glsl
#version 450

layout(location = 0) in vec2 in_position;
layout(location = 1) in vec2 in_texcoord;

layout(location = 0) out vec2 frag_texcoord;

void main() {
    gl_Position = vec4(in_position, 0.0, 1.0);
    frag_texcoord = in_texcoord;
}
```

`shaders/spectrogram.frag`
```glsl
#version 450

layout(location = 0) in vec2 frag_texcoord;
layout(location = 0) out vec4 out_color;

layout(binding = 0) uniform sampler2D spectrogram_texture;

layout(push_constant) uniform PushConstants {
    float scroll_offset;
    float time_range;
} constants;

void main() {
    // Apply horizontal scroll offset for ring buffer reading
    vec2 uv = frag_texcoord;
    uv.x = mod(uv.x + constants.scroll_offset, 1.0);

    out_color = texture(spectrogram_texture, uv);
}
```

**Test approach:**
```cpp
TEST(RendererTest, Initialization) {
    SDL_Init(SDL_INIT_VIDEO);
    SDL_Window* window = SDL_CreateWindow("Test",
        800, 600, SDL_WINDOW_HIDDEN);

    EXPECT_NO_THROW({
        Renderer renderer(window);
    });

    SDL_DestroyWindow(window);
    SDL_Quit();
}

// Framebuffer capture test
TEST(RendererTest, RenderOutput) {
    // Create offscreen renderer
    // Render solid color
    // Read back pixels and verify
}
```

**Implementation:**
```cpp
class Renderer {
public:
    Renderer(SDL_Window* window) : window_(window) {
        // Create GPU device
        gpu_device_ = SDL_CreateGPUDevice(
            SDL_GPU_SHADERFORMAT_SPIRV,
            true, // Debug mode
            nullptr
        );

        if (!gpu_device_) {
            throw std::runtime_error("Failed to create GPU device");
        }

        SDL_ClaimWindowForGPUDevice(gpu_device_, window_);

        initializeShaders();
        createSpectrogramTexture();
    }

private:
    void initializeShaders() {
        // Load and compile shaders
        auto vert_shader = loadShader("shaders/spectrogram.vert.spv");
        auto frag_shader = loadShader("shaders/spectrogram.frag.spv");

        // Create graphics pipeline
        SDL_GPUGraphicsPipelineCreateInfo pipeline_info = {};
        // ... configure pipeline

        spectrogram_pipeline_ = SDL_CreateGPUGraphicsPipeline(
            gpu_device_, &pipeline_info);
    }

    void renderSpectrogram() {
        SDL_GPUCommandBuffer* cmd =
            SDL_AcquireGPUCommandBuffer(gpu_device_);

        SDL_GPUTexture* swapchain_texture =
            SDL_AcquireGPUSwapchainTexture(cmd, window_,
                                          nullptr, nullptr);

        if (swapchain_texture) {
            SDL_GPURenderPass* pass = SDL_BeginGPURenderPass(
                cmd, &color_target_info, nullptr);

            SDL_BindGPUGraphicsPipeline(pass, spectrogram_pipeline_);
            SDL_BindGPUFragmentSamplers(pass, 0, &sampler_binding, 1);

            // Set push constants for scroll offset
            SDL_PushGPUFragmentUniformData(cmd, 0,
                &scroll_offset_, sizeof(float));

            SDL_DrawGPUPrimitives(pass, 6, 1, 0, 0); // Quad

            SDL_EndGPURenderPass(pass);
        }

        SDL_SubmitGPUCommandBuffer(cmd);
    }

    SDL_GPUDevice* gpu_device_;
    SDL_Window* window_;
    SDL_GPUGraphicsPipeline* spectrogram_pipeline_;
    SDL_GPUTexture* spectrogram_texture_;
    float scroll_offset_ = 0.0f;
};
```

#### 4.2 Clay UI Integration
**Test approach:**
```cpp
TEST(UILayerTest, LayoutGeneration) {
    SpectrogramSettings settings;
    UILayer ui(settings);

    auto commands = ui.buildUI(1920, 1080);

    EXPECT_GT(Clay_RenderCommandArray_Get(&commands)->length, 0);
}

TEST(UILayerTest, InteractionHandling) {
    SpectrogramSettings settings;
    UILayer ui(settings);

    SDL_Event event;
    event.type = SDL_EVENT_MOUSE_BUTTON_DOWN;
    event.button.x = 100;
    event.button.y = 200;

    ui.handleEvent(event);

    // Verify settings changed if button was clicked
}
```

**Implementation:**
```cpp
class UILayer {
public:
    Clay_RenderCommandArray buildUI(int screen_width,
                                    int screen_height) {
        Clay_BeginLayout(screen_width, screen_height);

        buildMainContainer();

        return Clay_EndLayout();
    }

private:
    void buildMainContainer() {
        CLAY_CONTAINER(
            CLAY_ID("MainContainer"),
            CLAY_LAYOUT({
                .sizing = {
                    .width = CLAY_SIZING_GROW(),
                    .height = CLAY_SIZING_GROW()
                },
                .layoutDirection = CLAY_LEFT_TO_RIGHT
            })
        ) {
            buildSpectrogramView();
            buildSettingsPanel();
        }
    }

    void buildSettingsPanel() {
        CLAY_CONTAINER(
            CLAY_ID("SettingsPanel"),
            CLAY_LAYOUT({
                .sizing = {
                    .width = CLAY_SIZING_FIXED(300),
                    .height = CLAY_SIZING_GROW()
                },
                .padding = {16, 16, 16, 16},
                .childGap = 12
            })
        ) {
            buildFFTSizeSlider();
            buildFrequencyScaleSelector();
            buildFrequencyRangeSliders();
            // ... more controls
        }
    }
};
```

---

### Phase 5: Integration and Optimization (Week 5-6)

#### 5.1 Main Application Loop
```cpp
class FritureApp {
public:
    FritureApp()
        : audio_engine_(SAMPLE_RATE, BUFFER_SIZE),
          fft_processor_(settings_.fft_size, WindowFunction::Hann),
          freq_resampler_(settings_.freq_scale,
                         settings_.min_freq, settings_.max_freq, 1080),
          spectrogram_image_(1920, 1080),
          ui_layer_(settings_) {

        SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO);

        window_ = SDL_CreateWindow("Friture C++",
            1920, 1080, SDL_WINDOW_RESIZABLE);

        renderer_ = std::make_unique<Renderer>(window_);

        audio_engine_.start();
    }

    void run() {
        bool running = true;
        auto last_fft_time = std::chrono::steady_clock::now();

        while (running) {
            // Handle events
            SDL_Event event;
            while (SDL_PollEvent(&event)) {
                if (event.type == SDL_EVENT_QUIT) {
                    running = false;
                }
                ui_layer_.handleEvent(event);
            }

            // Process audio if enough samples available
            auto now = std::chrono::steady_clock::now();
            auto elapsed = std::chrono::duration_cast<
                std::chrono::milliseconds>(now - last_fft_time);

            size_t samples_per_column = settings_.fft_size / 4; // 75% overlap
            float ms_per_column = samples_per_column * 1000.0f /
                                 SAMPLE_RATE;

            if (elapsed.count() >= ms_per_column) {
                processAudioFrame();
                last_fft_time = now;
            }

            // Render
            auto ui_commands = ui_layer_.buildUI(1920, 1080);
            renderer_->render(spectrogram_image_, ui_commands);

            // Frame limiting
            SDL_Delay(1); // ~60 FPS
        }
    }

private:
    void processAudioFrame() {
        auto& buffer = audio_engine_.getRingBuffer();
        size_t read_pos = buffer.getWritePosition() -
                         settings_.fft_size;

        std::vector<float> samples(settings_.fft_size);
        buffer.read(read_pos, samples.data(), settings_.fft_size);

        // FFT
        std::vector<float> spectrum(settings_.fft_size / 2 + 1);
        fft_processor_.process(samples.data(), spectrum.data());

        // Frequency resample
        std::vector<float> resampled(1080);
        freq_resampler_.resample(spectrum.data(), spectrum.size(),
                                resampled.data());

        // Normalize
        std::vector<float> normalized(1080);
        for (size_t i = 0; i < 1080; ++i) {
            normalized[i] = (resampled[i] - settings_.spec_min_db) /
                          (settings_.spec_max_db - settings_.spec_min_db);
            normalized[i] = std::clamp(normalized[i], 0.0f, 1.0f);
        }

        // Color transform
        std::vector<uint32_t> colors(1080);
        color_transform_.transformColumn(normalized.data(), 1080,
                                        colors.data());

        // Add to image
        spectrogram_image_.addColumn(colors.data(), 1080);

        // Update texture
        renderer_->updateSpectrogramTexture(
            spectrogram_image_.getPixelData(),
            spectrogram_image_.getWidth(),
            spectrogram_image_.getHeight()
        );
    }

    SpectrogramSettings settings_;
    AudioEngine audio_engine_;
    FFTProcessor fft_processor_;
    FrequencyResampler freq_resampler_;
    ColorTransform color_transform_;
    SpectrogramImage spectrogram_image_;
    UILayer ui_layer_;

    SDL_Window* window_;
    std::unique_ptr<Renderer> renderer_;
};
```

#### 5.2 Performance Testing
```cpp
TEST(PerformanceTest, FFTThroughput) {
    FFTProcessor processor(4096, WindowFunction::Hann);

    std::vector<float> input(4096);
    std::vector<float> output(2049);

    auto start = std::chrono::high_resolution_clock::now();

    const int iterations = 10000;
    for (int i = 0; i < iterations; ++i) {
        processor.process(input.data(), output.data());
    }

    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<
        std::chrono::microseconds>(end - start);

    float avg_us = duration.count() / static_cast<float>(iterations);

    // Should be < 100 μs per FFT (4096 points)
    EXPECT_LT(avg_us, 100.0f);

    std::cout << "FFT avg: " << avg_us << " μs\n";
}

TEST(PerformanceTest, EndToEndLatency) {
    // Measure audio input → spectrogram update latency
    // Target: < 10ms
}
```

#### 5.3 Optimization Checklist
- [ ] Profile with perf/VTune to identify hotspots
- [ ] SIMD optimization for color transform (AVX2/AVX-512)
- [ ] Multi-threading considerations:
  - Audio callback in separate thread (PortAudio handles this)
  - FFT processing in main thread (avoid locks)
  - Rendering in main thread
- [ ] Memory access patterns:
  - Prefer sequential access
  - Cache-align large buffers
  - Minimize cache misses in hot loops
- [ ] GPU optimization:
  - Use persistent-mapped buffers for texture updates
  - Minimize state changes
  - Batch UI draws

---

## 4. Testing Strategy

### 4.1 Unit Tests (Google Test)

**Coverage targets:**
- Signal processing: 90%+
- Data structures: 95%+
- Settings validation: 100%

**Headless testing approach:**
```cpp
// Example: FFT without audio hardware
TEST(FFTProcessorTest, OfflineProcessing) {
    // Generate synthetic signals
    // Process with FFT
    // Verify output mathematically
}
```

### 4.2 Integration Tests

**Audio pipeline test:**
```cpp
TEST(IntegrationTest, AudioToSpectrogram) {
    // Use WAV file as input instead of live audio
    // Process through full pipeline
    // Capture output spectrogram image
    // Compare against reference image (PSNR > 30 dB)
}
```

### 4.3 Rendering Tests

**Framebuffer capture:**
```cpp
TEST(RenderingTest, SpectrogramOutput) {
    // Create offscreen SDL context
    // Render known spectrogram data
    // Read back framebuffer
    // Verify pixels match expected pattern
}
```

### 4.4 Performance Benchmarks

**Continuous benchmarking:**
- FFT processing time (target: <100 μs for 4096 points)
- Full pipeline latency (target: <10 ms)
- Frame render time (target: <16 ms for 60 FPS)
- Memory usage (target: <50 MB)

---

## 5. Dependencies and Build System

### 5.1 CMake Configuration

**CMakeLists.txt structure:**
```cmake
cmake_minimum_required(VERSION 3.20)
project(friture-cpp VERSION 1.0.0 LANGUAGES CXX C)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Options
option(USE_MKL "Use Intel MKL for FFT" ON)
option(BUILD_TESTS "Build tests" ON)

# Dependencies
find_package(SDL3 REQUIRED)
find_package(PortAudio REQUIRED)

if(USE_MKL)
    find_package(MKL REQUIRED)
else()
    find_package(Eigen3 REQUIRED)
endif()

if(BUILD_TESTS)
    find_package(GTest REQUIRED)
    enable_testing()
endif()

# Clay (header-only, bundled)
add_library(clay INTERFACE)
target_include_directories(clay INTERFACE
    ${CMAKE_SOURCE_DIR}/third_party/clay)

# Main library
add_library(friture-core
    src/audio/audio_engine.cpp
    src/audio/ring_buffer.cpp
    src/processing/fft_processor.cpp
    src/processing/frequency_resampler.cpp
    src/processing/color_transform.cpp
    src/rendering/renderer.cpp
    src/rendering/spectrogram_image.cpp
    src/ui/ui_layer.cpp
)

target_include_directories(friture-core PUBLIC
    ${CMAKE_SOURCE_DIR}/include)

target_link_libraries(friture-core
    SDL3::SDL3
    PortAudio::PortAudio
    clay
    $<IF:$<BOOL:${USE_MKL}>,MKL::MKL,Eigen3::Eigen>
)

# Executable
add_executable(friture src/main.cpp)
target_link_libraries(friture friture-core)

# Tests
if(BUILD_TESTS)
    add_subdirectory(tests)
endif()
```

### 5.2 Dependency Details

| Dependency | Version | Purpose | License |
|------------|---------|---------|---------|
| **SDL3** | 3.x (preview) | Graphics, window, input | Zlib |
| **PortAudio** | v19.7+ | Audio I/O | MIT-like |
| **Intel MKL** | 2024+ | FFT (primary) | Intel Simplified |
| **Eigen** | 3.4+ | FFT (fallback) | MPL2 |
| **Clay** | Latest | UI layout | Zlib |
| **Google Test** | 1.14+ | Testing | BSD-3 |

**Platform-specific notes:**
- **Linux:** apt install libsdl3-dev portaudio19-dev
- **Windows:** vcpkg install sdl3 portaudio
- **macOS:** brew install sdl3 portaudio

### 5.3 Shader Compilation

**Compile GLSL to SPIR-V:**
```bash
glslangValidator -V shaders/spectrogram.vert -o spectrogram.vert.spv
glslangValidator -V shaders/spectrogram.frag -o spectrogram.frag.spv
```

Or use SDL_shadercross for multi-backend support:
```bash
shadercross -i spectrogram.vert -o spectrogram.vert.spv --spirv
```

---

## 6. Future Considerations

### 6.1 Features Not in Scope (v1.0)
- Other visualization types (oscilloscope, octave spectrum, etc.)
- Audio output/playback
- Recording/export functionality
- Plugin system
- Network streaming

### 6.2 Potential Enhancements (v2.0+)
- **GPU-accelerated FFT:** cuFFT (NVIDIA) or clFFT (OpenCL)
- **Advanced frequency scales:** Bark, ERB-rate, custom
- **Annotation tools:** Frequency markers, harmonic cursors
- **Multi-channel support:** Stereo spectrogram, phase visualization
- **Configuration persistence:** Save/load settings
- **Themes:** Custom color maps, dark/light UI modes

### 6.3 Cross-Platform Packaging
- **Linux:** AppImage, Flatpak
- **Windows:** MSI installer, portable EXE
- **macOS:** DMG with code signing

---

## 7. Success Criteria

### 7.1 Functional
- ✅ All spectrogram settings implemented and functional
- ✅ Smooth scrolling at 60 FPS
- ✅ <10ms audio latency
- ✅ No audio dropouts or glitches
- ✅ Accurate frequency representation (verified with tone generators)

### 7.2 Quality
- ✅ 80%+ test coverage
- ✅ Zero memory leaks (Valgrind clean)
- ✅ No compiler warnings (-Wall -Wextra)
- ✅ Clang-tidy clean
- ✅ Documentation complete

### 7.3 Performance
- ✅ CPU usage <25% on reference hardware (Intel i5-10600K)
- ✅ Memory usage <50 MB
- ✅ Startup time <2 seconds

---

## 8. Development Timeline

| Phase | Duration | Deliverables |
|-------|----------|--------------|
| **Phase 1: Infrastructure** | 1 week | Build system, RingBuffer, Settings |
| **Phase 2: Signal Processing** | 2 weeks | FFT, Frequency resampler, Color transform |
| **Phase 3: Audio Engine** | 1 week | PortAudio integration, pipeline tests |
| **Phase 4: Rendering** | 1 week | SDL3 renderer, GLSL shaders, Clay UI |
| **Phase 5: Integration** | 1 week | Full app, optimization, polish |
| **Testing & Documentation** | 1 week | Test coverage, docs, packaging |
| **Total** | **7 weeks** | Production-ready v1.0 |

---

## 9. References

### 9.1 Friture Architecture
- Original Python implementation: `/home/user/friture/friture/`
- Key files analyzed:
  - `spectrogram.py` - Main widget
  - `audioproc.py` - FFT processing
  - `frequency_resampler.py` - Scale conversion
  - `color_tranform.py` - CMRMAP
  - `spectrogram_image.py` - Ring buffer rendering

### 9.2 Technical Documentation
- **SDL3 GPU API:** https://wiki.libsdl.org/SDL3/CategoryGPU
- **PortAudio Tutorial:** http://www.portaudio.com/docs/
- **Intel MKL FFT:** https://www.intel.com/content/www/us/en/developer/tools/oneapi/onemkl.html
- **Clay UI:** https://github.com/nicbarker/clay
- **Eigen FFT:** https://eigen.tuxfamily.org/dox/unsupported/group__FFT__Module.html

### 9.3 Signal Processing Resources
- Window functions: https://en.wikipedia.org/wiki/Window_function
- Mel scale: https://en.wikipedia.org/wiki/Mel_scale
- ERB scale: https://en.wikipedia.org/wiki/Equivalent_rectangular_bandwidth
- A-weighting: https://en.wikipedia.org/wiki/A-weighting

---

**End of Document**

## Appendix A: Glossary

- **FFT:** Fast Fourier Transform - algorithm for computing DFT efficiently
- **DFT:** Discrete Fourier Transform - converts time-domain signal to frequency domain
- **Spectrogram:** Visual representation of spectrum over time (time-frequency plot)
- **Window function:** Tapering function applied before FFT to reduce spectral leakage
- **Overlap:** Amount of shared samples between consecutive FFT frames
- **Mel scale:** Perceptually-motivated frequency scale (linear below 1 kHz, log above)
- **ERB:** Equivalent Rectangular Bandwidth - psychoacoustic frequency scale
- **Ring buffer:** Circular buffer data structure for continuous streaming data
- **CMRMAP:** Colormap designed for monochrome readability
- **Nyquist frequency:** Half the sampling rate (maximum representable frequency)

## Appendix B: Code Style Guidelines

```cpp
// Naming conventions
class MyClass;                    // PascalCase for classes
void myFunction();                // camelCase for functions
int my_variable_;                 // snake_case with trailing _ for members
const int MY_CONSTANT = 42;       // UPPER_CASE for constants

// Formatting
// - 4 spaces indentation
// - Braces on same line for functions, next line for classes
// - Maximum 100 characters per line
// - Always use { } for conditionals, even single-line

// Comments
/// Brief description of class/function (Doxygen)
/**
 * Detailed multi-line description
 * @param input The input buffer
 * @return Processing result
 */
```
