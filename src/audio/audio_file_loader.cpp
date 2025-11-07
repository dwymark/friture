/**
 * @file audio_file_loader.cpp
 * @brief Implementation of WAV file loader
 */

#include <friture/audio/audio_file_loader.hpp>
#include <cstdio>
#include <cstring>
#include <algorithm>
#include <sstream>
#include <iostream>

namespace friture {

// WAV format constants
constexpr uint16_t WAVE_FORMAT_PCM = 0x0001;
constexpr uint16_t WAVE_FORMAT_IEEE_FLOAT = 0x0003;
constexpr uint16_t WAVE_FORMAT_EXTENSIBLE = 0xFFFE;

// ============================================================================
// WavInfo
// ============================================================================

std::string WavInfo::getFormatDescription() const {
    std::ostringstream oss;

    // Format type
    if (audio_format == WAVE_FORMAT_PCM) {
        oss << "PCM " << bits_per_sample << "-bit";
    } else if (audio_format == WAVE_FORMAT_IEEE_FLOAT) {
        oss << "IEEE Float " << bits_per_sample << "-bit";
    } else {
        oss << "Unknown format " << audio_format;
    }

    // Channels
    oss << ", ";
    if (channels == 1) {
        oss << "Mono";
    } else if (channels == 2) {
        oss << "Stereo";
    } else {
        oss << channels << " channels";
    }

    // Sample rate
    oss << ", " << sample_rate << " Hz";

    // Duration
    oss << ", " << std::fixed << duration_sec << " sec";

    return oss.str();
}

// ============================================================================
// AudioFileLoader
// ============================================================================

AudioFileLoader::AudioFileLoader()
    : info_(), error_() {
}

AudioFileLoader::~AudioFileLoader() {
}

bool AudioFileLoader::load(const char* filename, std::vector<float>& samples, float& sample_rate) {
    // Clear previous state
    error_.clear();
    info_ = WavInfo();
    samples.clear();

    // Open file
    FILE* fp = fopen(filename, "rb");
    if (!fp) {
        setError(std::string("Failed to open file: ") + filename);
        return false;
    }

    // Parse WAV header
    size_t data_offset = 0;
    size_t data_size = 0;
    if (!parseWavHeader(fp, data_offset, data_size)) {
        fclose(fp);
        return false;
    }

    // Calculate number of samples
    size_t bytes_per_sample = info_.bits_per_sample / 8;
    size_t total_samples = data_size / bytes_per_sample;
    size_t num_frames = total_samples / info_.channels;

    info_.num_samples = static_cast<uint32_t>(num_frames);
    info_.duration_sec = static_cast<float>(num_frames) / info_.sample_rate;

    std::cout << "WAV file info: " << info_.getFormatDescription() << std::endl;

    // Seek to data
    if (fseek(fp, static_cast<long>(data_offset), SEEK_SET) != 0) {
        setError("Failed to seek to audio data");
        fclose(fp);
        return false;
    }

    // Read audio data based on format
    std::vector<uint8_t> raw_data(data_size);
    size_t bytes_read = fread(raw_data.data(), 1, data_size, fp);
    fclose(fp);

    if (bytes_read != data_size) {
        setError("Failed to read complete audio data");
        return false;
    }

    // Convert to float based on format
    std::vector<float> temp_samples;

    if (info_.audio_format == WAVE_FORMAT_PCM) {
        temp_samples.resize(total_samples);

        if (info_.bits_per_sample == 16) {
            convertPCM16ToFloat(
                reinterpret_cast<const int16_t*>(raw_data.data()),
                temp_samples.data(),
                total_samples
            );
        } else if (info_.bits_per_sample == 24) {
            convertPCM24ToFloat(
                raw_data.data(),
                temp_samples.data(),
                total_samples
            );
        } else if (info_.bits_per_sample == 32) {
            convertPCM32ToFloat(
                reinterpret_cast<const int32_t*>(raw_data.data()),
                temp_samples.data(),
                total_samples
            );
        } else {
            setError("Unsupported PCM bit depth: " + std::to_string(info_.bits_per_sample));
            return false;
        }
    } else if (info_.audio_format == WAVE_FORMAT_IEEE_FLOAT) {
        if (info_.bits_per_sample == 32) {
            // Already float32, just copy
            temp_samples.resize(total_samples);
            memcpy(temp_samples.data(), raw_data.data(), data_size);
        } else {
            setError("Unsupported float bit depth: " + std::to_string(info_.bits_per_sample));
            return false;
        }
    } else {
        setError("Unsupported audio format: " + std::to_string(info_.audio_format));
        return false;
    }

    // Convert stereo to mono if needed
    if (info_.channels == 1) {
        samples = std::move(temp_samples);
    } else if (info_.channels == 2) {
        samples.resize(num_frames);
        stereoToMono(temp_samples.data(), samples.data(), num_frames);
    } else {
        setError("Unsupported channel count: " + std::to_string(info_.channels));
        return false;
    }

    sample_rate = static_cast<float>(info_.sample_rate);
    std::cout << "Loaded " << samples.size() << " mono samples" << std::endl;

    return true;
}

bool AudioFileLoader::parseWavHeader(FILE* fp, size_t& data_offset, size_t& data_size) {
    // Read RIFF header
    char riff_id[4];
    if (fread(riff_id, 1, 4, fp) != 4 || memcmp(riff_id, "RIFF", 4) != 0) {
        setError("Not a valid RIFF file");
        return false;
    }

    uint32_t file_size;
    if (fread(&file_size, 4, 1, fp) != 1) {
        setError("Failed to read file size");
        return false;
    }

    char wave_id[4];
    if (fread(wave_id, 1, 4, fp) != 4 || memcmp(wave_id, "WAVE", 4) != 0) {
        setError("Not a valid WAVE file");
        return false;
    }

    // Read chunks until we find 'fmt ' and 'data'
    bool found_fmt = false;
    bool found_data = false;

    while (!feof(fp) && !(found_fmt && found_data)) {
        char chunk_id[4];
        uint32_t chunk_size;

        if (!readChunkHeader(fp, chunk_id, chunk_size)) {
            // End of file or read error
            break;
        }

        if (memcmp(chunk_id, "fmt ", 4) == 0) {
            if (!parseFmtChunk(fp, chunk_size)) {
                return false;
            }
            found_fmt = true;
        } else if (memcmp(chunk_id, "data", 4) == 0) {
            data_offset = ftell(fp);
            data_size = chunk_size;
            found_data = true;
            // Don't read the data yet, just skip it
            fseek(fp, chunk_size, SEEK_CUR);
        } else {
            // Skip unknown chunk
            std::cout << "Skipping chunk: "
                      << chunk_id[0] << chunk_id[1] << chunk_id[2] << chunk_id[3]
                      << " (size: " << chunk_size << ")" << std::endl;
            fseek(fp, chunk_size, SEEK_CUR);
        }

        // WAV chunks are word-aligned
        if (chunk_size % 2 == 1) {
            fseek(fp, 1, SEEK_CUR);
        }
    }

    if (!found_fmt) {
        setError("Missing 'fmt ' chunk");
        return false;
    }

    if (!found_data) {
        setError("Missing 'data' chunk");
        return false;
    }

    return true;
}

bool AudioFileLoader::readChunkHeader(FILE* fp, char chunk_id[4], uint32_t& chunk_size) {
    if (fread(chunk_id, 1, 4, fp) != 4) {
        return false;
    }

    if (fread(&chunk_size, 4, 1, fp) != 1) {
        return false;
    }

    return true;
}

bool AudioFileLoader::parseFmtChunk(FILE* fp, uint32_t chunk_size) {
    if (chunk_size < 16) {
        setError("Invalid 'fmt ' chunk size");
        return false;
    }

    uint16_t audio_format;
    uint16_t num_channels;
    uint32_t sample_rate;
    uint32_t byte_rate;
    uint16_t block_align;
    uint16_t bits_per_sample;

    if (fread(&audio_format, 2, 1, fp) != 1 ||
        fread(&num_channels, 2, 1, fp) != 1 ||
        fread(&sample_rate, 4, 1, fp) != 1 ||
        fread(&byte_rate, 4, 1, fp) != 1 ||
        fread(&block_align, 2, 1, fp) != 1 ||
        fread(&bits_per_sample, 2, 1, fp) != 1) {
        setError("Failed to read 'fmt ' chunk data");
        return false;
    }

    // Skip any extra fmt data
    if (chunk_size > 16) {
        fseek(fp, chunk_size - 16, SEEK_CUR);
    }

    // Validate format
    if (audio_format != WAVE_FORMAT_PCM && audio_format != WAVE_FORMAT_IEEE_FLOAT) {
        setError("Unsupported audio format (only PCM and IEEE float supported)");
        return false;
    }

    if (num_channels != 1 && num_channels != 2) {
        setError("Unsupported channel count (only mono and stereo supported)");
        return false;
    }

    if (bits_per_sample != 16 && bits_per_sample != 24 && bits_per_sample != 32) {
        setError("Unsupported bit depth (only 16, 24, 32 supported)");
        return false;
    }

    // Store format info
    info_.audio_format = audio_format;
    info_.channels = num_channels;
    info_.sample_rate = sample_rate;
    info_.bits_per_sample = bits_per_sample;

    return true;
}

void AudioFileLoader::convertPCM16ToFloat(const int16_t* src, float* dst, size_t num_samples) {
    constexpr float scale = 1.0f / 32768.0f;
    for (size_t i = 0; i < num_samples; ++i) {
        dst[i] = static_cast<float>(src[i]) * scale;
    }
}

void AudioFileLoader::convertPCM24ToFloat(const uint8_t* src, float* dst, size_t num_samples) {
    constexpr float scale = 1.0f / 8388608.0f; // 2^23

    for (size_t i = 0; i < num_samples; ++i) {
        // Read 3 bytes as little-endian 24-bit signed integer
        int32_t sample = (static_cast<int32_t>(src[i * 3 + 0])) |
                        (static_cast<int32_t>(src[i * 3 + 1]) << 8) |
                        (static_cast<int32_t>(src[i * 3 + 2]) << 16);

        // Sign extend from 24-bit to 32-bit
        if (sample & 0x800000) {
            sample |= 0xFF000000;
        }

        dst[i] = static_cast<float>(sample) * scale;
    }
}

void AudioFileLoader::convertPCM32ToFloat(const int32_t* src, float* dst, size_t num_samples) {
    constexpr float scale = 1.0f / 2147483648.0f; // 2^31

    for (size_t i = 0; i < num_samples; ++i) {
        dst[i] = static_cast<float>(src[i]) * scale;
    }
}

void AudioFileLoader::stereoToMono(const float* stereo, float* mono, size_t num_frames) {
    for (size_t i = 0; i < num_frames; ++i) {
        mono[i] = (stereo[i * 2] + stereo[i * 2 + 1]) * 0.5f;
    }
}

void AudioFileLoader::setError(const std::string& message) {
    error_ = message;
    std::cerr << "AudioFileLoader error: " << message << std::endl;
}

} // namespace friture
