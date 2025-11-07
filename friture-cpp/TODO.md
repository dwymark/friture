# Friture C++ - Next Steps

**Last Updated:** 2025-11-07
**Branch:** `claude/analyze-friture-cpp-plan-011CUsbSFfrQGgkyG65buSed`
**Status:** AudioEngine ready for app integration

---

## ✅ Current State

### Working Features
- Complete signal processing pipeline (FFT → Resample → Color → Display)
- SDL2 spectrogram visualization at 60 FPS with SDL_ttf UI
- WAV file loader (all PCM formats, IEEE Float, mono/stereo)
- **RtAudio AudioEngine** - ready for integration
- Full test suite: **100% passing** (9/9 tests)

### Recent Completions (2025-11-07)
1. Fixed FFT and color transform test thresholds → 100% pass rate
2. Integrated RtAudio 6.0.1 library into build system
3. Created AudioEngine class with device enumeration, thread-safe capture, RMS monitoring

---

## 🎯 Next: FritureApp Integration (2-3 hours)

### Goal
Integrate AudioEngine into FritureApp for live microphone input

### Implementation Tasks

**1. Update FritureApp Class**
- Add `std::unique_ptr<AudioEngine> audio_engine_` member
- Add `enum class InputMode { File, Live }` state
- Add mode switching logic

**2. Keyboard Controls**
- `L` - Toggle live/file mode
- `D` - Cycle through audio input devices
- Update help overlay with new controls

**3. UI Updates**
- Show current mode in status bar ("FILE" / "LIVE")
- Display input device name when in live mode
- Input level meter (RMS bar) in live mode

**4. Data Flow**
```cpp
// File mode (existing):
ring_buffer_->read(position, buffer, size);

// Live mode (new):
audio_engine_->getRingBuffer().read(position, buffer, size);
```

**5. Error Handling**
- Gracefully handle "no audio devices" → stay in file mode
- Device disconnection → auto-switch to file mode
- Show error messages in UI

**6. Testing**
- Test with headless environment (should detect no devices)
- Verify fallback to file mode works correctly
- Test keyboard controls and UI updates

---

## Future Enhancements

### GPU Rendering with SDL3 (10-12 hours)
- GLSL shaders for colormapping and scrolling
- 60+ FPS at 4K resolution
- Smooth subpixel scrolling

### Clay UI Library (8-10 hours)
- Interactive settings panel with sliders
- Mouse-driven device selection
- Real-time setting changes without keyboard

### Performance Optimizations
- SIMD (AVX2) for color transform (<1 μs target)
- Multi-threaded FFT processing
- GPU texture streaming

### Additional Features
- Settings persistence (JSON config file)
- Multiple color themes (Viridis, Plasma, Hot, Cool)
- Export screenshots/recordings
- Cross-platform packaging (AppImage, .dmg, .exe)

---

## Quick Reference

### Build
```bash
mkdir build && cd build
cmake -DCMAKE_BUILD_TYPE=Release ..
make -j4
ctest  # All tests should pass
```

### Run
```bash
# File mode
./src/friture path/to/audio.wav

# With xvfb (headless)
xvfb-run -a -s "-screen 0 1280x720x24" ./src/friture test.wav
```

### Keyboard Controls
| Key | Action |
|-----|--------|
| SPACE | Pause/Resume |
| R | Reset to beginning |
| H | Toggle help overlay |
| 1-5 | Frequency scale (Linear/Log/Mel/ERB/Octave) |
| +/- | Adjust FFT size |
| **L** | **Toggle Live/File mode** (TODO) |
| **D** | **Cycle audio devices** (TODO) |
| Q/ESC | Quit |

### Dependencies
| Library | Version | Status |
|---------|---------|--------|
| SDL2 | 2.30.0 | ✅ |
| SDL2_ttf | 2.22.0 | ✅ |
| FFTW3 | 3.3.10 | ✅ |
| **RtAudio** | **6.0.1** | **✅ (vendored)** |
| ALSA | System | ✅ |
| GoogleTest | 1.14.0 | ✅ |

---

## Success Criteria

Before committing any feature:
- [ ] Compiles without warnings (-Wall -Wextra)
- [ ] All tests pass (ctest)
- [ ] Doxygen comments added
- [ ] Works in headless environment
- [ ] Performance targets met
- [ ] Git commit with clear message

---

**Immediate Next Task:** Integrate AudioEngine into FritureApp (see section above)
