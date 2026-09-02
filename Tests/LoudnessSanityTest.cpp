// Temporary sanity check for LoudnessAnalyzer.
// EBU Tech 3341 test case: a 997 Hz sine at -18 dBFS in both stereo channels
// should measure -18.0 LUFS (the +3 dB channel sum cancels the sine's -3 dB
// RMS, and the -0.691 offset cancels the K-shelf gain at 997 Hz).
//
// True-peak case: a full-scale sine at fs/4 with a 45 degree phase offset only
// ever hits +-0.7071 on the sample grid (sample peak -3 dB), but its true
// inter-sample peak is 1.0. The BS.1770 4x interpolator must recover ~0 dBTP.

#include "../source/Audio/LoudnessAnalyzer.h"

#include <cmath>
#include <iostream>

int main()
{
    constexpr double sampleRate = 48000.0;

    bool pass = true;

    {
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
                  << "True peak:  " << stats.truePeakDb << " dBTP (expected ~ -18.0)\n";

        pass = pass && std::abs (stats.integratedLufs - (-18.0f)) < 0.5f
                    && std::abs (stats.truePeakDb - (-18.0f)) < 0.2f;
    }

    {
        constexpr int numSamples = (int) sampleRate; // 1 second
        juce::AudioBuffer<float> buffer (2, numSamples);

        for (int i = 0; i < numSamples; ++i)
        {
            // fs/4 sine, 45 degree phase: samples alternate +-0.7071, true peak 1.0.
            const auto sample = (float) std::sin (juce::MathConstants<double>::halfPi * i
                                                  + juce::MathConstants<double>::pi / 4.0);
            buffer.setSample (0, i, sample);
            buffer.setSample (1, i, sample);
        }

        const auto stats = LoudnessAnalyzer::analyzeBuffer (buffer, sampleRate);

        std::cout << "Inter-sample true peak: " << stats.truePeakDb
                  << " dBTP (expected ~ 0.0, sample peak is -3.0)\n";

        pass = pass && std::abs (stats.truePeakDb) < 0.3f;
    }

    std::cout << (pass ? "PASS\n" : "FAIL\n");
    return pass ? 0 : 1;
}
