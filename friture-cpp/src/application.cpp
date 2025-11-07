/**
 * @file application.cpp
 * @brief Implementation of main Friture application class
 */

#include <friture/application.hpp>
#include <friture/audio/audio_file_loader.hpp>
#include <iostream>
#include <cmath>
#include <algorithm>
#include <iomanip>

namespace friture {

// ============================================================================
// Constructor / Destructor
// ============================================================================

FritureApp::FritureApp(int window_width, int window_height)
    : settings_(),
      running_(false),
      paused_(false),
      show_help_(false),
      window_(nullptr),
      renderer_(nullptr),
      texture_(nullptr),
      window_width_(window_width),
      window_height_(window_height),
      current_audio_position_(0),
      total_audio_samples_(0),
      fps_(0.0f),
      frame_count_(0)
{
    std::cout << "=== Friture C++ Spectrogram Viewer ===" << std::endl;
    std::cout << "Initializing application..." << std::endl;

    // Initialize SDL
    initializeSDL();

    // Create ring buffer (60 seconds of audio at 48 kHz)
    ring_buffer_ = std::make_unique<RingBuffer<float>>(
        static_cast<size_t>(settings_.sample_rate * 60));

    // Calculate spectrogram display height (use 60% of window height)
    size_t spectrogram_height = static_cast<size_t>(window_height_ * 0.6f);

    // Create processing components
    fft_processor_ = std::make_unique<FFTProcessor>(
        settings_.fft_size, settings_.window_type);

    freq_resampler_ = std::make_unique<FrequencyResampler>(
        settings_.freq_scale,
        settings_.min_freq,
        settings_.max_freq,
        settings_.sample_rate,
        settings_.fft_size,
        spectrogram_height);

    color_transform_ = std::make_unique<ColorTransform>(ColorTheme::CMRMAP);

    spectrogram_image_ = std::make_unique<SpectrogramImage>(
        window_width_, spectrogram_height);

    // Create text renderer for UI overlays
    text_renderer_ = std::make_unique<TextRenderer>(renderer_);
    if (!text_renderer_->isValid()) {
        std::cerr << "Warning: Text rendering unavailable: "
                  << text_renderer_->getError() << std::endl;
        std::cerr << "UI will display without text labels" << std::endl;
    }

    // Allocate working buffers
    fft_input_.resize(settings_.fft_size);
    fft_output_.resize(settings_.fft_size / 2 + 1);
    resampled_.resize(spectrogram_height);
    normalized_.resize(spectrogram_height);
    colors_.resize(spectrogram_height);

    // Create SDL texture now that we know the spectrogram dimensions
    texture_ = SDL_CreateTexture(
        renderer_,
        SDL_PIXELFORMAT_RGBA32,
        SDL_TEXTUREACCESS_STREAMING,
        window_width_,
        static_cast<int>(spectrogram_height)
    );

    if (!texture_) {
        throw std::runtime_error(std::string("Texture creation failed: ") + SDL_GetError());
    }

    std::cout << "Application initialized successfully" << std::endl;
    std::cout << "  Window: " << window_width_ << "x" << window_height_ << std::endl;
    std::cout << "  Spectrogram: " << window_width_ << "x" << spectrogram_height << std::endl;
    std::cout << "  FFT size: " << settings_.fft_size << std::endl;
    std::cout << "  Sample rate: " << settings_.sample_rate << " Hz" << std::endl;
}

FritureApp::~FritureApp() {
    if (texture_) {
        SDL_DestroyTexture(texture_);
    }
    if (renderer_) {
        SDL_DestroyRenderer(renderer_);
    }
    if (window_) {
        SDL_DestroyWindow(window_);
    }
    SDL_Quit();

    std::cout << "\nApplication shutdown complete" << std::endl;
}

// ============================================================================
// Initialization
// ============================================================================

void FritureApp::initializeSDL() {
    // Initialize SDL video subsystem
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        throw std::runtime_error(std::string("SDL initialization failed: ") + SDL_GetError());
    }

    std::cout << "SDL initialized (version " << SDL_MAJOR_VERSION << "."
              << SDL_MINOR_VERSION << "." << SDL_PATCHLEVEL << ")" << std::endl;

    // Create window
    window_ = SDL_CreateWindow(
        "Friture C++ - Spectrogram Viewer",
        SDL_WINDOWPOS_UNDEFINED,
        SDL_WINDOWPOS_UNDEFINED,
        window_width_,
        window_height_,
        SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE
    );

    if (!window_) {
        SDL_Quit();
        throw std::runtime_error(std::string("Window creation failed: ") + SDL_GetError());
    }

    // Create renderer with hardware acceleration if available
    renderer_ = SDL_CreateRenderer(window_, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if (!renderer_) {
        // Fall back to software rendering
        renderer_ = SDL_CreateRenderer(window_, -1, SDL_RENDERER_SOFTWARE);
    }

    if (!renderer_) {
        SDL_DestroyWindow(window_);
        SDL_Quit();
        throw std::runtime_error(std::string("Renderer creation failed: ") + SDL_GetError());
    }

    // Get renderer info
    SDL_RendererInfo info;
    SDL_GetRendererInfo(renderer_, &info);
    std::cout << "SDL Renderer: " << info.name << std::endl;

    // Note: Texture will be created after spectrogram_image_ is initialized
    texture_ = nullptr;
}

// ============================================================================
// Audio Generation (for testing)
// ============================================================================

void FritureApp::generateSineWave(float frequency, float duration) {
    std::cout << "\nGenerating sine wave: " << frequency << " Hz, "
              << duration << " seconds" << std::endl;

    size_t num_samples = static_cast<size_t>(duration * settings_.sample_rate);
    std::vector<float> samples(num_samples);

    for (size_t i = 0; i < num_samples; ++i) {
        float t = static_cast<float>(i) / settings_.sample_rate;
        samples[i] = 0.5f * std::sin(2.0f * M_PI * frequency * t);
    }

    // Write to ring buffer
    ring_buffer_->write(samples.data(), num_samples);
    total_audio_samples_ = num_samples;
    current_audio_position_ = 0;

    std::cout << "Generated " << num_samples << " samples" << std::endl;
}

void FritureApp::generateChirp(float f_start, float f_end, float duration) {
    std::cout << "\nGenerating chirp: " << f_start << " Hz → " << f_end
              << " Hz, " << duration << " seconds" << std::endl;

    size_t num_samples = static_cast<size_t>(duration * settings_.sample_rate);
    std::vector<float> samples(num_samples);

    float k = (f_end - f_start) / duration; // Hz/sec

    for (size_t i = 0; i < num_samples; ++i) {
        float t = static_cast<float>(i) / settings_.sample_rate;
        float phase = 2.0f * M_PI * (f_start * t + 0.5f * k * t * t);
        samples[i] = 0.5f * std::sin(phase);
    }

    // Write to ring buffer
    ring_buffer_->write(samples.data(), num_samples);
    total_audio_samples_ = num_samples;
    current_audio_position_ = 0;

    std::cout << "Generated " << num_samples << " samples" << std::endl;
}

bool FritureApp::loadAudioFromFile(const char* filename) {
    std::cout << "\nLoading audio from file: " << filename << std::endl;

    AudioFileLoader loader;
    std::vector<float> samples;
    float file_sample_rate;

    if (!loader.load(filename, samples, file_sample_rate)) {
        std::cerr << "Failed to load WAV file: " << loader.getError() << std::endl;
        std::cerr << "Generating test chirp instead..." << std::endl;
        generateChirp(100.0f, 10000.0f, 5.0f);
        return false;
    }

    // Check if sample rate matches our processing sample rate
    if (std::abs(file_sample_rate - settings_.sample_rate) > 1.0f) {
        std::cout << "Warning: File sample rate (" << file_sample_rate << " Hz) "
                  << "differs from processing sample rate (" << settings_.sample_rate << " Hz)"
                  << std::endl;
        std::cout << "Resampling not yet implemented - using file as-is" << std::endl;
        // TODO: Implement resampling in future version
        settings_.sample_rate = file_sample_rate;
    }

    // Write samples to ring buffer
    ring_buffer_->write(samples.data(), samples.size());
    total_audio_samples_ = samples.size();
    current_audio_position_ = 0;

    const WavInfo& info = loader.getInfo();
    std::cout << "Successfully loaded: " << info.getFormatDescription() << std::endl;
    std::cout << "Total samples: " << total_audio_samples_ << std::endl;

    return true;
}

// ============================================================================
// Main Loop
// ============================================================================

void FritureApp::run() {
    running_ = true;
    last_frame_time_ = std::chrono::steady_clock::now();
    last_fft_time_ = std::chrono::steady_clock::now();

    std::cout << "\n=== Application Running ===" << std::endl;
    std::cout << "Press 'H' for help" << std::endl;
    std::cout << "Press 'Q' or ESC to quit" << std::endl;

    while (running_) {
        auto frame_start = std::chrono::steady_clock::now();

        // Handle events
        handleEvents();

        // Process audio if not paused
        if (!paused_ && current_audio_position_ < total_audio_samples_) {
            auto now = std::chrono::steady_clock::now();
            auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
                now - last_fft_time_);

            // Calculate time per column based on overlap
            float ms_per_column = settings_.getTimePerColumn() * 1000.0f;

            // Process new FFT frame when enough time has elapsed
            if (elapsed.count() >= ms_per_column) {
                processAudioFrame();
                last_fft_time_ = now;
            }
        }

        // Render frame
        renderFrame();

        // Calculate FPS
        auto frame_end = std::chrono::steady_clock::now();
        auto frame_duration = std::chrono::duration_cast<std::chrono::microseconds>(
            frame_end - frame_start);

        fps_ = fps_ * 0.95f + (1000000.0f / frame_duration.count()) * 0.05f; // Smoothed FPS
        frame_count_++;

        // Cap at ~60 FPS
        if (frame_duration.count() < 16666) {
            SDL_Delay(1);
        }
    }
}

// ============================================================================
// Event Handling
// ============================================================================

void FritureApp::handleEvents() {
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        switch (event.type) {
            case SDL_QUIT:
                running_ = false;
                break;

            case SDL_KEYDOWN:
                handleKeyboard(event.key);
                break;

            case SDL_WINDOWEVENT:
                if (event.window.event == SDL_WINDOWEVENT_RESIZED) {
                    window_width_ = event.window.data1;
                    window_height_ = event.window.data2;
                    // TODO: Resize spectrogram image and texture
                }
                break;
        }
    }
}

void FritureApp::handleKeyboard(const SDL_KeyboardEvent& event) {
    switch (event.keysym.sym) {
        case SDLK_q:
        case SDLK_ESCAPE:
            running_ = false;
            break;

        case SDLK_SPACE:
            paused_ = !paused_;
            std::cout << (paused_ ? "Paused" : "Resumed") << std::endl;
            break;

        case SDLK_h:
            show_help_ = !show_help_;
            break;

        case SDLK_r:
            // Reset - go back to beginning
            current_audio_position_ = 0;
            spectrogram_image_->clear();
            std::cout << "Reset to beginning" << std::endl;
            break;

        case SDLK_c:
            // Cycle color theme
            // TODO: Implement theme cycling
            std::cout << "Color theme cycling not implemented yet" << std::endl;
            break;

        case SDLK_1:
            settings_.freq_scale = FrequencyScale::Linear;
            updateProcessingComponents();
            std::cout << "Frequency scale: Linear" << std::endl;
            break;

        case SDLK_2:
            settings_.freq_scale = FrequencyScale::Logarithmic;
            updateProcessingComponents();
            std::cout << "Frequency scale: Logarithmic" << std::endl;
            break;

        case SDLK_3:
            settings_.freq_scale = FrequencyScale::Mel;
            updateProcessingComponents();
            std::cout << "Frequency scale: Mel" << std::endl;
            break;

        case SDLK_4:
            settings_.freq_scale = FrequencyScale::ERB;
            updateProcessingComponents();
            std::cout << "Frequency scale: ERB" << std::endl;
            break;

        case SDLK_5:
            settings_.freq_scale = FrequencyScale::Octave;
            updateProcessingComponents();
            std::cout << "Frequency scale: Octave" << std::endl;
            break;

        case SDLK_EQUALS:  // + key
        case SDLK_PLUS:
            // Increase FFT size
            if (settings_.fft_size < 16384) {
                settings_.fft_size *= 2;
                updateProcessingComponents();
                std::cout << "FFT size: " << settings_.fft_size << std::endl;
            }
            break;

        case SDLK_MINUS:
            // Decrease FFT size
            if (settings_.fft_size > 32) {
                settings_.fft_size /= 2;
                updateProcessingComponents();
                std::cout << "FFT size: " << settings_.fft_size << std::endl;
            }
            break;
    }
}

void FritureApp::updateProcessingComponents() {
    // Recreate components with new settings
    size_t spectrogram_height = spectrogram_image_->getHeight();

    fft_processor_ = std::make_unique<FFTProcessor>(
        settings_.fft_size, settings_.window_type);

    freq_resampler_ = std::make_unique<FrequencyResampler>(
        settings_.freq_scale,
        settings_.min_freq,
        settings_.max_freq,
        settings_.sample_rate,
        settings_.fft_size,
        spectrogram_height);

    // Resize buffers
    fft_input_.resize(settings_.fft_size);
    fft_output_.resize(settings_.fft_size / 2 + 1);

    // Clear spectrogram
    spectrogram_image_->clear();
}

// ============================================================================
// Audio Processing
// ============================================================================

void FritureApp::processAudioFrame() {
    // Check if we have enough samples
    size_t samples_needed = settings_.fft_size;
    if (current_audio_position_ + samples_needed > total_audio_samples_) {
        // Reached end of audio
        return;
    }

    // Read samples from ring buffer
    ring_buffer_->read(current_audio_position_, fft_input_.data(), samples_needed);

    // Advance position by hop size (based on overlap)
    size_t hop_size = settings_.getSamplesPerColumn();
    current_audio_position_ += hop_size;

    // FFT processing
    fft_processor_->process(fft_input_.data(), fft_output_.data());

    // Frequency resampling
    freq_resampler_->resample(fft_output_.data(), resampled_.data());

    // Normalize to [0, 1] range
    size_t height = spectrogram_image_->getHeight();
    for (size_t i = 0; i < height; ++i) {
        normalized_[i] = (resampled_[i] - settings_.spec_min_db) /
                        (settings_.spec_max_db - settings_.spec_min_db);
        normalized_[i] = std::clamp(normalized_[i], 0.0f, 1.0f);
    }

    // Color transformation
    color_transform_->transformColumn(normalized_.data(), height, colors_.data());

    // Add column to spectrogram image
    spectrogram_image_->addColumn(colors_.data(), height);
}

// ============================================================================
// Rendering
// ============================================================================

void FritureApp::renderFrame() {
    // Clear screen to dark gray
    SDL_SetRenderDrawColor(renderer_, 30, 30, 30, 255);
    SDL_RenderClear(renderer_);

    // Update texture with spectrogram pixels
    const uint32_t* pixels = spectrogram_image_->getPixelData();
    int texture_width = static_cast<int>(spectrogram_image_->getWidth());
    int texture_height = static_cast<int>(spectrogram_image_->getHeight());

    // Lock texture and copy pixels
    void* texture_pixels;
    int pitch;
    if (SDL_LockTexture(texture_, nullptr, &texture_pixels, &pitch) == 0) {
        // Copy pixels - spectrogram image is stored column-major, SDL expects row-major
        uint32_t* dst = static_cast<uint32_t*>(texture_pixels);

        for (int y = 0; y < texture_height; ++y) {
            for (int x = 0; x < texture_width; ++x) {
                // Spectrogram image stores pixels column-major
                size_t src_idx = x * texture_height + y;
                size_t dst_idx = y * texture_width + x;
                dst[dst_idx] = pixels[src_idx];
            }
        }

        SDL_UnlockTexture(texture_);
    }

    // Render texture
    SDL_Rect dst_rect = {0, 0, window_width_, texture_height};
    SDL_RenderCopy(renderer_, texture_, nullptr, &dst_rect);

    // Draw UI overlay
    drawUI(renderer_);

    // Present
    SDL_RenderPresent(renderer_);
}

void FritureApp::drawUI(SDL_Renderer* renderer) {
    if (!text_renderer_ || !text_renderer_->isValid()) {
        // Fallback to simple colored rectangles if text rendering unavailable
        drawUIFallback(renderer);
        return;
    }

    // Define colors
    SDL_Color white = {255, 255, 255, 255};
    SDL_Color green = {0, 255, 0, 255};
    SDL_Color yellow = {255, 255, 0, 255};
    SDL_Color red = {255, 0, 0, 255};
    SDL_Color black = {0, 0, 0, 200};
    SDL_Color gray = {180, 180, 180, 255};

    // ========================================================================
    // Status Bar
    // ========================================================================

    // Draw semi-transparent status bar background
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 200);
    SDL_Rect status_bar = {0, window_height_ - 30, window_width_, 30};
    SDL_RenderFillRect(renderer, &status_bar);

    // FPS counter (left side)
    std::string fps_text = "FPS: " + std::to_string(static_cast<int>(fps_));
    SDL_Color fps_color = fps_ >= 55.0f ? green : (fps_ >= 30.0f ? yellow : red);
    text_renderer_->renderTextWithShadow(fps_text, 10, window_height_ - 25,
                                        fps_color, black, 16, 1);

    // Settings display (center)
    std::string fft_text = "FFT: " + std::to_string(settings_.fft_size);
    text_renderer_->renderTextWithShadow(fft_text, 120, window_height_ - 25,
                                        white, black, 16, 1);

    // Frequency scale
    const char* scale_names[] = {"Linear", "Log", "Mel", "ERB", "Octave"};
    int scale_idx = static_cast<int>(settings_.freq_scale);
    std::string scale_text = "Scale: " + std::string(scale_names[scale_idx]);
    text_renderer_->renderTextWithShadow(scale_text, 250, window_height_ - 25,
                                        white, black, 16, 1);

    // Frequency range
    char freq_range_buf[64];
    std::snprintf(freq_range_buf, sizeof(freq_range_buf),
                 "Range: %.0f-%.0f Hz", settings_.min_freq, settings_.max_freq);
    text_renderer_->renderTextWithShadow(freq_range_buf, 400, window_height_ - 25,
                                        gray, black, 16, 1);

    // Paused indicator (right side)
    if (paused_) {
        text_renderer_->renderTextWithShadow("PAUSED", window_width_ - 90,
                                            window_height_ - 25, red, black, 16, 1);
    }

    // ========================================================================
    // Frequency Axis Labels (Left Side)
    // ========================================================================

    int spectrogram_height = static_cast<int>(spectrogram_image_->getHeight());
    int num_labels = 10; // Draw 10 frequency labels

    for (int i = 0; i <= num_labels; ++i) {
        float t = static_cast<float>(i) / num_labels;
        int y = static_cast<int>(spectrogram_height * (1.0f - t)); // Flip Y (top = high freq)

        // Calculate frequency at this position based on scale
        float freq = 0.0f;
        float min_f = settings_.min_freq;
        float max_f = settings_.max_freq;

        switch (settings_.freq_scale) {
            case FrequencyScale::Linear:
                freq = min_f + t * (max_f - min_f);
                break;
            case FrequencyScale::Logarithmic:
                if (min_f > 0) {
                    float log_min = std::log10(min_f);
                    float log_max = std::log10(max_f);
                    freq = std::pow(10.0f, log_min + t * (log_max - log_min));
                }
                break;
            case FrequencyScale::Mel:
            case FrequencyScale::ERB:
            case FrequencyScale::Octave:
                // Approximate - just use linear for now
                freq = min_f + t * (max_f - min_f);
                break;
        }

        // Format frequency label
        char freq_label[32];
        if (freq >= 1000.0f) {
            std::snprintf(freq_label, sizeof(freq_label), "%.1fk", freq / 1000.0f);
        } else {
            std::snprintf(freq_label, sizeof(freq_label), "%.0f", freq);
        }

        // Draw label on left edge
        text_renderer_->renderTextWithShadow(freq_label, 5, y - 6,
                                            white, black, 12, 1);
    }

    // ========================================================================
    // Help Overlay
    // ========================================================================

    if (show_help_) {
        // Semi-transparent background
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 220);
        int help_w = window_width_ / 2;
        int help_h = window_height_ / 2;
        int help_x = window_width_ / 4;
        int help_y = window_height_ / 4;
        SDL_Rect help_bg = {help_x, help_y, help_w, help_h};
        SDL_RenderFillRect(renderer, &help_bg);

        // White border
        SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
        SDL_RenderDrawRect(renderer, &help_bg);

        // Title
        text_renderer_->renderTextCentered("Friture C++ - Keyboard Controls",
                                          window_width_ / 2, help_y + 20,
                                          white, 20);

        // Help text
        int line_y = help_y + 60;
        int line_spacing = 30;

        text_renderer_->renderText("SPACE  - Pause/Resume", help_x + 20, line_y, white, 16);
        line_y += line_spacing;

        text_renderer_->renderText("R      - Reset to beginning", help_x + 20, line_y, white, 16);
        line_y += line_spacing;

        text_renderer_->renderText("H      - Toggle this help", help_x + 20, line_y, white, 16);
        line_y += line_spacing;

        text_renderer_->renderText("1-5    - Frequency scale (Linear/Log/Mel/ERB/Octave)",
                                  help_x + 20, line_y, white, 16);
        line_y += line_spacing;

        text_renderer_->renderText("+/-    - FFT size", help_x + 20, line_y, white, 16);
        line_y += line_spacing;

        text_renderer_->renderText("Q/ESC  - Quit", help_x + 20, line_y, white, 16);
        line_y += line_spacing;

        // Footer
        text_renderer_->renderTextCentered("Press H to close",
                                          window_width_ / 2, help_y + help_h - 40,
                                          gray, 14);
    }
}

void FritureApp::drawUIFallback(SDL_Renderer* renderer) {
    // Fallback UI using colored rectangles (no text)

    // Status bar background
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 200);
    SDL_Rect status_bar = {0, window_height_ - 30, window_width_, 30};
    SDL_RenderFillRect(renderer, &status_bar);

    // FPS indicator (simple colored bar)
    int fps_width = static_cast<int>(fps_ * 2); // 60 FPS = 120 pixels
    fps_width = std::clamp(fps_width, 0, 200);

    // Color based on performance
    if (fps_ >= 55.0f) {
        SDL_SetRenderDrawColor(renderer, 0, 255, 0, 255); // Green
    } else if (fps_ >= 30.0f) {
        SDL_SetRenderDrawColor(renderer, 255, 255, 0, 255); // Yellow
    } else {
        SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255); // Red
    }

    SDL_Rect fps_bar = {10, window_height_ - 20, fps_width, 10};
    SDL_RenderFillRect(renderer, &fps_bar);

    // Paused indicator
    if (paused_) {
        SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255);
        SDL_Rect pause_indicator = {window_width_ - 50, window_height_ - 25, 40, 20};
        SDL_RenderFillRect(renderer, &pause_indicator);
    }

    // Help overlay
    if (show_help_) {
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 220);
        SDL_Rect help_bg = {window_width_ / 4, window_height_ / 4,
                           window_width_ / 2, window_height_ / 2};
        SDL_RenderFillRect(renderer, &help_bg);

        SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
        SDL_RenderDrawRect(renderer, &help_bg);
    }
}

} // namespace friture
