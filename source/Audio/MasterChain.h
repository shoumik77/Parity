#pragma once

#include <juce_audio_basics/juce_audio_basics.h>

#include <atomic>

#include "HistoryBuffer.h"

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

    //==============================================================================
    /** Peak linear levels (with ~300 ms decay) for the UI meters. */
    float getClipInputLevel() const noexcept   { return clipInputLevel.load(); }   // after gain, before clip
    float getPostClipLevel() const noexcept    { return postClipLevel.load(); }    // limiter input
    float getPostLimitLevel() const noexcept   { return postLimitLevel.load(); }

    /** Current gain reduction in dB (<= 0), peak-held with ~300 ms decay. */
    float getClipReductionDb() const noexcept   { return clipReductionDb.load(); }
    float getLimitReductionDb() const noexcept  { return limitReductionDb.load(); }

    //==============================================================================
    /** One ~10 ms slice of activity for the history displays. Levels are linear peaks. */
    struct Slot
    {
        float inputPeak = 0.0f;      // post-gain, pre-clip
        float clipThreshold = 1.0f;  // linear (large when bypassed)
        float postClipPeak = 0.0f;
        float ceiling = 1.0f;        // linear
        float outputPeak = 0.0f;
        float clippedFraction = 0.0f; // 0..1 share of samples that hit the clipper
        bool  clipActive = false, limitActive = false;
    };

    static constexpr int historyLength = 600; // 6 s at 10 ms per slot
    using History = HistoryBuffer<Slot, historyLength>;

    const History& getHistory() const noexcept  { return history; }

    /** Share of samples clipped over the last few seconds (0..1), for the readout. */
    float getRecentClippedFraction() const noexcept  { return recentClippedFraction.load(); }

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

    bool clipOn = false;

    std::atomic<float> clipInputLevel { 0.0f }, postClipLevel { 0.0f }, postLimitLevel { 0.0f };
    std::atomic<float> clipReductionDb { 0.0f }, limitReductionDb { 0.0f };
    int clipHoldCounter = 0, limitHoldCounter = 0;
    float meterDecayCoeff = 0.999f;

    // Slot accumulation for the history ring.
    History history;
    Slot pendingSlot;
    int slotLengthSamples = 441, samplesIntoSlot = 0, clippedInSlot = 0;
    std::atomic<float> recentClippedFraction { 0.0f };
    float recentClipAverage = 0.0f;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MasterChain)
};
