#pragma once

#include <juce_audio_basics/juce_audio_basics.h>

//==============================================================================
/**
    Master processing for the mix path: smoothed input gain, hard clipper,
    and a brickwall limiter (ceiling + release). Zero latency - the limiter
    uses an instant-attack envelope follower, no lookahead, so no delay
    compensation is needed.

    (juce::dsp::Limiter is deliberately not used: it applies makeup gain of
    -threshold and clips at 0 dBFS, acting as a maximizer rather than
    honouring a true output ceiling.)

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
    juce::SmoothedValue<float> ceiling { 1.0f };
    bool limitOn = false;

    // Brickwall limiter state: channel-linked envelope follower,
    // instant attack, exponential release.
    double currentSampleRate = 44100.0;
    float releaseCoeff = 0.0f;
    float envelope = 0.0f;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MasterChain)
};
