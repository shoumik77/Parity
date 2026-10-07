#include "MasterChain.h"

#include <cmath>

void MasterChain::prepare (double sampleRate, int maxBlockSize, int numChannels)
{
    juce::ignoreUnused (maxBlockSize, numChannels);

    currentSampleRate = sampleRate;
    gain.reset (sampleRate, 0.02);
    clipThreshold.reset (sampleRate, 0.02);
    ceiling.reset (sampleRate, 0.02);
    gain.setCurrentAndTargetValue (1.0f);
    clipThreshold.setCurrentAndTargetValue (bypassedClipThreshold);
    ceiling.setCurrentAndTargetValue (1.0f);
    reset();
}

void MasterChain::reset()
{
    gain.setCurrentAndTargetValue (gain.getTargetValue());
    clipThreshold.setCurrentAndTargetValue (clipThreshold.getTargetValue());
    ceiling.setCurrentAndTargetValue (ceiling.getTargetValue());
    envelope = 0.0f;
}

void MasterChain::setParameters (bool clipEnabled, float clipThresholdDb,
                                 bool limitEnabled, float gainDb,
                                 float ceilingDb, float releaseMs) noexcept
{
    // Disabled sections ramp to transparent settings instead of hard-switching,
    // so toggles never click.
    gain.setTargetValue (juce::Decibels::decibelsToGain (limitEnabled ? gainDb : 0.0f));
    clipThreshold.setTargetValue (clipEnabled ? juce::Decibels::decibelsToGain (clipThresholdDb)
                                              : bypassedClipThreshold);
    ceiling.setTargetValue (juce::Decibels::decibelsToGain (ceilingDb));
    limitOn = limitEnabled;
    releaseCoeff = std::exp (-1.0f / (float) (juce::jmax (1.0f, releaseMs) * 0.001 * currentSampleRate));
}

void MasterChain::process (juce::AudioBuffer<float>& buffer) noexcept
{
    const auto numSamples = buffer.getNumSamples();
    const auto numChannels = buffer.getNumChannels();

    for (int i = 0; i < numSamples; ++i)
    {
        const auto g = gain.getNextValue();
        const auto threshold = clipThreshold.getNextValue();
        const auto ceilingNow = ceiling.getNextValue();

        float framePeak = 0.0f;

        for (int ch = 0; ch < numChannels; ++ch)
        {
            const auto sample = juce::jlimit (-threshold, threshold,
                                              buffer.getSample (ch, i) * g);
            buffer.setSample (ch, i, sample);
            framePeak = juce::jmax (framePeak, std::abs (sample));
        }

        if (! limitOn)
            continue;

        // Channel-linked brickwall: instant attack, exponential release.
        envelope = framePeak > envelope ? framePeak
                                        : framePeak + releaseCoeff * (envelope - framePeak);

        if (envelope > ceilingNow)
        {
            const auto reduction = ceilingNow / envelope;

            for (int ch = 0; ch < numChannels; ++ch)
                buffer.setSample (ch, i, buffer.getSample (ch, i) * reduction);
        }
    }
}
