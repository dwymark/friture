/**
 * @file audio_engine.hpp
 * @brief Real-time audio input engine using RtAudio
 *
 * Manages audio device enumeration, stream management, and real-time
 * audio capture into a ring buffer for spectrogram processing.
 *
 * @author Friture C++ Port
 * @date 2025-11-07
 */

#ifndef FRITURE_AUDIO_ENGINE_HPP
#define FRITURE_AUDIO_ENGINE_HPP

#include <friture/ringbuffer.hpp>
#include <friture/audio/audio_device_info.hpp>
#include <RtAudio.h>
#include <memory>
#include <vector>
#include <atomic>
#include <string>

namespace friture {

/**
 * @brief Real-time audio input engine
 *
 * Wraps RtAudio for cross-platform audio input with automatic device
 * enumeration, format conversion, and ring buffer integration.
 *
 * Features:
 * - Thread-safe audio callback
 * - Automatic device enumeration
 * - Input level monitoring (RMS)
 * - Graceful error handling
 *
 * Example usage:
 * @code
 * AudioEngine engine(48000, 512);
 * auto devices = engine.getInputDevices();
 * engine.setInputDevice(devices[0].id);
 * engine.start();
 * // ... process audio from engine.getRingBuffer()
 * engine.stop();
 * @endcode
 */
class AudioEngine {
public:
    /**
     * @brief Construct audio engine with parameters
     * @param sample_rate Desired sample rate (Hz)
     * @param buffer_size Audio buffer size in frames
     * @param ring_buffer_seconds Size of ring buffer in seconds (default 60s)
     * @throws std::runtime_error if RtAudio initialization fails
     */
    AudioEngine(size_t sample_rate = 48000,
                size_t buffer_size = 512,
                size_t ring_buffer_seconds = 60);

    /**
     * @brief Destructor - stops audio stream if running
     */
    ~AudioEngine();


    /**
     * @brief Get list of available input devices
     * @return Vector of device info structures
     *
     * Returns all devices with at least one input channel.
     * The default device (if any) has is_default=true.
     */
    std::vector<AudioDeviceInfo> getInputDevices() const;

    /**
     * @brief Set the input device to use
     * @param device_id Device ID from AudioDeviceInfo
     * @return true if successful, false on error
     *
     * Must be called before start(). Calling this while stream is
     * active will stop the current stream first.
     */
    bool setInputDevice(unsigned int device_id);

    /**
     * @brief Start the audio input stream
     * @return true if successful, false on error
     *
     * Opens audio device and begins capturing to ring buffer.
     * Safe to call multiple times (idempotent).
     */
    bool start();

    /**
     * @brief Stop the audio input stream
     *
     * Safe to call multiple times (idempotent).
     */
    void stop();

    /**
     * @brief Check if stream is currently running
     * @return true if capturing audio
     */
    bool isRunning() const;

    /**
     * @brief Get access to the ring buffer
     * @return Reference to ring buffer
     *
     * Thread-safe for reading while audio callback is writing.
     */
    RingBuffer<float>& getRingBuffer() { return *ring_buffer_; }
    const RingBuffer<float>& getRingBuffer() const { return *ring_buffer_; }

    /**
     * @brief Get current input level (RMS)
     * @return RMS level in range [0.0, 1.0]
     *
     * Updated in real-time by audio callback.
     * Useful for input level meters in UI.
     */
    float getInputLevel() const { return input_level_.load(); }

    /**
     * @brief Get last error message
     * @return Error string, empty if no error
     */
    std::string getError() const { return error_message_; }

    /**
     * @brief Get current sample rate
     * @return Sample rate in Hz
     */
    size_t getSampleRate() const { return sample_rate_; }

    /**
     * @brief Get current buffer size
     * @return Buffer size in frames
     */
    size_t getBufferSize() const { return buffer_size_; }

private:
    /**
     * @brief Audio callback (called from audio thread)
     * @param output_buffer Output buffer (unused, null)
     * @param input_buffer Input buffer with audio samples
     * @param frame_count Number of frames in buffer
     * @param stream_time Current stream time
     * @param status Stream status flags
     * @param user_data Pointer to AudioEngine instance
     * @return 0 to continue, non-zero to stop
     */
    static int audioCallback(void* output_buffer,
                            void* input_buffer,
                            unsigned int frame_count,
                            double stream_time,
                            unsigned int status,
                            void* user_data);

    /**
     * @brief Process audio samples in callback
     * @param input Interleaved audio samples
     * @param frame_count Number of frames
     */
    void processAudioCallback(const float* input, unsigned int frame_count);

    /**
     * @brief Calculate RMS level of audio buffer
     * @param buffer Audio samples
     * @param count Number of samples
     * @return RMS level [0.0, 1.0]
     */
    static float calculateRMS(const float* buffer, size_t count);

    // RtAudio instance
    std::unique_ptr<RtAudio> audio_;

    // Configuration
    size_t sample_rate_;
    size_t buffer_size_;
    unsigned int current_device_id_;
    bool device_set_;

    // Audio buffer
    std::unique_ptr<RingBuffer<float>> ring_buffer_;

    // State
    std::atomic<bool> is_running_;
    std::atomic<float> input_level_;
    std::string error_message_;

    // Prevent copying
    AudioEngine(const AudioEngine&) = delete;
    AudioEngine& operator=(const AudioEngine&) = delete;
};

} // namespace friture

#endif // FRITURE_AUDIO_ENGINE_HPP
