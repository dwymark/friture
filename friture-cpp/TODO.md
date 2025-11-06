# Friture C++ Port - Next Steps

**Status:** ✅ Phase 3 COMPLETE - Main SDL2 Application Working! 🎉

**Current State:**
- Fully functional real-time spectrogram viewer
- All signal processing components operational
- SDL2 rendering at 60 FPS
- Headless testing support
- 138 tests passing (135 functional, 3 performance tolerances)

**Branch:** `claude/analyze-friture-port-plan-011CUsTPs64KeHkPqPatACBq`

---

## Recommended Implementation Order

### 1️⃣ PRIORITY: WAV File Loading (3-4 hours)

**Goal:** Load real audio files instead of only synthetic signals

**Implementation Details:**
- Create `AudioFileLoader` class with WAV file support
- Parse RIFF/WAV header (chunk-based reading)
- Support multiple formats:
  - PCM 16-bit (most common)
  - PCM 24-bit and 32-bit
  - IEEE Float 32-bit
  - Mono and stereo (convert stereo to mono by averaging channels)
- Handle various sample rates (resample if needed, or error for now)
- Validate file format before loading

**Files to Create:**
- `include/friture/audio/audio_file_loader.hpp`
- `src/audio/audio_file_loader.cpp`
- `tests/unit/audio_file_loader_test.cpp`

**Edge Cases & Considerations:**
- ✅ File doesn't exist → Clear error message
- ✅ Invalid WAV format → Reject with detailed error
- ✅ Sample rate mismatch → Either resample or document limitation
- ✅ File too large → Stream in chunks or set reasonable limit (e.g., 60 seconds)
- ✅ Compressed formats (ADPCM, μ-law) → Document as unsupported for v1.0
- ✅ Corrupted files → Robust error handling
- ✅ Non-standard chunk ordering → Handle out-of-order chunks
- ✅ Metadata chunks (INFO, LIST) → Skip gracefully
- ⚠️ Memory management → Don't load entire file if >100MB
- ⚠️ Endianness → WAV is little-endian, ensure proper byte order

**Testing Strategy:**
- Unit tests with synthetic WAV files (generated in test)
- Test with real-world WAV files from examples
- Boundary tests (empty file, 1 sample, very large)
- Format validation tests
- Sample rate conversion tests (if implemented)

**Integration:**
- Update `Application::loadAudioFromFile()` to use new loader
- Add progress indicator for large files
- Display file metadata (sample rate, duration, channels)

---

### 2️⃣ SDL_ttf Integration for Text Rendering (2-3 hours)

**Goal:** Render actual text instead of colored rectangles

**Implementation Details:**
- Integrate SDL2_ttf library
- Load embedded or system font (Liberation Sans, DejaVu Sans)
- Create `TextRenderer` helper class
- Render:
  - FPS counter with actual numbers
  - Settings display (FFT size, frequency scale, etc.)
  - Help overlay with keyboard shortcuts
  - Status messages (paused, loading, etc.)
  - Frequency axis labels (Hz/kHz markers)

**Files to Create:**
- `include/friture/ui/text_renderer.hpp`
- `src/ui/text_renderer.cpp`
- Update `Application::drawUI()` to use text rendering

**Edge Cases & Considerations:**
- ✅ Font file not found → Fallback to system font or embedded font
- ✅ Font size scaling → Handle window resize
- ✅ UTF-8 support → Ensure proper text encoding
- ✅ Color customization → White/black text based on background
- ⚠️ Performance → Cache rendered text surfaces
- ⚠️ Memory leaks → Proper TTF_Font cleanup
- ⚠️ Headless testing → May need dummy font or skip in headless mode

**Build System:**
- Add `find_package(SDL2_ttf)` to CMakeLists.txt
- Update install_deps.sh with `libsdl2-ttf-dev`
- Make text rendering optional if SDL_ttf not available

**UI Improvements:**
- Help overlay (H key):
  ```
  Keyboard Controls:
  SPACE - Pause/Resume
  R - Reset
  1-5 - Frequency Scale
  +/- - FFT Size
  Q - Quit
  ```
- Status bar with actual text:
  - "FPS: 60.2 | FFT: 4096 | Scale: Mel | Paused"

---

### 3️⃣ Screenshot Export & BMP Save (1-2 hours)

**Goal:** Save current spectrogram view to image file

**Implementation Details:**
- Add keyboard shortcut (S key) to save screenshot
- Export current spectrogram view as BMP file
- Optionally support PNG (via stb_image_write or SDL2_image)
- Include timestamp in filename
- Add UI overlay annotations (frequency labels, settings)

**Files to Modify:**
- `Application::handleKeyboard()` - Add 'S' key handler
- `Application::saveScreenshot()` - New method
- Optionally create `ImageExporter` utility class

**Edge Cases & Considerations:**
- ✅ Output directory doesn't exist → Create automatically
- ✅ File name collision → Append timestamp or counter
- ✅ Disk full → Handle write errors gracefully
- ✅ Permissions → Check write access before saving
- ⚠️ PNG support → Optional, BMP is sufficient for v1.0
- ⚠️ Include metadata → Embed settings as comment in file header

**Features:**
- Auto-filename: `spectrogram_2025-11-06_23-15-42.bmp`
- Save full window or just spectrogram area (user choice)
- Display confirmation message: "Saved: output/spectrogram_xxx.bmp"
- Option to save with/without UI overlay

---

### 4️⃣ Audio Engine - PortAudio Integration (6-8 hours)

**Goal:** Real-time microphone/line-in capture for live spectrogram

**Implementation Details:**
- Create `AudioEngine` class wrapping PortAudio
- Device enumeration and selection
- Audio input stream with callback
- Thread-safe ring buffer integration
- Latency management (<10ms target)
- Handle buffer underruns/overruns

**Files to Create:**
- `include/friture/audio/audio_engine.hpp`
- `src/audio/audio_engine.cpp`
- `include/friture/audio/audio_device_info.hpp` (device metadata)
- `tests/unit/audio_engine_test.cpp`
- `tests/integration/audio_pipeline_test.cpp`

**Edge Cases & Considerations:**
- ✅ No input devices → Graceful fallback to file/synthetic mode
- ✅ Default device invalid → Try first available device
- ✅ Sample rate mismatch → Resample or configure device
- ✅ Buffer overflow → Drop frames and log warning
- ✅ Buffer underflow → Insert silence, don't crash
- ✅ Device disconnected during capture → Detect and stop gracefully
- ✅ Multiple channels → Average to mono or select channel
- ⚠️ Platform differences:
  - Linux: ALSA, PulseAudio, JACK
  - Windows: WASAPI, DirectSound
  - macOS: CoreAudio
- ⚠️ Latency tuning → Balance buffer size vs responsiveness
- ⚠️ Audio callback thread → No allocations, no locks
- ⚠️ Headless testing → Mock audio input or skip tests

**API Design:**
```cpp
class AudioEngine {
public:
    AudioEngine(size_t sample_rate = 48000, size_t buffer_size = 512);

    // Device management
    std::vector<AudioDeviceInfo> getInputDevices();
    void setInputDevice(int device_id);

    // Stream control
    void start();
    void stop();
    bool isRunning() const;

    // Data access
    RingBuffer<float>& getRingBuffer();

    // Monitoring
    float getInputLevel() const;  // RMS level for meter
    size_t getDroppedFrames() const;

private:
    static int audioCallback(const void* input, void* output,
                            unsigned long frames, ...);
};
```

**Testing Strategy:**
- Unit tests with null device
- Simulated input (sine wave generator device if available)
- File-based playback as input
- Thread safety tests
- Latency benchmarks
- Stress tests (long duration, rapid start/stop)

**Integration with Application:**
- Add mode selection: File vs Live Input
- Device selection menu (later with Clay UI)
- Input level meter in status bar
- Auto-start on application launch (optional)

---

### 5️⃣ Advanced Rendering - SDL3/GLSL Shaders (10-12 hours)

**Goal:** GPU-accelerated rendering with smooth scrolling

**Implementation Details:**
- Migrate from SDL2 to SDL3 GPU API
- Implement GLSL vertex and fragment shaders
- Efficient texture streaming (avoid CPU→GPU copy overhead)
- Push constants for scroll offset
- Optional: Frequency axis labels rendered as shader overlays

**Files to Create:**
- `include/friture/rendering/gpu_renderer.hpp`
- `src/rendering/gpu_renderer.cpp`
- `shaders/spectrogram.vert.glsl`
- `shaders/spectrogram.frag.glsl`
- Shader compilation script (GLSL → SPIR-V)

**Shader Features:**
- Vertex shader: Full-screen quad with texture coordinates
- Fragment shader:
  - Texture sampling with scroll offset
  - Optional color adjustment (brightness, contrast)
  - Optional grid overlay for frequency markers
  - Anti-aliasing for smooth scaling

**Edge Cases & Considerations:**
- ✅ SDL3 not available → Fallback to SDL2 renderer
- ✅ GPU not available → Software fallback
- ✅ Shader compilation failure → Detailed error logging
- ✅ Texture size limits → Handle large spectrograms (>8K)
- ⚠️ Shader language versions (GLSL 450 vs older)
- ⚠️ Different GPU vendors (NVIDIA, AMD, Intel)
- ⚠️ Headless rendering → Use offscreen framebuffers
- ⚠️ VSync handling → Tearing prevention
- ⚠️ HDR displays → Color space considerations

**Performance Targets:**
- 60+ FPS at 4K resolution
- <1ms texture upload time
- <5ms total frame time

**Shader Example (Fragment):**
```glsl
#version 450

layout(location = 0) in vec2 frag_texcoord;
layout(location = 0) out vec4 out_color;

layout(binding = 0) uniform sampler2D spectrogram_texture;

layout(push_constant) uniform PushConstants {
    float scroll_offset;
    float brightness;
    float contrast;
} constants;

void main() {
    vec2 uv = frag_texcoord;
    uv.x = mod(uv.x + constants.scroll_offset, 1.0);

    vec4 color = texture(spectrogram_texture, uv);

    // Optional adjustments
    color.rgb = (color.rgb - 0.5) * constants.contrast + 0.5 + constants.brightness;

    out_color = color;
}
```

---

### 6️⃣ UI Layer - Clay Integration (8-10 hours)

**Goal:** Interactive settings panel with sliders and controls

**Implementation Details:**
- Integrate Clay immediate-mode UI library
- Build settings sidebar/panel
- Interactive controls:
  - Sliders for FFT size, min/max frequency, dB range
  - Dropdown for frequency scale
  - Device selector (when AudioEngine available)
  - Color theme selector
  - Toggle buttons for pause, reset, etc.
- Mouse interaction handling
- Resize and layout management

**Files to Create:**
- `include/friture/ui/ui_layer.hpp`
- `src/ui/ui_layer.cpp`
- `third_party/clay/` (Clay library as submodule or vendored)
- Update `Application` to integrate UILayer

**UI Layout Design:**
```
┌────────────────────────────────────────────┐
│  Friture C++ Spectrogram      [_][□][X]   │
├────────────────────────────────────────────┤
│                                   Settings │
│                                   ┌───────┐│
│                                   │Device │││
│   [Spectrogram Display Area]     │▼ Mic  │││
│                                   ├───────┤│
│                                   │FFT    │││
│                                   │4096 ▸ │││
│                                   ├───────┤│
│                                   │Scale  │││
│                                   │◉ Mel  │││
│                                   │○ Lin  │││
│                                   └───────┘│
├────────────────────────────────────────────┤
│ FPS: 60 | FFT: 4096 | Mel     [Paused]    │
└────────────────────────────────────────────┘
```

**Edge Cases & Considerations:**
- ✅ Window too small → Scroll or collapse UI
- ✅ Touch input → Handle both mouse and touch
- ✅ Focus management → Keyboard navigation
- ✅ Settings validation → Prevent invalid values
- ⚠️ Clay rendering → Integrate with SDL2/SDL3
- ⚠️ Layout responsiveness → Different screen sizes
- ⚠️ Theme support → Dark/light mode
- ⚠️ Accessibility → Keyboard-only navigation

**Clay Integration:**
- Clay generates render commands (rectangles, text, etc.)
- Convert Clay commands to SDL draw calls
- Handle input events (mouse, keyboard) and feed to Clay
- State management for interactive widgets

**Settings Persistence:**
- Save settings to config file on exit
- Load settings on startup
- JSON or INI format for human-readability

---

## Additional Future Enhancements

### 7️⃣ Settings Persistence (2 hours)
- Save/load configuration from JSON file
- Remember window size, position
- Persist FFT size, frequency range, color theme
- Recent files list

### 8️⃣ Multiple Color Themes (2 hours)
- Add more colormaps (Viridis, Plasma, Hot, Cool)
- Theme editor/customization
- High-contrast mode for accessibility

### 9️⃣ Performance Profiling & Optimization (4 hours)
- Profile with perf/valgrind/VTune
- SIMD optimizations (AVX2/AVX-512)
- Multi-threading exploration
- Cache optimization

### 🔟 Cross-Platform Testing & Packaging (6 hours)
- Test on Windows, macOS, Linux
- Create installers (MSI, DMG, AppImage)
- CI/CD pipeline (GitHub Actions)
- Release builds with optimizations

---

## Quick Reference

### Current Build Commands
```bash
cd /home/user/friture/friture-cpp/build
cmake -DCMAKE_BUILD_TYPE=Debug ..
make friture -j4

# Run
cd src
xvfb-run -a -s "-screen 0 1280x720x24" ./friture
```

### Testing
```bash
# All unit tests
make -j4
ctest --output-on-failure

# Specific test
./tests/unit/fft_processor_test
```

### Dependencies Status
| Library | Status | Version | Purpose |
|---------|--------|---------|---------|
| SDL2 | ✅ Installed | 2.30.0 | Rendering |
| SDL2_ttf | ⚠️ **TODO** | - | Text rendering |
| PortAudio | ✅ Installed | 19.6.0 | Audio input (not yet integrated) |
| FFTW3 | ✅ Installed | 3.3.10 | FFT processing |
| GoogleTest | ✅ Installed | 1.14.0 | Testing |
| Eigen3 | ✅ Installed | 3.4.0 | Math utilities |
| Clay UI | ⚠️ **TODO** | - | UI layout |

---

## Success Criteria

Each feature should meet these criteria before moving to the next:

- ✅ **Compiles** without warnings (-Wall -Wextra -Wpedantic)
- ✅ **Tests pass** with AddressSanitizer and UBSanitizer enabled
- ✅ **Documented** with Doxygen-style comments
- ✅ **Headless compatible** (works in CI environment)
- ✅ **Performance target** met (if applicable)
- ✅ **Code reviewed** for correctness and style
- ✅ **Git committed** with descriptive message

---

**Last Updated:** 2025-11-06
**Next Immediate Task:** WAV File Loading (Item #1)
**Current Milestone:** Phase 4 - Enhanced I/O & Real-time Audio
