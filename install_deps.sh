#!/bin/bash
# install_deps.sh - Install all Friture C++ dependencies
# Tested on Ubuntu 24.04

set -e

echo "=== Installing Friture C++ Dependencies ==="

apt-get update

# Core build tools
echo "Installing build tools..."
apt-get install -y \
    build-essential \
    cmake \
    pkg-config \
    git

# Development libraries
echo "Installing development libraries..."
apt-get install -y \
    libsdl2-dev \
    portaudio19-dev \
    libeigen3-dev \
    libgtest-dev \
    libfftw3-dev

# Headless testing
echo "Installing headless testing tools..."
apt-get install -y \
    xpra \
    xvfb

# Additional runtime libraries (optional, for full functionality)
echo "Installing additional runtime libraries..."
apt-get install -y \
    libasound2-dev \
    libpulse-dev \
    libdecor-0-dev \
    libsamplerate0-dev \
    libjack-jackd2-dev || echo "Some optional dependencies failed (non-critical)"

echo ""
echo "✓ All core dependencies installed successfully"
echo ""
echo "Next steps:"
echo "  mkdir build && cd build"
echo "  cmake .."
echo "  make -j\$(nproc)"
