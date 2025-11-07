/**
 * @file main.cpp
 * @brief Main entry point for Friture C++ Spectrogram Viewer
 *
 * This program creates a real-time spectrogram visualization application
 * that demonstrates the complete signal processing pipeline.
 *
 * Usage:
 *   ./friture [audio_file.wav]
 *
 * If no audio file is provided, generates a test chirp signal.
 *
 * Controls:
 *   SPACE - Pause/Resume
 *   R     - Reset to beginning
 *   H     - Toggle help
 *   1-5   - Change frequency scale (Linear/Log/Mel/ERB/Octave)
 *   +/-   - Adjust FFT size
 *   Q/ESC - Quit
 */

#include <friture/application.hpp>
#include <iostream>
#include <exception>

void printUsage(const char* program_name) {
    std::cout << "Friture C++ - Real-time Spectrogram Viewer" << std::endl;
    std::cout << "\nUsage:" << std::endl;
    std::cout << "  " << program_name << " [audio_file.wav]" << std::endl;
    std::cout << "\nIf no audio file is provided, a test signal will be generated." << std::endl;
    std::cout << "\nKeyboard Controls:" << std::endl;
    std::cout << "  SPACE    - Pause/Resume playback" << std::endl;
    std::cout << "  R        - Reset to beginning" << std::endl;
    std::cout << "  H        - Toggle help overlay" << std::endl;
    std::cout << "  L        - Toggle Live/File mode" << std::endl;
    std::cout << "  D        - Cycle audio input devices" << std::endl;
    std::cout << "  1        - Linear frequency scale" << std::endl;
    std::cout << "  2        - Logarithmic frequency scale" << std::endl;
    std::cout << "  3        - Mel frequency scale" << std::endl;
    std::cout << "  4        - ERB frequency scale" << std::endl;
    std::cout << "  5        - Octave frequency scale" << std::endl;
    std::cout << "  +        - Increase FFT size" << std::endl;
    std::cout << "  -        - Decrease FFT size" << std::endl;
    std::cout << "  Q/ESC    - Quit application" << std::endl;
    std::cout << std::endl;
}

int main(int argc, char* argv[]) {
    try {
        // Print usage if --help requested
        if (argc > 1 && (std::string(argv[1]) == "--help" || std::string(argv[1]) == "-h")) {
            printUsage(argv[0]);
            return 0;
        }

        // Create application
        friture::FritureApp app(1280, 720);

        // Load audio or generate test signal
        if (argc > 1) {
            // Load from file
            if (!app.loadAudioFromFile(argv[1])) {
                std::cerr << "Failed to load audio file: " << argv[1] << std::endl;
                std::cerr << "Generating test signal instead..." << std::endl;
                app.generateChirp(100.0f, 10000.0f, 5.0f);
            }
        } else {
            // Generate test chirp
            std::cout << "\nNo audio file provided - generating test chirp" << std::endl;
            std::cout << "Usage: " << argv[0] << " [audio_file.wav]" << std::endl;
            std::cout << std::endl;
            app.generateChirp(100.0f, 10000.0f, 5.0f);
        }

        // Run application
        app.run();

        std::cout << "\nExiting normally" << std::endl;
        return 0;

    } catch (const std::exception& e) {
        std::cerr << "\nFATAL ERROR: " << e.what() << std::endl;
        return 1;
    } catch (...) {
        std::cerr << "\nFATAL ERROR: Unknown exception" << std::endl;
        return 2;
    }
}
