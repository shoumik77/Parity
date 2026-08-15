// Temporary sanity check for LoudnessAnalyzer.
// EBU Tech 3341 test case: a 997 Hz sine at -18 dBFS in both stereo channels
// should measure -18.0 LUFS (the +3 dB channel sum cancels the sine's -3 dB
// RMS, and the -0.691 offset cancels the K-shelf gain at 997 Hz).

#include "../source/Audio/LoudnessAnalyzer.h"

#include <cmath>
#include <iostream>

int main()
{
    constexpr double sampleRate = 48000.0;
    constexpr double freq = 997.0;
    constexpr float amplitude = 0.12589254f; // -18 dBFS
    constexpr int numSamples = (int) (sampleRate * 10.0); // 10 seconds

    juce::AudioBuffer<float> buffer (2, numSamples);

    for (int i = 0; i < numSamples; ++i)
    {
        const auto sample = amplitude * (float) std::sin (2.0 * juce::MathConstants<double>::pi * freq * i / sampleRate);
        buffer.setSample (0, i, sample);
        buffer.setSample (1, i, sample);
    }

    const auto stats = LoudnessAnalyzer::analyzeBuffer (buffer, sampleRate);

    std::cout << "Integrated: " << stats.integratedLufs << " LUFS (expected ~ -18.0)\n"
              << "Peak:       " << stats.peakDb << " dB (expected ~ -18.0)\n";

    const bool pass = std::abs (stats.integratedLufs - (-18.0f)) < 0.5f
                   && std::abs (stats.peakDb - (-18.0f)) < 0.1f;

    std::cout << (pass ? "PASS\n" : "FAIL\n");
    return pass ? 0 : 1;
}
