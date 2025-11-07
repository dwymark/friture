# Friture C++ - Next Steps

**Last Updated:** 2025-11-07
**Branch:** `claude/analyze-friture-cpp-plan-011CUsdHXJgdHVjyGk7nUdPL`
**Status:** ✅ **Live Audio Input Complete!**

---

## ✅ Current State

### Working Features
- Complete signal processing pipeline (FFT → Resample → Color → Display)
- SDL2 spectrogram visualization at 60 FPS with SDL_ttf UI
- WAV file loader (all PCM formats, IEEE Float, mono/stereo)
- **✅ Live Microphone Input** - fully integrated with mode switching
- Full test suite: **100% passing** (9/9 tests)

### Recent Completions (2025-11-07)

#### Phase 1: RtAudio Integration
1. Fixed FFT and color transform test thresholds → 100% pass rate
2. Integrated RtAudio 6.0.1 library into build system
3. Created AudioEngine class with device enumeration, thread-safe capture, RMS monitoring

#### Phase 2: FritureApp Integration ✅ **COMPLETE**
1. Added `InputMode` enum (File/Live) to application
2. Implemented mode switching methods (switchToLiveMode, switchToFileMode, cycleInputDevice)
3. Updated processAudioFrame() for dual-mode data source
4. Added keyboard controls: `L` (toggle mode), `D` (cycle devices)
5. Enhanced UI with:
   - Mode indicator in status bar (FILE/LIVE with color coding)
   - Real-time input level meter with gradient (green→yellow→red)
   - Device name display in live mode
   - Updated help overlay with new shortcuts
6. Graceful fallback for headless/no-device environments

---

## 🎯 Next: GPU Rendering with SDL3 (10-12 hours)

### Goal
Transform SDL2 CPU rendering to GPU-accelerated rendering with SDL3

### Why GPU Rendering?
Current implementation uses SDL_RenderCopy (CPU texture upload every frame):
- Limited to ~60 FPS at 1920×1080
- High CPU usage for texture updates
- No subpixel scrolling
- Inefficient for 4K displays

SDL3 GPU rendering benefits:
- 60+ FPS at 4K resolution
- <5% CPU usage (GPU does colormapping)
- Smooth subpixel scrolling
- Persistent-mapped buffers (zero-copy texture streaming)

### Implementation Tasks

**1. SDL3 Migration**
- Update CMakeLists.txt to find SDL3
- Replace SDL2 API calls with SDL3 equivalents
- Port window/renderer creation to SDL3_GPU

**2. GLSL Shaders**
- Vertex shader: full-screen quad with texture coordinates
- Fragment shader:
  - Scroll offset via push constants
  - CMRMAP colormap in 1D lookup texture
  - Real-time colormapping on GPU

**3. Spectrogram Texture Streaming**
- Use GPU_UploadToTexture for column updates
- Ring buffer wrapping handled in shader
- No CPU-side pixel format conversion

**4. Performance Targets**
- 60+ FPS at 4K (3840×2160)
- <10ms frame time
- <5% CPU usage

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
| **L** | **Toggle Live/File mode** ✅ |
| **D** | **Cycle audio devices** ✅ |
| 1-5 | Frequency scale (Linear/Log/Mel/ERB/Octave) |
| +/- | Adjust FFT size |
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
