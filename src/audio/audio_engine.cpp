/**
 * @file audio_engine.cpp
 * @brief Implementation of AudioEngine
 */

#include <friture/audio/audio_engine.hpp>
#include <RtAudio.h>
#include <iostream>
#include <cmath>
#include <algorithm>

namespace friture {

// ============================================================================
// Constructor / Destructor
// ============================================================================

AudioEngine::AudioEngine(size_t sample_rate,
                        size_t buffer_size,
                        size_t ring_buffer_seconds)
    : sample_rate_(sample_rate),
      buffer_size_(buffer_size),
      current_device_id_(0),
      device_set_(false),
      is_running_(false),
      input_level_(0.0f)
{
    audio_ = std::make_unique<RtAudio>();

    // Show warnings to help with debugging
    audio_->showWarnings(true);

    // Create ring buffer
    size_t ring_buffer_size = sample_rate * ring_buffer_seconds;
    ring_buffer_ = std::make_unique<RingBuffer<float>>(ring_buffer_size);

    std::cout << "AudioEngine initialized" << std::endl;
    std::cout << "  Sample rate: " << sample_rate_ << " Hz" << std::endl;
    std::cout << "  Buffer size: " << buffer_size_ << " frames" << std::endl;
    std::cout << "  Ring buffer: " << ring_buffer_seconds << " seconds ("
              << ring_buffer_size << " samples)" << std::endl;
}

AudioEngine::~AudioEngine() {
    stop();
}

// ============================================================================
// Device Enumeration
// ============================================================================

std::vector<AudioDeviceInfo> AudioEngine::getInputDevices() const {
    std::vector<AudioDeviceInfo> devices;

    if (!audio_) {
        return devices;
    }

    unsigned int device_count = audio_->getDeviceCount();
    unsigned int default_input = audio_->getDefaultInputDevice();

    for (unsigned int id = 0; id < device_count; id++) {
        RtAudio::DeviceInfo info = audio_->getDeviceInfo(id);

        // Only include devices with input channels
        if (info.inputChannels > 0) {
            AudioDeviceInfo device_info;
            device_info.id = id;
            device_info.name = info.name;
            device_info.input_channels = info.inputChannels;
            device_info.output_channels = info.outputChannels;
            device_info.is_default = (id == default_input);

            // Build sample rate bitmask
            device_info.sample_rates = 0;
            for (auto rate : info.sampleRates) {
                if (rate == 44100) device_info.sample_rates |= (1 << 0);
                if (rate == 48000) device_info.sample_rates |= (1 << 1);
                if (rate == 96000) device_info.sample_rates |= (1 << 2);
            }

            devices.push_back(device_info);
        }
    }

    return devices;
}

// ============================================================================
// Device Selection
// ============================================================================

bool AudioEngine::setInputDevice(unsigned int device_id) {
    // Stop current stream if running
    if (is_running_) {
        stop();
    }

    // Validate device
    RtAudio::DeviceInfo info = audio_->getDeviceInfo(device_id);
    if (info.inputChannels == 0) {
        error_message_ = "Device has no input channels or is not available";
        return false;
    }

    current_device_id_ = device_id;
    device_set_ = true;

    return true;
}

// ============================================================================
// Stream Control
// ============================================================================

bool AudioEngine::start() {
    if (is_running_) {
        return true; // Already running
    }

    if (!audio_) {
        error_message_ = "RtAudio not initialized";
        return false;
    }

    // Use default device if none set
    if (!device_set_) {
        current_device_id_ = audio_->getDefaultInputDevice();
        device_set_ = true;
    }

    // Set up stream parameters
    RtAudio::StreamParameters input_params;
    input_params.deviceId = current_device_id_;
    input_params.nChannels = 1; // Mono
    input_params.firstChannel = 0;

    unsigned int buffer_frames = static_cast<unsigned int>(buffer_size_);

    // Open stream - throws exception on error in RtAudio 5.x
    try {
        audio_->openStream(
            nullptr,          // No output
            &input_params,    // Input parameters
            RTAUDIO_FLOAT32,  // Sample format
            static_cast<unsigned int>(sample_rate_),
            &buffer_frames,
            &AudioEngine::audioCallback,
            this              // User data
        );
    } catch (RtAudioError& e) {
        error_message_ = "Failed to open stream: " + std::string(e.what());
        std::cerr << error_message_ << std::endl;
        return false;
    }

    // Start stream - throws exception on error in RtAudio 5.x
    try {
        audio_->startStream();
    } catch (RtAudioError& e) {
        error_message_ = "Failed to start stream: " + std::string(e.what());
        std::cerr << error_message_ << std::endl;
        audio_->closeStream();
        return false;
    }

    is_running_ = true;
    error_message_.clear();

    std::cout << "Audio stream started on device " << current_device_id_ << std::endl;
    return true;
}

void AudioEngine::stop() {
    if (!is_running_) {
        return; // Already stopped
    }

    if (audio_ && audio_->isStreamOpen()) {
        if (audio_->isStreamRunning()) {
            audio_->stopStream();
        }
        audio_->closeStream();
    }

    is_running_ = false;
    std::cout << "Audio stream stopped" << std::endl;
}

bool AudioEngine::isRunning() const {
    return is_running_.load();
}

// ============================================================================
// Audio Callback
// ============================================================================

int AudioEngine::audioCallback(void* output_buffer,
                               void* input_buffer,
                               unsigned int frame_count,
                               double stream_time,
                               RtAudioStreamStatus status,
                               void* user_data)
{
    (void)output_buffer;  // Unused
    (void)stream_time;    // Unused

    // Check for stream errors
    if (status) {
        std::cerr << "Audio stream status: ";
        if (status & RTAUDIO_INPUT_OVERFLOW)
            std::cerr << "Input overflow! ";
        if (status & RTAUDIO_OUTPUT_UNDERFLOW)
            std::cerr << "Output underflow! ";
        std::cerr << std::endl;
    }

    // Get engine instance
    AudioEngine* engine = static_cast<AudioEngine*>(user_data);
    if (!engine || !input_buffer) {
        return 1; // Stop stream on error
    }

    // Process audio
    const float* input = static_cast<const float*>(input_buffer);
    engine->processAudioCallback(input, frame_count);

    return 0; // Continue stream
}

void AudioEngine::processAudioCallback(const float* input, unsigned int frame_count) {
    // Write to ring buffer (thread-safe)
    ring_buffer_->write(input, frame_count);

    // Update input level (RMS)
    float rms = calculateRMS(input, frame_count);
    input_level_.store(rms);
}

// ============================================================================
// Utility Functions
// ============================================================================

float AudioEngine::calculateRMS(const float* buffer, size_t count) {
    if (count == 0) return 0.0f;

    float sum_squares = 0.0f;
    for (size_t i = 0; i < count; ++i) {
        sum_squares += buffer[i] * buffer[i];
    }

    return std::sqrt(sum_squares / count);
}

} // namespace friture
