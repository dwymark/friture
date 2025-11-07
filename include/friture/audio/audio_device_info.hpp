/**
 * @file audio_device_info.hpp
 * @brief Audio device information structure
 *
 * Simple structure to hold audio device metadata for device enumeration
 * and selection in the UI.
 *
 * @author Friture C++ Port
 * @date 2025-11-07
 */

#ifndef FRITURE_AUDIO_DEVICE_INFO_HPP
#define FRITURE_AUDIO_DEVICE_INFO_HPP

#include <string>
#include <cstddef>

namespace friture {

/**
 * @brief Information about an audio input device
 *
 * Contains metadata for displaying and selecting audio devices.
 */
struct AudioDeviceInfo {
    unsigned int id;              ///< Device ID (for RtAudio)
    std::string name;             ///< Human-readable device name
    unsigned int input_channels;  ///< Number of input channels
    unsigned int output_channels; ///< Number of output channels
    unsigned int sample_rates;    ///< Bitmask of supported sample rates
    bool is_default;              ///< True if this is the default device

    /**
     * @brief Check if device supports a given sample rate
     * @param rate Sample rate in Hz
     * @return true if supported
     */
    bool supportsSampleRate(unsigned int rate) const {
        // Common sample rates
        switch (rate) {
            case 44100: return (sample_rates & (1 << 0)) != 0;
            case 48000: return (sample_rates & (1 << 1)) != 0;
            case 96000: return (sample_rates & (1 << 2)) != 0;
            default: return false;
        }
    }

    /**
     * @brief Get a formatted string description
     * @return String like "Device Name (2 ch, 48kHz)"
     */
    std::string getDescription() const {
        std::string desc = name;
        if (input_channels > 0) {
            desc += " (" + std::to_string(input_channels) + " ch";
            if (is_default) desc += ", default";
            desc += ")";
        }
        return desc;
    }
};

} // namespace friture

#endif // FRITURE_AUDIO_DEVICE_INFO_HPP
