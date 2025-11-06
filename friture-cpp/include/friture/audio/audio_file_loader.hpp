/**
 * @file audio_file_loader.hpp
 * @brief WAV file loader with broad format support
 *
 * Supports loading WAV files in various formats:
 * - PCM 16-bit, 24-bit, 32-bit
 * - IEEE Float 32-bit
 * - Mono and stereo (stereo is converted to mono by averaging channels)
 * - Various sample rates (stored as-is, resampling not implemented)
 *
 * Implementation handles:
 * - RIFF chunk-based parsing
 * - Non-standard chunk ordering
 * - Metadata chunks (skipped gracefully)
 * - File validation and error reporting
 *
 * Current limitation: Loads entire file into memory.
 * For large files (>100MB), see TODO.md for streaming implementation notes.
 *
 * @author Friture C++ Port
 * @date 2025-11-06
 */

#ifndef FRITURE_AUDIO_FILE_LOADER_HPP
#define FRITURE_AUDIO_FILE_LOADER_HPP

#include <vector>
#include <string>
#include <cstdint>

namespace friture {

/**
 * @brief WAV file metadata information
 */
struct WavInfo {
    uint32_t sample_rate = 0;        ///< Sample rate in Hz
    uint16_t channels = 0;            ///< Number of channels (1=mono, 2=stereo)
    uint16_t bits_per_sample = 0;     ///< Bits per sample (16, 24, 32)
    uint16_t audio_format = 0;        ///< Audio format (1=PCM, 3=IEEE float)
    uint32_t num_samples = 0;         ///< Total samples per channel
    float duration_sec = 0.0f;        ///< Duration in seconds

    /**
     * @brief Check if WAV info is valid
     * @return true if all fields are valid
     */
    bool isValid() const {
        return sample_rate > 0 && channels > 0 &&
               bits_per_sample > 0 && num_samples > 0;
    }

    /**
     * @brief Get human-readable format description
     */
    std::string getFormatDescription() const;
};

/**
 * @brief WAV file loader with broad format support
 *
 * Usage:
 * @code
 * AudioFileLoader loader;
 * std::vector<float> samples;
 * float sample_rate;
 *
 * if (loader.load("audio.wav", samples, sample_rate)) {
 *     std::cout << "Loaded " << samples.size() << " samples at "
 *               << sample_rate << " Hz" << std::endl;
 * }
 * @endcode
 */
class AudioFileLoader {
public:
    /**
     * @brief Construct AudioFileLoader
     */
    AudioFileLoader();

    /**
     * @brief Destructor
     */
    ~AudioFileLoader();

    /**
     * @brief Load entire WAV file into memory
     * @param filename Path to WAV file
     * @param samples Output buffer for audio samples (mono, normalized to [-1, 1])
     * @param sample_rate Output sample rate in Hz
     * @return true on success, false on error
     *
     * If file is stereo, channels are averaged to mono.
     * All formats are converted to float [-1, 1] range.
     */
    bool load(const char* filename, std::vector<float>& samples, float& sample_rate);

    /**
     * @brief Get metadata from last loaded file
     * @return WAV file information
     */
    const WavInfo& getInfo() const { return info_; }

    /**
     * @brief Get last error message
     * @return Error string, or empty if no error
     */
    const std::string& getError() const { return error_; }

private:
    /**
     * @brief Parse WAV file header and chunks
     * @param fp File pointer
     * @param data_offset Output: offset to audio data
     * @param data_size Output: size of audio data in bytes
     * @return true if header is valid
     */
    bool parseWavHeader(FILE* fp, size_t& data_offset, size_t& data_size);

    /**
     * @brief Read RIFF chunk header
     * @param fp File pointer
     * @param chunk_id Output: 4-character chunk ID
     * @param chunk_size Output: chunk size in bytes
     * @return true on success
     */
    bool readChunkHeader(FILE* fp, char chunk_id[4], uint32_t& chunk_size);

    /**
     * @brief Parse 'fmt ' chunk
     * @param fp File pointer
     * @param chunk_size Size of fmt chunk
     * @return true if format is supported
     */
    bool parseFmtChunk(FILE* fp, uint32_t chunk_size);

    /**
     * @brief Convert PCM 16-bit to float [-1, 1]
     * @param src Source buffer (16-bit signed integers)
     * @param dst Destination buffer (float)
     * @param num_samples Number of samples to convert
     */
    void convertPCM16ToFloat(const int16_t* src, float* dst, size_t num_samples);

    /**
     * @brief Convert PCM 24-bit to float [-1, 1]
     * @param src Source buffer (packed 24-bit data)
     * @param dst Destination buffer (float)
     * @param num_samples Number of samples to convert
     */
    void convertPCM24ToFloat(const uint8_t* src, float* dst, size_t num_samples);

    /**
     * @brief Convert PCM 32-bit to float [-1, 1]
     * @param src Source buffer (32-bit signed integers)
     * @param dst Destination buffer (float)
     * @param num_samples Number of samples to convert
     */
    void convertPCM32ToFloat(const int32_t* src, float* dst, size_t num_samples);

    /**
     * @brief Convert stereo to mono by averaging channels
     * @param stereo Input stereo samples (interleaved L,R,L,R,...)
     * @param mono Output mono samples
     * @param num_frames Number of stereo frames
     */
    void stereoToMono(const float* stereo, float* mono, size_t num_frames);

    /**
     * @brief Set error message
     * @param message Error description
     */
    void setError(const std::string& message);

    WavInfo info_;          ///< WAV file metadata
    std::string error_;     ///< Last error message
};

} // namespace friture

#endif // FRITURE_AUDIO_FILE_LOADER_HPP
