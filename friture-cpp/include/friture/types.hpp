/**
 * @file types.hpp
 * @brief Common type definitions for Friture C++ port
 *
 * This file contains enum class definitions for various spectrogram
 * settings and audio processing parameters.
 *
 * @author Friture C++ Port
 * @date 2025-11-06
 */

#ifndef FRITURE_TYPES_HPP
#define FRITURE_TYPES_HPP

namespace friture {

/**
 * @brief Window function types for FFT processing
 *
 * Window functions are applied to audio samples before FFT to reduce
 * spectral leakage. Different windows offer different tradeoffs between
 * frequency resolution and sidelobe suppression.
 */
enum class WindowFunction {
    Hann,      ///< Hann window (good general purpose, moderate sidelobes)
    Hamming    ///< Hamming window (slightly better sidelobe suppression)
};

/**
 * @brief Frequency scale types for spectrogram display
 *
 * Different frequency scales provide different perceptual characteristics:
 * - Linear: Equal spacing in Hz (good for analysis)
 * - Logarithmic: Equal spacing in log scale (good for music)
 * - Mel: Perceptually linear below 1 kHz (speech processing)
 * - ERB: Equivalent Rectangular Bandwidth (psychoacoustic)
 * - Octave: Musical octave spacing
 */
enum class FrequencyScale {
    Linear,        ///< Linear frequency scale (equal Hz spacing)
    Logarithmic,   ///< Logarithmic frequency scale (equal ratio spacing)
    Mel,           ///< Mel scale (perceptually linear for speech)
    ERB,           ///< Equivalent Rectangular Bandwidth scale
    Octave         ///< Octave scale (musical, log base 2)
};

/**
 * @brief Psychoacoustic weighting types
 *
 * Weighting curves adjust the frequency response to match human
 * hearing sensitivity at different sound pressure levels.
 *
 * Standards: IEC 61672-1:2013
 */
enum class WeightingType {
    None,   ///< No weighting (flat response)
    A,      ///< A-weighting (40 phon contour, most common)
    B,      ///< B-weighting (70 phon contour, rarely used)
    C       ///< C-weighting (100 phon contour, high SPL)
};

/**
 * @brief Convert WindowFunction enum to string
 * @param wf Window function type
 * @return Human-readable string representation
 */
inline const char* toString(WindowFunction wf) {
    switch (wf) {
        case WindowFunction::Hann:    return "Hann";
        case WindowFunction::Hamming: return "Hamming";
        default:                      return "Unknown";
    }
}

/**
 * @brief Convert FrequencyScale enum to string
 * @param fs Frequency scale type
 * @return Human-readable string representation
 */
inline const char* toString(FrequencyScale fs) {
    switch (fs) {
        case FrequencyScale::Linear:       return "Linear";
        case FrequencyScale::Logarithmic:  return "Logarithmic";
        case FrequencyScale::Mel:          return "Mel";
        case FrequencyScale::ERB:          return "ERB";
        case FrequencyScale::Octave:       return "Octave";
        default:                           return "Unknown";
    }
}

/**
 * @brief Convert WeightingType enum to string
 * @param wt Weighting type
 * @return Human-readable string representation
 */
inline const char* toString(WeightingType wt) {
    switch (wt) {
        case WeightingType::None: return "None";
        case WeightingType::A:    return "A-weighting";
        case WeightingType::B:    return "B-weighting";
        case WeightingType::C:    return "C-weighting";
        default:                  return "Unknown";
    }
}

} // namespace friture

#endif // FRITURE_TYPES_HPP
