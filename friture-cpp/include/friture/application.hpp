/**
 * @file application.hpp
 * @brief Main Friture application class
 *
 * This class integrates all components (FFT, resampling, color transform,
 * spectrogram image) with SDL2 rendering to create a working spectrogram
 * visualization application.
 *
 * @author Friture C++ Port
 * @date 2025-11-06
 */

#ifndef FRITURE_APPLICATION_HPP
#define FRITURE_APPLICATION_HPP

#include <friture/settings.hpp>
#include <friture/ringbuffer.hpp>
#include <friture/fft_processor.hpp>
#include <friture/frequency_resampler.hpp>
#include <friture/color_transform.hpp>
#include <friture/spectrogram_image.hpp>
#include <friture/ui/text_renderer.hpp>

#include <SDL2/SDL.h>
#include <memory>
#include <vector>
#include <chrono>

namespace friture {

/**
 * @brief Main application class for Friture spectrogram viewer
 *
 * This class manages the entire application lifecycle:
 * - SDL2 window and rendering
 * - Audio processing pipeline (FFT → Resample → Color → Image)
 * - User input handling
 * - Frame timing and display
 *
 * Usage:
 * @code
 * FritureApp app(1920, 1080);
 * app.loadAudioFromFile("test.wav");  // or use generateSineWave()
 * app.run();  // Runs until user quits
 * @endcode
 */
class FritureApp {
public:
    /**
     * @brief Construct application with window dimensions
     * @param window_width Window width in pixels
     * @param window_height Window height in pixels
     * @throws std::runtime_error if SDL initialization fails
     */
    FritureApp(int window_width = 1280, int window_height = 720);

    /**
     * @brief Destructor - cleans up SDL resources
     */
    ~FritureApp();

    /**
     * @brief Main application loop
     *
     * Runs until user closes window or presses Q.
     * Handles:
     * - Event processing (keyboard, mouse, window events)
     * - Audio frame processing (FFT pipeline)
     * - Rendering to screen
     * - Frame timing (targets 60 FPS)
     */
    void run();

    /**
     * @brief Load audio from WAV file
     * @param filename Path to WAV file (mono or stereo, 16-bit PCM)
     * @return true if successful, false on error
     *
     * Loads audio into ring buffer for processing.
     * Currently supports: 16-bit PCM, mono/stereo, any sample rate
     */
    bool loadAudioFromFile(const char* filename);

    /**
     * @brief Generate synthetic sine wave for testing
     * @param frequency Frequency in Hz
     * @param duration Duration in seconds
     *
     * Useful for testing without audio files.
     */
    void generateSineWave(float frequency, float duration);

    /**
     * @brief Generate synthetic chirp for testing
     * @param f_start Start frequency in Hz
     * @param f_end End frequency in Hz
     * @param duration Duration in seconds
     */
    void generateChirp(float f_start, float f_end, float duration);

    /**
     * @brief Get current settings (for modification)
     * @return Reference to settings object
     */
    SpectrogramSettings& getSettings() { return settings_; }

    /**
     * @brief Check if application is running
     * @return true if running, false if quit
     */
    bool isRunning() const { return running_; }

private:
    /**
     * @brief Initialize SDL2 components
     * @throws std::runtime_error on failure
     */
    void initializeSDL();

    /**
     * @brief Handle SDL events (keyboard, mouse, window)
     */
    void handleEvents();

    /**
     * @brief Process one audio frame through FFT pipeline
     *
     * Pipeline:
     * 1. Read samples from ring buffer
     * 2. FFT processing → spectrum
     * 3. Frequency resampling → screen height
     * 4. Normalize to [0,1]
     * 5. Color transform → RGBA
     * 6. Add column to spectrogram image
     */
    void processAudioFrame();

    /**
     * @brief Render current spectrogram to screen
     *
     * Updates SDL texture with spectrogram pixels and renders.
     * Also draws UI overlay (FPS, settings, etc.)
     */
    void renderFrame();

    /**
     * @brief Draw UI overlay with status info
     * @param renderer SDL renderer
     */
    void drawUI(SDL_Renderer* renderer);

    /**
     * @brief Draw UI fallback without text (colored rectangles)
     * @param renderer SDL renderer
     *
     * Used when text rendering is unavailable
     */
    void drawUIFallback(SDL_Renderer* renderer);

    /**
     * @brief Handle keyboard input
     * @param event SDL keyboard event
     */
    void handleKeyboard(const SDL_KeyboardEvent& event);

    /**
     * @brief Update settings and recreate processing components
     *
     * Called when user changes settings (FFT size, frequency scale, etc.)
     */
    void updateProcessingComponents();

    // ========================================================================
    // Settings and State
    // ========================================================================

    SpectrogramSettings settings_;  ///< Current settings
    bool running_;                   ///< Application running flag
    bool paused_;                    ///< Playback paused flag
    bool show_help_;                 ///< Show help overlay

    // ========================================================================
    // SDL Components
    // ========================================================================

    SDL_Window* window_;             ///< SDL window
    SDL_Renderer* renderer_;         ///< SDL renderer
    SDL_Texture* texture_;           ///< Spectrogram texture
    int window_width_;               ///< Window width
    int window_height_;              ///< Window height

    // ========================================================================
    // Audio Data
    // ========================================================================

    std::unique_ptr<RingBuffer<float>> ring_buffer_;  ///< Audio sample buffer
    size_t current_audio_position_;  ///< Current read position in audio
    size_t total_audio_samples_;     ///< Total audio samples loaded

    // ========================================================================
    // Processing Components
    // ========================================================================

    std::unique_ptr<FFTProcessor> fft_processor_;
    std::unique_ptr<FrequencyResampler> freq_resampler_;
    std::unique_ptr<ColorTransform> color_transform_;
    std::unique_ptr<SpectrogramImage> spectrogram_image_;
    std::unique_ptr<TextRenderer> text_renderer_;

    // ========================================================================
    // Temporary Buffers (reused each frame)
    // ========================================================================

    std::vector<float> fft_input_;       ///< FFT input buffer
    std::vector<float> fft_output_;      ///< FFT output (spectrum)
    std::vector<float> resampled_;       ///< Resampled spectrum
    std::vector<float> normalized_;      ///< Normalized [0,1] values
    std::vector<uint32_t> colors_;       ///< RGBA color column

    // ========================================================================
    // Timing
    // ========================================================================

    std::chrono::steady_clock::time_point last_frame_time_;
    std::chrono::steady_clock::time_point last_fft_time_;
    float fps_;                          ///< Current FPS
    int frame_count_;                    ///< Total frames rendered

    // Prevent copying
    FritureApp(const FritureApp&) = delete;
    FritureApp& operator=(const FritureApp&) = delete;
};

} // namespace friture

#endif // FRITURE_APPLICATION_HPP
