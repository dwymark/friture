# Friture C++ Port

A standalone C++ port of [Friture](https://github.com/tlecomte/friture)'s real-time 2D spectrogram visualization.

## Quick Start

```bash
# Install dependencies (Ubuntu 24.04)
./install_deps.sh

# Build
mkdir build && cd build
cmake ..
make -j$(nproc)

# Run tests
./examples/test_eigen
```

## Documentation

- **[CLAUDE.md](CLAUDE.md)** - Complete environment setup guide (649 lines)
- **[FRITURE_CPP_PORT_PLAN.md](../FRITURE_CPP_PORT_PLAN.md)** - Full specification and implementation plan

## Technology Stack

- **Graphics**: SDL2/SDL3 with GLSL shaders
- **Audio**: PortAudio
- **Signal Processing**: FFTW3 / Eigen
- **UI**: Clay (immediate-mode layout)
- **Build**: CMake 3.20+
- **Testing**: GoogleTest

## Project Status

### ✓ Working (Validated in Cloud Environment)
- CMake build system (3.28.3)
- Eigen3 matrix operations (3.4.0)
- GoogleTest framework (1.14.0)
- Headless testing with Xpra

### ⚠ Needs Configuration
- FFTW3 float linking (`-lfftw3f`)
- SDL2 runtime dependencies
- PortAudio runtime dependencies

## Development Environment

This project is designed to build and test in headless cloud environments. See [CLAUDE.md](CLAUDE.md) for complete setup instructions.

**Tested on:** Ubuntu 24.04 with GCC 13.3.0

## License

See [COPYING.txt](../COPYING.txt) - GNU GPL v3

## Original Project

This is a C++ port of the Python-based [Friture](https://github.com/tlecomte/friture) audio analyzer.
