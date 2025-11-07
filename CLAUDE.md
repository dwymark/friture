# Friture C++ Port - Development Environment Setup

**Date:** 2025-11-06
**Purpose:** Instructions for setting up the cloud build environment for Friture C++ port
**Tested on:** Ubuntu 24.04 (Noble)

---

## Overview

This document provides comprehensive instructions for setting up a headless cloud development environment for the Friture C++ port project. The setup has been tested and validated in a cloud container environment.

## Prerequisites

- Ubuntu 24.04 (Noble) or compatible
- Root or sudo access
- Internet connection for package downloads

---

## 1. Install Build Tools & Dependencies

### 1.1 Update Package Lists

```bash
apt-get update
```

**Note:** You may see GPG errors in cloud environments. These are usually harmless for development purposes.

### 1.2 Install Core Build Tools

```bash
apt-get install -y \
    build-essential \
    cmake \
    pkg-config \
    git
```

**Validation:**
```bash
gcc --version  # Should show gcc 13.3.0
cmake --version  # Should show cmake 3.28.3
```

### 1.3 Install Project Dependencies

```bash
apt-get install -y \
    libsdl2-dev \
    portaudio19-dev \
    libeigen3-dev \
    libgtest-dev \
    libfftw3-dev \
    xpra \
    xvfb
```

### 1.4 Install Additional Libraries (May Be Needed)

Some dependencies may require additional packages. Install as needed:

```bash
apt-get install -y \
    libasound2-dev \
    libpulse-dev \
    libdecor-0-dev \
    libsamplerate0-dev \
    libjack-jackd2-dev
```

### 1.5 Verify Installation

Use the following script to check installed dependencies:

```bash
cat > /tmp/check_deps.sh << 'EOF'
#!/bin/bash
echo "=== Checking Dependencies ==="
echo ""

for header in SDL2/SDL.h portaudio.h eigen3/Eigen/Core gtest/gtest.h fftw3.h; do
    if [ -f "/usr/include/$header" ] || [ -f "/usr/local/include/$header" ]; then
        echo "✓ $header"
    else
        echo "✗ $header (missing)"
    fi
done

for bin in gcc g++ cmake xpra; do
    if which "$bin" >/dev/null 2>&1; then
        echo "✓ $bin"
    else
        echo "✗ $bin (missing)"
    fi
done
EOF

chmod +x /tmp/check_deps.sh && /tmp/check_deps.sh
```

Expected output should show all items with ✓ marks.

---

## 2. Project Structure

The Friture C++ port follows this directory structure:

```
friture-cpp/
├── CMakeLists.txt           # Main build configuration
├── src/                     # Source files (future)
│   ├── audio/
│   ├── processing/
│   ├── rendering/
│   └── ui/
├── include/                 # Public headers (future)
│   └── friture/
├── examples/                # Dependency validation programs
│   ├── test_portaudio.cpp
│   ├── test_sdl2.cpp
│   ├── test_fft.cpp
│   └── test_eigen.cpp
├── tests/                   # Unit tests
│   └── placeholder_test.cpp
├── shaders/                 # GLSL shaders (future)
├── third_party/             # External dependencies (e.g., Clay)
└── build/                   # Build directory (gitignored)
```

---

## 3. Building the Project

### 3.1 Initial Build

```bash
cd /home/user/friture/friture-cpp
mkdir -p build
cd build
cmake ..
make -j$(nproc)
```

### 3.2 Build Configuration Options

CMake options available:

- `BUILD_TESTS`: Build unit tests (default: ON)
- `BUILD_EXAMPLES`: Build example programs (default: ON)
- `USE_FFTW`: Use FFTW3 for FFT instead of Eigen (default: ON)

Example with custom options:

```bash
cmake .. -DBUILD_TESTS=OFF -DUSE_FFTW=OFF
make
```

### 3.3 Build Output

Successful build will create:

- `examples/test_eigen` - Eigen matrix operations test ✓ WORKING
- `examples/test_fft` - FFT performance comparison (needs fftw3f linking fix)
- `examples/test_sdl2` - SDL2 rendering test (needs additional deps)
- `examples/test_portaudio` - Audio capture test (needs additional deps)
- `tests/placeholder_test` - GoogleTest placeholder

---

## 4. Running Tests

### 4.1 Run Eigen Test (Working)

```bash
cd /home/user/friture/friture-cpp/build
./examples/test_eigen
```

**Expected Output:**
```
=== Eigen Test ===
Eigen version: 3.4.0

=== Basic Matrix Operations ===
Random 3x3 matrix:
...

✓ Eigen test PASSED
```

### 4.2 Run Unit Tests

```bash
./tests/placeholder_test
```

### 4.3 Run SDL2 Test (Headless with Xpra)

For graphical tests in a headless environment, use xpra:

```bash
# Start xpra display server
xpra start :100 --start=./examples/test_sdl2

# After the program runs, capture a screenshot
xpra screenshot :100 --file=sdl2_frame.png

# Stop the server
xpra stop :100
```

Alternatively, use xvfb for headless OpenGL:

```bash
xvfb-run -a ./examples/test_sdl2
```

---

## 5. Dependency Status & Known Issues

### 5.1 Working Components

| Component | Status | Notes |
|-----------|--------|-------|
| **CMake** | ✓ Working | Version 3.28.3 |
| **GCC** | ✓ Working | Version 13.3.0, C++20 support |
| **Eigen3** | ✓ Working | Version 3.4.0, all tests pass |
| **GoogleTest** | ✓ Working | Version 1.14.0 |
| **FFTW3** | ⚠ Partial | Headers found, needs `libfftw3f` linking |
| **Xpra** | ✓ Working | For headless GUI testing |

### 5.2 Components Needing Fixes

| Component | Issue | Solution |
|-----------|-------|----------|
| **SDL2** | Missing runtime deps | Needs: `libdecor`, `libpulse`, `libsamplerate` |
| **PortAudio** | Missing JACK/ALSA | Needs: `libjack-jackd2-0`, `libasound2` runtime libs |
| **FFTW3 Float** | Linking error | Add `-lfftw3f` to link flags in CMake |

### 5.3 Fixing SDL2 Dependencies

```bash
apt-get install -y \
    libdecor-0-0 \
    libpulse0 \
    libsamplerate0 \
    libwayland-client0
```

### 5.4 Fixing PortAudio Dependencies

```bash
apt-get install -y \
    libjack-jackd2-0 \
    libasound2 \
    libportaudio2
```

### 5.5 Fixing FFTW3 Float Linking

Edit `examples/CMakeLists.txt`:

```cmake
# Change from:
target_link_libraries(test_fft ${FFTW3_LIBRARY} m)

# To:
target_link_libraries(test_fft fftw3f fftw3 m Eigen3::Eigen)
```

Then rebuild:
```bash
cd build
make test_fft
./examples/test_fft
```

---

## 6. Development Workflow

### 6.1 Typical Development Cycle

```bash
# 1. Edit source files
vim ../src/some_file.cpp

# 2. Rebuild (incremental)
make

# 3. Run tests
ctest --output-on-failure

# 4. Run specific example
./examples/test_eigen
```

### 6.2 Clean Rebuild

```bash
cd /home/user/friture/friture-cpp
rm -rf build
mkdir build && cd build
cmake ..
make -j$(nproc)
```

### 6.3 Debugging Build Issues

Enable verbose output:

```bash
make VERBOSE=1
```

Check specific compilation commands:

```bash
make test_eigen VERBOSE=1 2>&1 | grep "Building CXX"
```

---

## 7. Testing Strategy

### 7.1 Headless Testing Approach

For components requiring display/audio:

1. **SDL2/OpenGL**: Use `xvfb-run` or `xpra` for headless rendering
2. **PortAudio**: Use "null" audio backend or test with file I/O
3. **Rendering validation**: Save frames to BMP files for inspection

### 7.2 Example: Headless SDL2 with Screenshot

```bash
# Start xpra server
xpra start :100

# Run SDL2 test on virtual display
DISPLAY=:100 ./examples/test_sdl2 &
SDL_PID=$!

# Wait for rendering
sleep 3

# Capture screenshot
xpra screenshot :100 --file=output.png

# Clean up
kill $SDL_PID
xpra stop :100

# View screenshot (download from cloud or use base64)
file output.png
```

### 7.3 Automated Test Execution

```bash
#!/bin/bash
# run_all_tests.sh

set -e  # Exit on error

echo "=== Running All Tests ==="

# Unit tests
echo "Running GoogleTest suite..."
./tests/placeholder_test

# Eigen test
echo "Running Eigen test..."
./examples/test_eigen

# Add more tests as they become available
echo "All tests passed ✓"
```

---

## 8. Performance Benchmarking

### 8.1 FFT Performance Test

When `test_fft` is working:

```bash
./examples/test_fft
```

Expected performance (4096-point FFT):
- FFTW3: ~50-100 μs (optimized)
- Eigen: ~100-200 μs (portable)

### 8.2 Matrix Operations Benchmark

```bash
./examples/test_eigen
```

Expected: 1000×1000 matrix multiplication ~150-200ms

---

## 9. Common Issues & Solutions

### Issue 1: CMake Can't Find Eigen3

**Symptom:**
```
CMake Error: Could not find a package configuration file provided by "Eigen3"
```

**Solution:**
```bash
apt-get install -y libeigen3-dev
# Or set manually:
cmake .. -DEIGEN3_INCLUDE_DIR=/usr/include/eigen3
```

### Issue 2: Undefined References to `fftwf_*`

**Symptom:**
```
undefined reference to `fftwf_alloc_real'
```

**Solution:**
Link against the float version of FFTW:
```cmake
target_link_libraries(target fftw3f fftw3 m)
```

### Issue 3: SDL2 Window Won't Display

**Symptom:**
```
SDL_CreateWindow: No available video device
```

**Solution:**
Use xpra or xvfb:
```bash
xvfb-run -a -s "-screen 0 1920x1080x24" ./examples/test_sdl2
```

### Issue 4: Port Audio: "No default input device"

**Symptom:**
```
Failed to open stream: Invalid device
```

**Solution:**
This is expected in headless environments. Modify test to:
1. Use file-based audio input
2. Generate synthetic audio
3. Use "null" audio backend

---

## 10. Next Steps for Development

### 10.1 Phase 1: Core Infrastructure

- [ ] Implement `RingBuffer<T>` class with tests
- [ ] Create `SpectrogramSettings` struct
- [ ] Set up continuous integration

### 10.2 Phase 2: Signal Processing

- [ ] Implement `FFTProcessor` with FFTW3
- [ ] Implement `FrequencyResampler` (Mel, ERB, Log scales)
- [ ] Implement `ColorTransform` (CMRMAP colormap)

### 10.3 Phase 3: Audio & Rendering

- [ ] Integrate PortAudio for real-time capture
- [ ] Implement SDL3 GPU renderer (or SDL2 as fallback)
- [ ] Create spectrogram image ring buffer

### 10.4 Phase 4: UI & Integration

- [ ] Integrate Clay UI library
- [ ] Connect all components in main application
- [ ] Performance optimization

---

## 11. Environment Variables

Useful environment variables for development:

```bash
# For headless display
export DISPLAY=:99

# For debugging
export VERBOSE=1

# For SDL2
export SDL_VIDEODRIVER=x11  # or 'dummy' for headless

# For PortAudio
export PA_ALSA_PLUGHW=1
```

---

## 12. Docker/Container Setup (Optional)

If using Docker, save dependencies with:

```dockerfile
FROM ubuntu:24.04

RUN apt-get update && apt-get install -y \
    build-essential cmake pkg-config git \
    libsdl2-dev portaudio19-dev libeigen3-dev \
    libgtest-dev libfftw3-dev xpra xvfb \
    libasound2-dev libpulse-dev

WORKDIR /workspace
```

---

## 13. Troubleshooting Checklist

Before asking for help, verify:

- [ ] All dependencies installed (`/tmp/check_deps.sh`)
- [ ] CMake configured successfully
- [ ] Build directory is clean (`rm -rf build && mkdir build`)
- [ ] Using correct include paths (e.g., `eigen3/Eigen/Dense`)
- [ ] Linking correct libraries (`-lfftw3f` for float FFT)
- [ ] Environment variables set for headless testing

---

## 14. Additional Resources

- **SDL2 Documentation**: https://wiki.libsdl.org/SDL2/
- **Eigen Documentation**: https://eigen.tuxfamily.org/dox/
- **FFTW3 Manual**: http://www.fftw.org/fftw3_doc/
- **PortAudio Tutorial**: http://www.portaudio.com/docs/
- **CMake Documentation**: https://cmake.org/documentation/
- **GoogleTest Primer**: https://google.github.io/googletest/

---

## 15. Quick Reference Commands

```bash
# Full clean rebuild
rm -rf build && mkdir build && cd build && cmake .. && make -j$(nproc)

# Run single test
./examples/test_eigen

# Run all tests
ctest --output-on-failure

# Headless SDL2 test
xvfb-run -a ./examples/test_sdl2

# Check dependencies
ldconfig -p | grep -E '(eigen|fftw|sdl|portaudio)'

# Find header files
find /usr/include -name "fftw3.h" -o -name "portaudio.h"
```

---

## Appendix A: Validated Configuration

**System:** Ubuntu 24.04 (cloud container)
**GCC:** 13.3.0
**CMake:** 3.28.3
**C++ Standard:** C++20

**Working Tests:**
- ✓ Eigen3 matrix operations
- ✓ GoogleTest framework
- ✓ CMake build system
- ✓ Xpra headless GUI

**Pending Fixes:**
- FFTW3 float linking
- SDL2 runtime dependencies
- PortAudio runtime dependencies

---

## Appendix B: Complete Dependency Install Script

```bash
#!/bin/bash
# install_deps.sh - Complete dependency installation

set -e

echo "=== Installing Friture C++ Dependencies ==="

apt-get update

# Core build tools
apt-get install -y \
    build-essential \
    cmake \
    pkg-config \
    git

# Development libraries
apt-get install -y \
    libsdl2-dev \
    portaudio19-dev \
    libeigen3-dev \
    libgtest-dev \
    libfftw3-dev

# Headless testing
apt-get install -y \
    xpra \
    xvfb

# Additional runtime libraries
apt-get install -y \
    libasound2-dev \
    libpulse-dev \
    libdecor-0-dev \
    libsamplerate0-dev \
    libjack-jackd2-dev

echo "✓ All dependencies installed successfully"
```

---

**End of Document**

For questions or issues, refer to the main project documentation at `/home/user/friture/FRITURE_CPP_PORT_PLAN.md`.
