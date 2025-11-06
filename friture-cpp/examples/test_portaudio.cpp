/**
 * PortAudio Test - Enumerate devices and capture audio
 */
#include <portaudio.h>
#include <iostream>
#include <cmath>
#include <vector>
#include <chrono>
#include <thread>

static int audioCallback(const void *inputBuffer, void *outputBuffer,
                        unsigned long framesPerBuffer,
                        const PaStreamCallbackTimeInfo* timeInfo,
                        PaStreamCallbackFlags statusFlags,
                        void *userData) {
    const float *in = static_cast<const float*>(inputBuffer);
    auto *samples = static_cast<std::vector<float>*>(userData);

    // Calculate RMS level
    float sum = 0.0f;
    for (unsigned long i = 0; i < framesPerBuffer; ++i) {
        sum += in[i] * in[i];
    }
    float rms = std::sqrt(sum / framesPerBuffer);

    samples->push_back(rms);

    return paContinue;
}

int main() {
    std::cout << "=== PortAudio Test ===" << std::endl;

    PaError err = Pa_Initialize();
    if (err != paNoError) {
        std::cerr << "Failed to initialize PortAudio: " << Pa_GetErrorText(err) << std::endl;
        return 1;
    }

    std::cout << "PortAudio version: " << Pa_GetVersion() << std::endl;
    std::cout << "PortAudio version text: " << Pa_GetVersionText() << std::endl;

    // List devices
    int numDevices = Pa_GetDeviceCount();
    std::cout << "\nNumber of devices: " << numDevices << std::endl;

    for (int i = 0; i < numDevices; ++i) {
        const PaDeviceInfo *deviceInfo = Pa_GetDeviceInfo(i);
        std::cout << "\nDevice " << i << ": " << deviceInfo->name << std::endl;
        std::cout << "  Max input channels: " << deviceInfo->maxInputChannels << std::endl;
        std::cout << "  Max output channels: " << deviceInfo->maxOutputChannels << std::endl;
        std::cout << "  Default sample rate: " << deviceInfo->defaultSampleRate << " Hz" << std::endl;
    }

    // Try to open default input device
    int defaultInput = Pa_GetDefaultInputDevice();
    if (defaultInput >= 0) {
        std::cout << "\n=== Testing Audio Capture ===" << std::endl;
        std::cout << "Default input device: " << defaultInput << std::endl;

        PaStreamParameters inputParams;
        inputParams.device = defaultInput;
        inputParams.channelCount = 1;
        inputParams.sampleFormat = paFloat32;
        inputParams.suggestedLatency = Pa_GetDeviceInfo(defaultInput)->defaultLowInputLatency;
        inputParams.hostApiSpecificStreamInfo = nullptr;

        PaStream *stream;
        std::vector<float> rmsLevels;

        err = Pa_OpenStream(&stream, &inputParams, nullptr, 48000, 512,
                           paClipOff, audioCallback, &rmsLevels);

        if (err == paNoError) {
            std::cout << "Stream opened successfully. Capturing for 2 seconds..." << std::endl;

            Pa_StartStream(stream);
            std::this_thread::sleep_for(std::chrono::seconds(2));
            Pa_StopStream(stream);
            Pa_CloseStream(stream);

            std::cout << "Captured " << rmsLevels.size() << " audio buffers" << std::endl;

            // Calculate average RMS
            if (!rmsLevels.empty()) {
                float avgRms = 0.0f;
                for (float rms : rmsLevels) {
                    avgRms += rms;
                }
                avgRms /= rmsLevels.size();
                std::cout << "Average RMS level: " << avgRms << std::endl;
                std::cout << "Average dB: " << 20.0f * std::log10(avgRms + 1e-10f) << " dB" << std::endl;
            }

            std::cout << "\n✓ PortAudio test PASSED" << std::endl;
        } else {
            std::cerr << "Failed to open stream: " << Pa_GetErrorText(err) << std::endl;
            std::cout << "\n⚠ PortAudio enumeration works, but no audio input available" << std::endl;
        }
    } else {
        std::cout << "\n⚠ No default input device available (expected in headless environment)" << std::endl;
    }

    Pa_Terminate();
    return 0;
}
