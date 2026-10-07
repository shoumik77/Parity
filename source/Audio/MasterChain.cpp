#include "MasterChain.h"

void MasterChain::prepare (double sampleRate, int maxBlockSize, int numChannels)
{
    gain.reset (sampleRate, 0.02);
    clipThreshold.reset (sampleRate, 0.02);
    gain.setCurrentAndTargetValue (1.0f);
    clipThreshold.setCurrentAndTargetValue (bypassedClipThreshold);

    limiter.prepare ({ sampleRate, (juce::uint32) maxBlockSize, (juce::uint32) numChannels });
    reset();
}

void MasterChain::reset()
{
    gain.setCurrentAndTargetValue (gain.getTargetValue());
    clipThreshold.setCurrentAndTargetValue (clipThreshold.getTargetValue());
    limiter.reset();
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
    limitOn = limitEnabled;
    limiter.setThreshold (ceilingDb);
    limiter.setRelease (releaseMs);
}

void MasterChain::process (juce::AudioBuffer<float>& buffer) noexcept
{
    const auto numSamples = buffer.getNumSamples();
    const auto numChannels = buffer.getNumChannels();

    for (int i = 0; i < numSamples; ++i)
    {
        const auto g = gain.getNextValue();
        const auto threshold = clipThreshold.getNextValue();

        for (int ch = 0; ch < numChannels; ++ch)
        {
            const auto sample = buffer.getSample (ch, i) * g;
            buffer.setSample (ch, i, juce::jlimit (-threshold, threshold, sample));
        }
    }

    // The limiter only attenuates above the ceiling, so engaging it is
    // click-free without extra smoothing.
    if (limitOn)
    {
        juce::dsp::AudioBlock<float> block (buffer);
        juce::dsp::ProcessContextReplacing<float> context (block);
        limiter.process (context);
    }
}
