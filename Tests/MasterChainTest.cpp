// Temporary check that MasterChain audibly alters the signal.

#include "../source/Audio/MasterChain.h"

#include <cmath>
#include <iostream>

int main()
{
    constexpr double sampleRate = 48000.0;
    constexpr int numSamples = 48000;

    auto makeSine = [&]
    {
        juce::AudioBuffer<float> buffer (2, numSamples);

        for (int i = 0; i < numSamples; ++i)
        {
            const auto s = (float) std::sin (2.0 * juce::MathConstants<double>::pi * 100.0 * i / sampleRate);
            buffer.setSample (0, i, s);
            buffer.setSample (1, i, s);
        }

        return buffer;
    };

    auto peakDb = [] (const juce::AudioBuffer<float>& b)
    {
        return juce::Decibels::gainToDecibels (b.getMagnitude (0, b.getNumSamples() / 2, b.getNumSamples() / 2));
    };

    bool pass = true;

    {
        // Clip at -6 dB: full-scale sine should flat-top at ~-6 dB.
        MasterChain chain;
        chain.prepare (sampleRate, 512, 2);
        chain.setParameters (true, -6.0f, false, 0.0f, -1.0f, 100.0f);
        auto buffer = makeSine();
        chain.process (buffer);
        const auto p = peakDb (buffer);
        std::cout << "Clip -6 dB: peak " << p << " dB (expected ~ -6)\n";
        pass = pass && std::abs (p - (-6.0f)) < 0.2f;
    }

    {
        // Limiter: +6 dB gain into -3 dB ceiling, peak should hold near -3 dB.
        MasterChain chain;
        chain.prepare (sampleRate, 512, 2);
        chain.setParameters (false, 0.0f, true, 6.0f, -3.0f, 100.0f);
        auto buffer = makeSine();
        chain.process (buffer);
        const auto p = peakDb (buffer);
        std::cout << "Limit -3 dB ceiling: peak " << p << " dB (expected ~ -3)\n";
        pass = pass && p < -2.0f && p > -5.0f;
    }

    {
        // Everything bypassed: output identical to input.
        MasterChain chain;
        chain.prepare (sampleRate, 512, 2);
        chain.setParameters (false, -6.0f, false, 6.0f, -3.0f, 100.0f);
        auto buffer = makeSine();
        chain.process (buffer);
        const auto p = peakDb (buffer);
        std::cout << "Bypassed: peak " << p << " dB (expected ~ 0)\n";
        pass = pass && std::abs (p) < 0.1f;
    }

    std::cout << (pass ? "PASS\n" : "FAIL\n");
    return pass ? 0 : 1;
}
