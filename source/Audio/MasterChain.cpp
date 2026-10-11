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
    meterDecayCoeff = std::exp (-1.0f / (float) (0.3 * sampleRate));
    slotLengthSamples = juce::jmax (1, (int) std::round (0.01 * sampleRate));
    reset();
}

void MasterChain::reset()
{
    gain.setCurrentAndTargetValue (gain.getTargetValue());
    clipThreshold.setCurrentAndTargetValue (clipThreshold.getTargetValue());
    ceiling.setCurrentAndTargetValue (ceiling.getTargetValue());
    envelope = 0.0f;
    clipInputLevel.store (0.0f);
    postClipLevel.store (0.0f);
    postLimitLevel.store (0.0f);
    clipReductionDb.store (0.0f);
    limitReductionDb.store (0.0f);
    clipHoldCounter = limitHoldCounter = 0;
    history.clear();
    pendingSlot = {};
    samplesIntoSlot = 0;
    clippedInSlot = 0;
    recentClipAverage = 0.0f;
    recentClippedFraction.store (0.0f);
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
    clipOn = clipEnabled;
    limitOn = limitEnabled;
    releaseCoeff = std::exp (-1.0f / (float) (juce::jmax (1.0f, releaseMs) * 0.001 * currentSampleRate));
}

void MasterChain::process (juce::AudioBuffer<float>& buffer) noexcept
{
    const auto numSamples = buffer.getNumSamples();
    const auto numChannels = buffer.getNumChannels();

    auto inputMeter = clipInputLevel.load();
    auto clipMeter = postClipLevel.load();
    auto limitMeter = postLimitLevel.load();
    auto clipGr = clipReductionDb.load();
    auto limitGr = limitReductionDb.load();

    // Gain-reduction readouts: hold the deepest reduction for ~600 ms, then
    // return towards 0 at 20 dB/s so the figure stays readable between hits.
    const auto decayDb = 20.0f / (float) currentSampleRate;
    const auto holdSamples = (int) (0.6 * currentSampleRate);

    auto holdReduction = [decayDb, holdSamples] (float current, float instantaneous, int& sinceDeepest)
    {
        if (instantaneous <= current)
        {
            sinceDeepest = 0;
            return instantaneous;
        }

        return ++sinceDeepest > holdSamples ? juce::jmin (instantaneous, current + decayDb) : current;
    };

    for (int i = 0; i < numSamples; ++i)
    {
        const auto g = gain.getNextValue();
        const auto threshold = clipThreshold.getNextValue();
        const auto ceilingNow = ceiling.getNextValue();

        float inputPeak = 0.0f, framePeak = 0.0f;
        bool clippedThisFrame = false;

        for (int ch = 0; ch < numChannels; ++ch)
        {
            const auto driven = buffer.getSample (ch, i) * g;
            const auto sample = juce::jlimit (-threshold, threshold, driven);
            buffer.setSample (ch, i, sample);

            const auto magnitude = std::abs (driven);
            inputPeak = juce::jmax (inputPeak, magnitude);
            framePeak = juce::jmax (framePeak, std::abs (sample));
            clippedThisFrame = clippedThisFrame || magnitude > threshold;
        }

        inputMeter = juce::jmax (inputPeak, inputMeter * meterDecayCoeff);
        clipMeter = juce::jmax (framePeak, clipMeter * meterDecayCoeff);

        const auto clipReductionNow = clippedThisFrame ? juce::Decibels::gainToDecibels (threshold / inputPeak, -60.0f)
                                                       : 0.0f;
        clipGr = holdReduction (clipGr, clipReductionNow, clipHoldCounter);

        auto limitedPeak = framePeak;
        float limitReductionNow = 0.0f;

        if (limitOn)
        {
            // Channel-linked brickwall: instant attack, exponential release.
            envelope = framePeak > envelope ? framePeak
                                            : framePeak + releaseCoeff * (envelope - framePeak);

            if (envelope > ceilingNow)
            {
                const auto reduction = ceilingNow / envelope;
                limitedPeak = framePeak * reduction;
                limitReductionNow = juce::Decibels::gainToDecibels (reduction, -60.0f);

                for (int ch = 0; ch < numChannels; ++ch)
                    buffer.setSample (ch, i, buffer.getSample (ch, i) * reduction);
            }
        }

        limitMeter = juce::jmax (limitedPeak, limitMeter * meterDecayCoeff);
        limitGr = holdReduction (limitGr, limitReductionNow, limitHoldCounter);

        // Accumulate the history slot.
        pendingSlot.inputPeak = juce::jmax (pendingSlot.inputPeak, inputPeak);
        pendingSlot.postClipPeak = juce::jmax (pendingSlot.postClipPeak, framePeak);
        pendingSlot.outputPeak = juce::jmax (pendingSlot.outputPeak, limitedPeak);
        clippedInSlot += clippedThisFrame ? 1 : 0;

        if (++samplesIntoSlot >= slotLengthSamples)
        {
            pendingSlot.clipThreshold = threshold;
            pendingSlot.ceiling = ceilingNow;
            pendingSlot.clippedFraction = (float) clippedInSlot / (float) slotLengthSamples;
            pendingSlot.clipActive = clipOn;
            pendingSlot.limitActive = limitOn;
            history.push (pendingSlot);

            // ~3 s running average of the clipped share for the readout.
            recentClipAverage += (pendingSlot.clippedFraction - recentClipAverage) * (1.0f / 300.0f);
            recentClippedFraction.store (recentClipAverage);

            pendingSlot = {};
            samplesIntoSlot = 0;
            clippedInSlot = 0;
        }
    }

    clipInputLevel.store (inputMeter);
    postClipLevel.store (clipMeter);
    postLimitLevel.store (limitMeter);
    clipReductionDb.store (clipGr);
    limitReductionDb.store (limitGr);
}
