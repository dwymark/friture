# Friture C++ Port - Next Steps

**Last Updated:** 2025-11-07
**Current Branch:** `claude/analyze-friture-cpp-plan-011CUsYHBRGU3nTfMGd4Lbo6`
**Status:** Phase 4 - Real-Time Audio Input

---

## Current State ✅

**Completed Features:**
- ✅ Complete signal processing pipeline (FFT, frequency resampling, color transform)
- ✅ SDL2 spectrogram visualization at 60 FPS
- ✅ WAV file loader (PCM 16/24/32-bit, IEEE Float 32-bit, mono/stereo)
- ✅ SDL_ttf text rendering with full UI:
  - Real-time FPS counter with color coding
  - Settings display (FFT size, frequency scale, range)
  - Frequency axis labels (Hz/kHz markers)
  - Keyboard help overlay (press H)
  - Status indicators (paused state)
- ✅ Comprehensive test suite (9 tests, 78% passing)
- ✅ All core components (ring buffer, settings, FFT, resampling, color transforms)

---

## 🎯 Next Priority: Real-Time Audio Input

### RtAudio Integration (5-6 hours)

**Goal:** Transform from WAV file viewer to live audio analyzer

**Why RtAudio over PortAudio:**
- Native C++ API (better fit for C++20 project)
- Simpler, more intuitive API
- Single-file integration (RtAudio.cpp + RtAudio.h)
- Automatic format/channel conversions
- Same cross-platform support and low-latency performance

**Implementation Plan:**

1. **Download RtAudio** (~10 min)
   - Get from https://github.com/thestk/rtaudio
   - Place in `third_party/rtaudio/`

2. **Create AudioEngine Class** (2 hours)
   - `include/friture/audio/audio_engine.hpp`
   - `src/audio/audio_engine.cpp`
   - `include/friture/audio/audio_device_info.hpp`

3. **API Design:**
   ```cpp
   class AudioEngine {
   public:
       AudioEngine(size_t sample_rate = 48000, size_t buffer_size = 512);
       std::vector<AudioDeviceInfo> getInputDevices();
       void setInputDevice(unsigned int device_id);
       void start();
       void stop();
       RingBuffer<float>& getRingBuffer();
       float getInputLevel() const;  // RMS level for meter
   };
   ```

4. **Testing** (1.5 hours)
   - `tests/unit/audio_engine_test.cpp`
   - `tests/integration/audio_pipeline_test.cpp`
   - Device enumeration, stream start/stop, thread safety

5. **Application Integration** (1.5 hours)
   - Mode selection: File vs Live Input
   - Basic device selection (keyboard controls)
   - Input level meter in status bar
   - Keyboard: `L` - toggle live, `D` - cycle devices

6. **Build System** (~15 min)
   - Update CMakeLists.txt
   - Update install_deps.sh with ALSA packages

**Edge Cases:**
- No input devices → Fallback to file mode
- Buffer overflow/underflow handling
- Device disconnection during capture
- Platform differences (ALSA, PulseAudio, JACK)

---

## Future Priorities

### SDL3 GPU Rendering (10-12 hours)
- GLSL shaders for GPU-accelerated rendering
- Smooth scrolling, 60+ FPS at 4K
- Efficient texture streaming

### Clay UI Integration (8-10 hours)
- Interactive settings panel
- Sliders, dropdowns, buttons
- Device selector with live preview
- Mouse interaction

### Additional Enhancements
- Settings persistence (JSON config)
- Multiple color themes (Viridis, Plasma, Hot, Cool)
- Performance profiling & SIMD optimization
- Cross-platform testing & packaging

---

## Quick Reference

### Build & Run
```bash
cd /home/user/friture/friture-cpp/build
cmake -DCMAKE_BUILD_TYPE=Debug ..
make friture -j4

# Run with WAV file
cd src
xvfb-run -a -s "-screen 0 1280x720x24" ./friture [audio_file.wav]

# Run all tests
cd ..
ctest --output-on-failure
```

### Keyboard Controls
- **SPACE** - Pause/Resume
- **R** - Reset to beginning
- **H** - Toggle help overlay
- **1-5** - Frequency scale (Linear/Log/Mel/ERB/Octave)
- **+/-** - FFT size
- **Q/ESC** - Quit

### Dependencies Status
| Library | Status | Purpose |
|---------|--------|---------|
| SDL2 | ✅ 2.30.0 | Rendering |
| SDL2_ttf | ✅ 2.22.0 | Text rendering |
| FFTW3 | ✅ 3.3.10 | FFT processing |
| GoogleTest | ✅ 1.14.0 | Testing |
| Eigen3 | ✅ 3.4.0 | Math utilities |
| **RtAudio** | ⚠️ TODO | Real-time audio input |

---

## Success Criteria

Each feature must meet:
- ✅ Compiles without warnings
- ✅ Tests pass with sanitizers enabled
- ✅ Documented with Doxygen comments
- ✅ Headless compatible
- ✅ Performance target met
- ✅ Git committed with descriptive message

---

**Next Immediate Task:** RtAudio Integration (see above)
