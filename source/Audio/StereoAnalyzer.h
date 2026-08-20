#pragma once

#include <juce_audio_basics/juce_audio_basics.h>

#include <atomic>

//==============================================================================
/**
    Measures the stereo image: inter-channel correlation and width.

    - Correlation: Pearson correlation of L/R, -1..+1. +1 is mono-compatible,
      0 is fully decorrelated, negative values indicate phase problems.
    - Width: side-to-mid energy ratio in dB. -inf is mono, 0 dB means equal
      mid and side energy.

    Measurements are exponentially smoothed over roughly 400 ms. process()
    is allocation-free and safe for the audio thread; results are published
    through atomics.
*/
class StereoAnalyzer
{
public:
    static constexpr float widthFloorDb = -40.0f;
    static constexpr float noValue = -1000.0f; // silence / mono input marker

    StereoAnalyzer() = default;

    void prepare (double sampleRate);
    void reset();

    /** Processes a block. Audio thread only. */
    void process (const juce::AudioBuffer<float>& buffer) noexcept;

    /** Correlation -1..+1, or noValue when there's no usable signal. */
    float getCorrelation() const noexcept  { return correlation.load(); }

    /** Width in dB (side/mid), clamped to widthFloorDb, or noValue. */
    float getWidthDb() const noexcept      { return widthDb.load(); }

private:
    double currentSampleRate = 48000.0;

    double sumLL = 0.0, sumRR = 0.0, sumLR = 0.0;
    double sumMM = 0.0, sumSS = 0.0;

    std::atomic<float> correlation { noValue };
    std::atomic<float> widthDb { noValue };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (StereoAnalyzer)
};
