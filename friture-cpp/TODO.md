# Friture C++ Port - Implementation Roadmap

**Status:** Infrastructure Complete ✅
**Next Phase:** Phase 1 - Core Infrastructure Implementation

---

## 🚀 Immediate Next Steps (Ready to Start)

### PR #1: Fix Dependency Linking Issues
**Size:** Small (~50 lines)
**Time:** 30 minutes
**Priority:** High

- [ ] Fix FFTW3 float linking in `examples/CMakeLists.txt`
  - Add `-lfftw3f` to test_fft target
- [ ] Test that all example programs build successfully
- [ ] Update CLAUDE.md with validated fixes
- [ ] Run `./examples/test_fft` and document performance results

**Acceptance Criteria:**
- All example programs compile without errors
- `test_fft` runs and shows FFTW3 vs Eigen benchmark

---

### PR #2: RingBuffer Template Class - Core Implementation
**Size:** Medium (~300 lines)
**Time:** 3-4 hours
**Priority:** High
**Dependencies:** None

**Files to Create:**
```
include/friture/ringbuffer.hpp
tests/unit/ringbuffer_test.cpp
```

**Implementation Tasks:**
- [ ] Create `RingBuffer<T>` template class
  - Constructor with capacity parameter
  - `write(const T* data, size_t count)` method
  - `read(size_t offset, T* output, size_t count)` method
  - `getWritePosition()` method
  - Private: `std::vector<T> buffer_`
  - Private: `std::atomic<size_t> write_pos_`
  - Private: `size_t capacity_`

- [ ] Write unit tests (TDD approach):
  - Test basic write and read
  - Test wrap-around behavior
  - Test reading at different offsets
  - Test buffer boundaries

**Acceptance Criteria:**
- All unit tests pass
- No memory leaks (valgrind clean)
- Documentation comments (Doxygen style)

---

### PR #3: RingBuffer Thread Safety & Performance
**Size:** Small (~150 lines)
**Time:** 2 hours
**Priority:** Medium
**Dependencies:** PR #2

**Implementation Tasks:**
- [ ] Add thread safety tests
  - Concurrent write/read from different threads
  - Test with std::thread
  - Verify no race conditions

- [ ] Add performance benchmarks
  - Measure write latency (target: <1μs for 512 samples)
  - Test with different buffer sizes
  - Compare against Python version if possible

**Acceptance Criteria:**
- Thread safety tests pass with ThreadSanitizer
- Performance meets target (<1μs per 512-sample write)
- Benchmark results documented

---

### PR #4: Settings Management - Data Structures
**Size:** Medium (~250 lines)
**Time:** 2-3 hours
**Priority:** High
**Dependencies:** None

**Files to Create:**
```
include/friture/types.hpp
include/friture/settings.hpp
tests/unit/settings_test.cpp
```

**Implementation Tasks:**
- [ ] Define enum types in `types.hpp`:
  ```cpp
  enum class WindowFunction { Hann, Hamming };
  enum class FrequencyScale { Linear, Logarithmic, Mel, ERB, Octave };
  enum class WeightingType { None, A, B, C };
  ```

- [ ] Create `SpectrogramSettings` struct:
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
      void setFrequencyRange(float min, float max);
  };
  ```

- [ ] Write validation tests:
  - Test default values
  - Test valid ranges
  - Test invalid inputs (min > max, etc.)
  - Test setters update correctly

**Acceptance Criteria:**
- All validation logic works correctly
- Unit tests have 100% coverage
- Settings can be serialized (future: JSON)

---

## 📦 Phase 2: Signal Processing Pipeline (Next Sprint)

### PR #5: FFT Processor - Basic Implementation
**Size:** Large (~400 lines)
**Time:** 4-6 hours
**Priority:** High
**Dependencies:** PR #2, PR #4

**Files to Create:**
```
include/friture/fft_processor.hpp
src/processing/fft_processor.cpp
tests/unit/fft_processor_test.cpp
```

**Implementation Tasks:**
- [ ] Create `FFTProcessor` class
  - Constructor with FFT size and window type
  - `process(const float* input, float* output)` method
  - Window function generation (Hann, Hamming)
  - FFTW3 integration
  - Power spectrum calculation
  - dB conversion

- [ ] Write unit tests:
  - Test impulse response
  - Test sine wave (verify peak at correct frequency)
  - Test noise floor
  - Compare against Python implementation

**Acceptance Criteria:**
- FFT produces correct frequency spectrum
- Passes all mathematical correctness tests
- Performance: <100μs for 4096-point FFT

---

### PR #6: FFT Processor - Psychoacoustic Weighting
**Size:** Medium (~200 lines)
**Time:** 2-3 hours
**Priority:** Medium
**Dependencies:** PR #5

**Implementation Tasks:**
- [ ] Implement A-weighting calculation
- [ ] Implement B-weighting calculation
- [ ] Implement C-weighting calculation
- [ ] Add tests for each weighting type
- [ ] Validate against known standards (IEC 61672)

**Acceptance Criteria:**
- Weighting curves match standards (±0.5 dB)
- Tests validate against reference values

---

### PR #7: Frequency Resampler - Linear Scale
**Size:** Medium (~250 lines)
**Time:** 3 hours
**Priority:** High
**Dependencies:** PR #5

**Files to Create:**
```
include/friture/frequency_resampler.hpp
src/processing/frequency_resampler.cpp
tests/unit/frequency_resampler_test.cpp
```

**Implementation Tasks:**
- [ ] Create `FrequencyResampler` class
- [ ] Implement linear scale mapping
- [ ] Implement linear interpolation
- [ ] Write tests for flat spectrum
- [ ] Write tests for peak preservation

**Acceptance Criteria:**
- Linear scale works correctly
- Interpolation preserves peaks
- Tests pass

---

### PR #8: Frequency Resampler - Mel/ERB/Octave Scales
**Size:** Medium (~300 lines)
**Time:** 3-4 hours
**Priority:** High
**Dependencies:** PR #7

**Implementation Tasks:**
- [ ] Implement Mel scale transformation
- [ ] Implement ERB scale transformation
- [ ] Implement Logarithmic scale
- [ ] Implement Octave scale
- [ ] Write tests for each scale
- [ ] Validate against Python implementation

**Acceptance Criteria:**
- All scales produce correct frequency mapping
- Tests validate mathematical correctness
- Matches Python output

---

### PR #9: Color Transform - CMRMAP Implementation
**Size:** Small (~200 lines)
**Time:** 2 hours
**Priority:** Medium
**Dependencies:** None (can run in parallel)

**Files to Create:**
```
include/friture/color_transform.hpp
src/processing/color_transform.cpp
tests/unit/color_transform_test.cpp
```

**Implementation Tasks:**
- [ ] Generate CMRMAP lookup table (256 entries)
- [ ] Implement `valueToColor()` method
- [ ] Implement `transformColumn()` batch method
- [ ] Test monotonic luminance
- [ ] Test color endpoints (black at 0, white at 1)

**Acceptance Criteria:**
- Colormap matches Python version
- Luminance increases monotonically
- Fast lookup (<1μs per pixel)

---

## 🎨 Phase 3: Rendering & Audio (Sprint 3)

### PR #10: Spectrogram Image - Ring Buffer Pixmap
**Size:** Medium (~300 lines)
**Time:** 3-4 hours
**Priority:** High
**Dependencies:** PR #9

**Files to Create:**
```
include/friture/spectrogram_image.hpp
src/rendering/spectrogram_image.cpp
tests/unit/spectrogram_image_test.cpp
```

**Implementation Tasks:**
- [ ] Create double-buffered pixmap (2× width)
- [ ] Implement `addColumn()` method
- [ ] Track write offset for scrolling
- [ ] Handle resize operations
- [ ] Write tests

**Acceptance Criteria:**
- Ring buffer pixmap works correctly
- No visual artifacts
- Tests pass

---

### PR #11: Audio Engine - PortAudio Integration
**Size:** Large (~400 lines)
**Time:** 4-6 hours
**Priority:** High
**Dependencies:** PR #2

**Files to Create:**
```
include/friture/audio_engine.hpp
src/audio/audio_engine.cpp
tests/integration/audio_engine_test.cpp
```

**Implementation Tasks:**
- [ ] Implement PortAudio initialization
- [ ] Device enumeration
- [ ] Audio stream management
- [ ] Ring buffer integration
- [ ] Error handling
- [ ] Integration tests (with file input)

**Acceptance Criteria:**
- Can enumerate audio devices
- Can capture audio (when available)
- Works with simulated input for testing
- Latency <10ms

---

### PR #12: SDL2 Renderer - Basic Setup
**Size:** Large (~500 lines)
**Time:** 6-8 hours
**Priority:** High
**Dependencies:** PR #10

**Files to Create:**
```
include/friture/renderer.hpp
src/rendering/renderer.cpp
shaders/spectrogram.vert
shaders/spectrogram.frag
tests/integration/renderer_test.cpp
```

**Implementation Tasks:**
- [ ] Initialize SDL2 + GPU context
- [ ] Load and compile GLSL shaders
- [ ] Create texture for spectrogram
- [ ] Implement texture upload
- [ ] Render quad with scrolling
- [ ] Headless testing with framebuffer capture

**Acceptance Criteria:**
- Renders at 60 FPS
- Scrolling is smooth
- Tests can capture screenshots
- Works in headless mode (xvfb)

---

## 🏗️ Phase 4: UI & Integration (Sprint 4)

### PR #13: Clay UI Integration
**Size:** Large (~600 lines)
**Time:** 8-10 hours
**Priority:** Medium
**Dependencies:** PR #12

**Tasks:**
- [ ] Download and integrate Clay library
- [ ] Create UI layout for settings panel
- [ ] Implement sliders and controls
- [ ] Handle input events
- [ ] Generate render commands
- [ ] Tests for UI interactions

---

### PR #14: Main Application - Signal Pipeline
**Size:** Large (~500 lines)
**Time:** 6-8 hours
**Priority:** High
**Dependencies:** PR #5-12

**Files to Create:**
```
src/main.cpp
include/friture/application.hpp
src/application.cpp
```

**Implementation Tasks:**
- [ ] Create main application class
- [ ] Connect audio → FFT → resampling → color → image
- [ ] Implement main loop
- [ ] Handle timing and frame limiting
- [ ] Command-line arguments
- [ ] Integration tests

---

### PR #15: Performance Optimization
**Size:** Medium (~300 lines)
**Time:** 4-6 hours
**Priority:** Medium
**Dependencies:** PR #14

**Tasks:**
- [ ] Profile with perf/VTune
- [ ] SIMD optimization for color transform
- [ ] Optimize memory access patterns
- [ ] Cache-align buffers
- [ ] Benchmark results documentation

---

## 📝 Documentation & Polish (Sprint 5)

### PR #16: API Documentation
**Size:** Small
**Time:** 3-4 hours
**Priority:** Low

- [ ] Complete Doxygen comments for all public APIs
- [ ] Generate HTML documentation
- [ ] Add architecture diagrams
- [ ] Code examples

---

### PR #17: User Documentation
**Size:** Small
**Time:** 2-3 hours
**Priority:** Low

- [ ] Usage guide
- [ ] Command-line options
- [ ] Configuration file format
- [ ] Troubleshooting guide

---

### PR #18: Packaging & Distribution
**Size:** Medium
**Time:** 4-6 hours
**Priority:** Low

- [ ] Create install target
- [ ] Package for Linux (AppImage/deb)
- [ ] CI/CD setup (GitHub Actions)
- [ ] Release process documentation

---

## 📊 Progress Tracking

### Current Status
- ✅ Infrastructure (PRs done: 0/0)
- ⏳ Phase 1: Core (PRs done: 0/4)
- 🔜 Phase 2: Signal Processing (PRs done: 0/5)
- 🔜 Phase 3: Rendering & Audio (PRs done: 0/3)
- 🔜 Phase 4: Integration (PRs done: 0/3)
- 🔜 Documentation & Polish (PRs done: 0/3)

**Total PRs:** 0/18 complete

---

## 🎯 Recommended Order

**Week 1 (Quick Wins):**
1. PR #1 - Fix linking (30 min)
2. PR #2 - RingBuffer core (1 day)
3. PR #3 - RingBuffer threading (0.5 day)
4. PR #4 - Settings (1 day)

**Week 2 (Signal Processing Core):**
5. PR #5 - FFT core (2 days)
6. PR #6 - FFT weighting (1 day)
7. PR #7 - Freq resampler linear (1 day)

**Week 3 (Signal Processing Complete):**
8. PR #8 - Freq resampler scales (1.5 days)
9. PR #9 - Color transform (0.5 day)
10. PR #10 - Spectrogram image (1 day)

**Week 4 (Audio & Rendering):**
11. PR #11 - Audio engine (2 days)
12. PR #12 - SDL2 renderer (2 days)

**Week 5 (Integration):**
13. PR #13 - Clay UI (2-3 days)
14. PR #14 - Main app (2 days)

**Week 6 (Polish):**
15. PR #15 - Optimization (1-2 days)
16. PR #16 - API docs (1 day)
17. PR #17 - User docs (1 day)
18. PR #18 - Packaging (1 day)

---

## 💡 Notes

- **Test-Driven Development:** Write tests first for each PR
- **Code Review:** Each PR should be reviewable in 30-60 minutes
- **CI/CD:** Run tests automatically on each PR
- **Documentation:** Update CLAUDE.md with findings
- **Performance:** Benchmark each component

---

## 🐛 Known Issues to Address

1. **FFTW3 float linking** - PR #1 (immediate)
2. **SDL2 runtime deps** - Document in CLAUDE.md
3. **PortAudio runtime deps** - Document in CLAUDE.md

---

**Last Updated:** 2025-11-06
**Maintainer:** See CLAUDE.md for setup details
