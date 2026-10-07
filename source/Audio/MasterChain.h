#pragma once

#include <juce_dsp/juce_dsp.h>

//==============================================================================
/**
    Master processing for the mix path: smoothed input gain, hard clipper,
    and a brickwall limiter (ceiling + release). Zero latency - the limiter
    has no lookahead, so no delay compensation is needed.

    setParameters() may be called from the audio thread each block;
    process() is allocation-free.
*/
class MasterChain
{
public:
    MasterChain() = default;

    void prepare (double sampleRate, int maxBlockSize, int numChannels);
    void reset();

    void setParameters (bool clipEnabled, float clipThresholdDb,
                        bool limitEnabled, float gainDb,
                        float ceilingDb, float releaseMs) noexcept;

    void process (juce::AudioBuffer<float>& buffer) noexcept;

private:
    // Clip threshold ramps up here when the clipper is bypassed: high enough
    // to be transparent, low enough that the smoothing ramp stays short.
    static constexpr float bypassedClipThreshold = 8.0f; // +18 dB

    juce::SmoothedValue<float> gain { 1.0f };
    juce::SmoothedValue<float> clipThreshold { bypassedClipThreshold };
    bool limitOn = false;

    juce::dsp::Limiter<float> limiter;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MasterChain)
};
