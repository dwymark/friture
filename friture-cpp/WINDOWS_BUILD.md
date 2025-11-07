# Building Friture C++ on Windows 11

This guide provides step-by-step instructions for building and running the Friture C++ project on Windows 11 using CMake and Visual Studio.

## Prerequisites

Before starting, ensure you have the following installed:

- **Git** - Version control system
- **CMake** (3.20 or later) - Build system generator
- **Visual Studio 2022** with C++ components:
  - Desktop development with C++ workload
  - C++ CMake tools for Windows
  - Windows 10/11 SDK

You can download Visual Studio 2022 Community Edition (free) from [visualstudio.microsoft.com](https://visualstudio.microsoft.com/).

## Step 1: Install vcpkg Package Manager

vcpkg is Microsoft's cross-platform package manager for C/C++ libraries. It simplifies dependency management on Windows.

### 1.1 Clone vcpkg

Open PowerShell or Command Prompt and run:

```powershell
cd C:\
git clone https://github.com/microsoft/vcpkg.git
cd vcpkg
```

### 1.2 Bootstrap vcpkg

Run the bootstrap script:

```powershell
.\bootstrap-vcpkg.bat
```

### 1.3 Integrate with Visual Studio (Optional but Recommended)

```powershell
.\vcpkg integrate install
```

This command makes installed packages available to all Visual Studio projects automatically.

### 1.4 Add vcpkg to PATH (Optional)

For easier command-line usage, add `C:\vcpkg` to your system PATH environment variable.

## Step 2: Install Project Dependencies

Install all required libraries using vcpkg. Run these commands from the `C:\vcpkg` directory:

```powershell
# Core dependencies
.\vcpkg install sdl2:x64-windows
.\vcpkg install sdl2-ttf:x64-windows
.\vcpkg install fftw3:x64-windows
.\vcpkg install eigen3:x64-windows
.\vcpkg install gtest:x64-windows

# Optional: Install PortAudio if available
# Note: PortAudio may not be in vcpkg; RtAudio is used as alternative
.\vcpkg install portaudio:x64-windows
```

**Installation time**: This may take 10-30 minutes depending on your internet connection and system performance.

### Why `x64-windows`?

The `:x64-windows` suffix specifies the target triplet (64-bit Windows dynamic libraries). For static linking, use `:x64-windows-static`.

## Step 3: Clone the Friture Repository

If you haven't already cloned the repository:

```powershell
cd C:\
git clone https://github.com/dwymark/friture.git
cd friture\friture-cpp
```

## Step 4: Configure the Project with CMake

Create a build directory and configure the project:

```powershell
mkdir build
cd build

# Configure with Visual Studio 2022 generator and vcpkg toolchain
cmake .. -G "Visual Studio 17 2022" -A x64 ^
  -DCMAKE_TOOLCHAIN_FILE=C:/vcpkg/scripts/buildsystems/vcpkg.cmake ^
  -DBUILD_TESTS=ON ^
  -DBUILD_EXAMPLES=ON
```

### Configuration Options

- `-G "Visual Studio 17 2022"` - Uses Visual Studio 2022 as the generator
- `-A x64` - Specifies 64-bit architecture
- `-DCMAKE_TOOLCHAIN_FILE=...` - Points to vcpkg's CMake integration
- `-DBUILD_TESTS=ON` - Enables building test executables
- `-DBUILD_EXAMPLES=ON` - Enables building example programs

**Note**: If CMake reports missing dependencies, verify they are installed with:
```powershell
C:\vcpkg\vcpkg list
```

## Step 5: Build the Project

### Option A: Build with CMake Command Line

Build the Release configuration:

```powershell
cmake --build . --config Release
```

Build the Debug configuration:

```powershell
cmake --build . --config Debug
```

For parallel builds (faster), add the `-j` flag:

```powershell
cmake --build . --config Release -j 8
```

### Option B: Build with Visual Studio IDE

1. Open the generated solution file in the build directory:
   ```powershell
   start friture-cpp.sln
   ```

2. In Visual Studio:
   - Select **Release** or **Debug** from the configuration dropdown
   - Press **Ctrl+Shift+B** or select **Build > Build Solution**

## Step 6: Run the Application

After a successful build, executables are located in:
- Release: `build\src\Release\friture.exe`
- Debug: `build\src\Debug\friture.exe`

Run from PowerShell:

```powershell
# From the build directory
.\src\Release\friture.exe
```

Or double-click the executable in Windows Explorer.

### Running Examples

Example programs are in the `examples` directory:

```powershell
# Test Eigen integration
.\examples\Release\test_eigen.exe

# Test FFT functionality
.\examples\Release\test_fft.exe

# Test SDL2 rendering
.\examples\Release\test_sdl2.exe

# Test audio pipeline
.\examples\Release\pipeline_test.exe
```

## Step 7: Running Tests (Optional)

If you built with `-DBUILD_TESTS=ON`, run the test suite:

```powershell
# Run all tests with CTest
ctest -C Release

# Or run CTest with verbose output
ctest -C Release -V

# Or run individual test executables directly
.\tests\unit\Release\processing_tests.exe
```

## Troubleshooting

### Missing DLL Errors

If you encounter errors like "SDL2.dll not found" when running executables:

**Solution 1**: Copy DLLs to executable directory
```powershell
# Copy required DLLs from vcpkg to your build output
copy C:\vcpkg\installed\x64-windows\bin\*.dll .\src\Release\
```

**Solution 2**: Add vcpkg bin directory to PATH
```powershell
# Add to system PATH (requires admin privileges)
$env:Path += ";C:\vcpkg\installed\x64-windows\bin"
```

**Solution 3**: Use static linking
Reconfigure with static triplet:
```powershell
cmake .. -G "Visual Studio 17 2022" -A x64 ^
  -DCMAKE_TOOLCHAIN_FILE=C:/vcpkg/scripts/buildsystems/vcpkg.cmake ^
  -DVCPKG_TARGET_TRIPLET=x64-windows-static
```

### CMake Cannot Find vcpkg Packages

Ensure you specified the toolchain file correctly:
```powershell
-DCMAKE_TOOLCHAIN_FILE=C:/vcpkg/scripts/buildsystems/vcpkg.cmake
```

Verify packages are installed:
```powershell
C:\vcpkg\vcpkg list
```

### "PkgConfig not found" Error

This is expected on Windows. The CMakeLists.txt has been updated to make PkgConfig optional for Windows builds.

### FFTW3 Not Found

Ensure FFTW3 is installed:
```powershell
C:\vcpkg\vcpkg install fftw3:x64-windows
```

If the issue persists, you can disable FFTW and use Eigen for FFT:
```powershell
cmake .. -G "Visual Studio 17 2022" -A x64 ^
  -DCMAKE_TOOLCHAIN_FILE=C:/vcpkg/scripts/buildsystems/vcpkg.cmake ^
  -DUSE_FFTW=OFF
```

### Build Errors with RtAudio

RtAudio should build automatically with WASAPI (Windows Audio Session API) support on Windows. If you encounter issues:

1. Check that you're building for 64-bit: `-A x64`
2. Verify Windows SDK is installed in Visual Studio
3. Check RtAudio CMake options in the console output

### PortAudio vs RtAudio

This project uses **RtAudio** (included in `third_party/rtaudio`) for cross-platform audio I/O. RtAudio provides native WASAPI support on Windows and doesn't require external installation.

PortAudio is optional and may not be needed unless specifically required by your workflow.

## Alternative Build Methods

### Using Ninja (Faster Builds)

Ninja is a faster build system than MSBuild. To use it:

1. Install Ninja via vcpkg:
   ```powershell
   C:\vcpkg\vcpkg install ninja
   ```

2. Configure with Ninja generator:
   ```powershell
   cmake .. -G "Ninja" ^
     -DCMAKE_TOOLCHAIN_FILE=C:/vcpkg/scripts/buildsystems/vcpkg.cmake ^
     -DCMAKE_BUILD_TYPE=Release
   ```

3. Build:
   ```powershell
   ninja
   ```

### Using CMake GUI

For a graphical configuration experience:

1. Launch `cmake-gui`
2. Set "Where is the source code" to `C:/friture/friture-cpp`
3. Set "Where to build the binaries" to `C:/friture/friture-cpp/build`
4. Click **Configure**, select Visual Studio 2022
5. Add entry `CMAKE_TOOLCHAIN_FILE` = `C:/vcpkg/scripts/buildsystems/vcpkg.cmake`
6. Click **Generate**
7. Click **Open Project** to launch Visual Studio

## Project Structure

After building, your directory structure will look like:

```
friture-cpp/
├── build/
│   ├── src/
│   │   ├── Release/
│   │   │   └── friture.exe          # Main application
│   │   └── Debug/
│   │       └── friture.exe
│   ├── examples/
│   │   └── Release/
│   │       ├── test_eigen.exe       # Dependency tests
│   │       ├── test_fft.exe
│   │       ├── test_sdl2.exe
│   │       └── pipeline_test.exe
│   └── tests/
│       └── unit/
│           └── Release/
│               └── processing_tests.exe
├── include/                          # Header files
├── src/                              # Source files
├── examples/                         # Example programs
├── tests/                            # Unit tests
└── third_party/                      # Third-party libraries
    └── rtaudio/                      # RtAudio audio I/O library
```

## Development Workflow

### Recommended: Use Visual Studio Code

For a modern development experience, consider using Visual Studio Code with:

1. **CMake Tools** extension
2. **C/C++** extension by Microsoft

This provides:
- IntelliSense code completion
- Integrated CMake configuration
- Debugging support
- Git integration

### Incremental Builds

After making code changes:

```powershell
# Rebuild only changed files (much faster)
cmake --build . --config Release

# Or in Visual Studio, just press Ctrl+Shift+B
```

### Cleaning the Build

To start fresh:

```powershell
# Remove build directory
cd ..
rmdir /s /q build

# Reconfigure and rebuild
mkdir build
cd build
cmake .. -G "Visual Studio 17 2022" -A x64 ^
  -DCMAKE_TOOLCHAIN_FILE=C:/vcpkg/scripts/buildsystems/vcpkg.cmake
cmake --build . --config Release
```

## Next Steps

- **Read the Documentation**: Check [README.md](README.md) and [CLAUDE.md](CLAUDE.md) for architecture details
- **Explore Examples**: Run example programs to test functionality
- **Run Tests**: Execute the test suite to verify your build
- **Start Developing**: Make changes and contribute to the project

## Getting Help

If you encounter issues:

1. Check this guide's Troubleshooting section
2. Review the main [README.md](README.md)
3. Search existing GitHub issues
4. Open a new issue with:
   - Windows version (run `winver`)
   - Visual Studio version
   - CMake version (`cmake --version`)
   - vcpkg version
   - Full error messages and logs

## Additional Resources

- [CMake Documentation](https://cmake.org/documentation/)
- [vcpkg Documentation](https://vcpkg.io/)
- [Visual Studio CMake Projects](https://learn.microsoft.com/en-us/cpp/build/cmake-projects-in-visual-studio)
- [RtAudio Documentation](https://www.music.mcgill.ca/~gary/rtaudio/)
- [SDL2 Documentation](https://wiki.libsdl.org/)
