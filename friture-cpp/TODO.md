# Friture C++ Port - Implementation Roadmap

**Status:** Phase 1 - Core Infrastructure In Progress
**Completed:** FFTW3 linking fix + RingBuffer implementation ✅
**Next:** Settings Management

---

## ✅ Completed (2025-11-06)

### PR #1: FFTW3 Linking Fix
- Fixed float library linking (`-lfftw3f`)
- Validated performance: FFTW3 @ 33μs, Eigen @ 88μs per 4096-point FFT
- All test frequencies correctly detected

### PR #2-3: RingBuffer Implementation
- Lock-free circular buffer template class
- 10 comprehensive unit tests (all passing)
- Thread safety validated with concurrent readers/writers
- Performance: 0.115 μs per 512-sample write (exceeds target!)
- AddressSanitizer + UBSan: No issues detected
- Full Doxygen documentation

**Files Added:**
- `include/friture/ringbuffer.hpp` (161 lines)
- `tests/unit/ringbuffer_test.cpp` (249 lines)
- `tests/unit/CMakeLists.txt` (43 lines with sanitizers)

---

## 🎯 Next Steps

### PR #4: Settings Management - Data Structures
**Size:** Medium (~250 lines)
**Time:** 2-3 hours
**Priority:** High
**Status:** Ready to start

**Files to Create:**
```
include/friture/types.hpp
include/friture/settings.hpp
tests/unit/settings_test.cpp
```

**Implementation Tasks:**

1. **Create `include/friture/types.hpp`:**
   ```cpp
   enum class WindowFunction { Hann, Hamming };
   enum class FrequencyScale { Linear, Logarithmic, Mel, ERB, Octave };
   enum class WeightingType { None, A, B, C };
   ```

2. **Create `include/friture/settings.hpp`:**
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

3. **Write validation tests:**
   - Test default values
   - Test valid ranges
   - Test invalid inputs (min > max, negative values, etc.)
   - Test setters update correctly

**Acceptance Criteria:**
- All validation logic works correctly
- Unit tests have 100% coverage
- Full Doxygen documentation

---

## 📦 Phase 2: Signal Processing Pipeline

### PR #5: FFT Processor - Basic Implementation
**Time:** 4-6 hours
**Dependencies:** RingBuffer ✅, Settings (PR #4)

**Implementation:**
- `include/friture/fft_processor.hpp`
- `src/processing/fft_processor.cpp`
- `tests/unit/fft_processor_test.cpp`

**Key Features:**
- Window function generation (Hann, Hamming)
- FFTW3 integration for real FFT
- Power spectrum calculation
- dB conversion
- Target: <100μs per 4096-point FFT

---

### PR #6: FFT Processor - Psychoacoustic Weighting
**Time:** 2-3 hours
**Dependencies:** PR #5

**Implementation:**
- A-weighting, B-weighting, C-weighting
- Validate against IEC 61672 standards (±0.5 dB)

---

### PR #7: Frequency Resampler - Linear Scale
**Time:** 3 hours
**Dependencies:** PR #5

**Implementation:**
- `include/friture/frequency_resampler.hpp`
- Map FFT bins to screen pixels
- Linear interpolation
- Linear scale first, then Mel/ERB/Octave in PR #8

---

### PR #8: Frequency Resampler - Mel/ERB/Octave Scales
**Time:** 3-4 hours
**Dependencies:** PR #7

**Implementation:**
- Mel scale transformation
- ERB scale (psychoacoustic)
- Logarithmic scale
- Octave scale

---

### PR #9: Color Transform - CMRMAP Implementation
**Time:** 2 hours
**Dependencies:** None (can run in parallel)

**Implementation:**
- `include/friture/color_transform.hpp`
- CMRMAP colormap lookup table (256 entries)
- Fast batch column transformation
- Target: <1μs per pixel

---

## 📊 Progress Summary

| Phase | Component | Status | Lines | Tests |
|-------|-----------|--------|-------|-------|
| **Infrastructure** | CMake + Deps | ✅ | 77 | - |
| **Phase 1** | RingBuffer | ✅ | 410 | 10 |
| **Phase 1** | Settings | 🔜 Next | ~250 | TBD |
| **Phase 2** | FFT Processor | 📋 Planned | ~400 | TBD |
| **Phase 2** | Freq Resampler | 📋 Planned | ~300 | TBD |
| **Phase 2** | Color Transform | 📋 Planned | ~200 | TBD |

**Total Implemented:** 487 lines + tests
**Next Milestone:** Complete Phase 1 (Settings)

---

## 🏗️ Future Phases

### Phase 3: Audio Engine & Rendering (Week 4-5)
- PortAudio integration
- SDL2/SDL3 renderer with GLSL shaders
- Spectrogram image ring buffer

### Phase 4: UI & Integration (Week 5-6)
- Clay UI integration
- Main application loop
- Performance optimization

### Phase 5: Documentation & Polish (Week 6)
- API documentation
- User guide
- Packaging

---

## 🔧 Development Environment

**Branch:** `claude/analyze-friture-port-plan-011CUs9JDdoz687n4urZ88nM`

**Build Commands:**
```bash
cd /home/user/friture/friture-cpp
mkdir -p build && cd build
cmake -DCMAKE_BUILD_TYPE=Debug ..
make ringbuffer_test
./tests/unit/ringbuffer_test
```

**Test with Sanitizers:**
```bash
# Sanitizers enabled automatically in Debug mode
cmake -DCMAKE_BUILD_TYPE=Debug ..
make ringbuffer_test
ASAN_OPTIONS=detect_leaks=1 ./tests/unit/ringbuffer_test
```

---

## 📚 References

- **FRITURE_CPP_PORT_PLAN.md** - Complete specification (1661 lines)
- **CLAUDE.md** - Environment setup guide (649 lines)
- **Original Friture:** `/home/user/friture/friture/` (Python implementation)

---

**Last Updated:** 2025-11-06
**Performance Targets:**
- FFT: <100μs (achieved 33μs with FFTW3 ✅)
- RingBuffer write: <1μs (achieved 0.115μs ✅)
- Memory: <50MB for typical operation
- Frame rate: 60+ FPS
