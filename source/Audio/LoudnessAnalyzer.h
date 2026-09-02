#pragma once

#include <juce_audio_basics/juce_audio_basics.h>

#include <array>
#include <atomic>

//==============================================================================
/**
    EBU R128 / ITU-R BS.1770-4 loudness analyzer.

    Measures K-weighted momentary (400 ms), short-term (3 s), and gated
    integrated loudness in LUFS, plus true peak in dBTP (4x-oversampled
    inter-sample peak per BS.1770-4 Annex 2). process() is lock-free and
    allocation-free, safe for the audio thread; results are published
    through atomics for the UI thread.
*/
class LoudnessAnalyzer
{
public:
    static constexpr float silenceLufs = -100.0f;

    struct Stats
    {
        float integratedLufs = silenceLufs;
        float truePeakDb = silenceLufs;
    };

    LoudnessAnalyzer() = default;

    void prepare (double sampleRate, int maxBlockSize);
    void reset();

    /** Processes a block of audio. Audio thread only. */
    void process (const juce::AudioBuffer<float>& buffer) noexcept;

    float getMomentaryLufs() const noexcept   { return momentaryLufs.load(); }
    float getShortTermLufs() const noexcept   { return shortTermLufs.load(); }
    float getIntegratedLufs() const noexcept  { return integratedLufs.load(); }
    float getTruePeakDb() const noexcept      { return truePeakDb.load(); }

    /** Analyzes an entire buffer offline (message/background thread). */
    static Stats analyzeBuffer (const juce::AudioBuffer<float>& buffer, double sampleRate);

private:
    //==============================================================================
    /** One direct-form-I biquad per channel pair, coefficients shared. */
    struct Biquad
    {
        double b0 = 1.0, b1 = 0.0, b2 = 0.0, a1 = 0.0, a2 = 0.0;

        struct State { double x1 = 0.0, x2 = 0.0, y1 = 0.0, y2 = 0.0; };

        float processSample (State& s, float x) const noexcept
        {
            const double y = b0 * x + b1 * s.x1 + b2 * s.x2 - a1 * s.y1 - a2 * s.y2;
            s.x2 = s.x1; s.x1 = x;
            s.y2 = s.y1; s.y1 = y;
            return (float) y;
        }
    };

    static Biquad makeShelf (double sampleRate);
    static Biquad makeHighPass (double sampleRate);

    static constexpr int maxChannels = 2;

    //==============================================================================
    /** BS.1770-4 Annex 2 true-peak meter: 4x oversampling via a 4-phase,
        12-taps-per-phase polyphase FIR interpolator. */
    struct TruePeakDetector
    {
        static constexpr int numPhases = 4;
        static constexpr int numTaps = 12;
        static const float coefficients[numPhases][numTaps];

        void reset() noexcept
        {
            for (auto& h : history)
                h.fill (0.0f);

            currentMax = 0.0f;
        }

        /** Feeds a block and returns the running maximum oversampled magnitude. */
        float processBlock (const float* const* channels, int numChannels, int numSamples) noexcept;

        std::array<std::array<float, numTaps>, maxChannels> history {};
        float currentMax = 0.0f;
    };

    static constexpr int stepsPerMomentaryWindow = 4;   // 400 ms window / 100 ms step
    static constexpr int stepsPerShortTermWindow = 30;  // 3 s window / 100 ms step

    // Gating histogram: 0.1 LU bins covering [-70, 0) LUFS.
    static constexpr int numHistogramBins = 700;
    static constexpr float histogramMinLufs = -70.0f;
    static constexpr float histogramBinWidth = 0.1f;

    //==============================================================================
    /** Internal measurement engine, usable from both real-time and offline paths. */
    struct Engine
    {
        void prepare (double sampleRate);
        void reset();

        /** Feeds samples; calls onStep(meanSquareOf100ms) each completed 100 ms step. */
        template <typename StepCallback>
        void processSamples (const float* const* channels, int numChannels, int numSamples,
                             StepCallback&& onStep) noexcept;

        float computeIntegrated() const noexcept;

        Biquad shelf, highPass;
        std::array<Biquad::State, maxChannels> shelfState, highPassState;

        int stepSizeSamples = 0;
        int samplesIntoStep = 0;
        double stepSumSquares = 0.0;

        std::array<double, stepsPerShortTermWindow> stepEnergies {}; // ring buffer of 100 ms mean squares
        int stepWriteIndex = 0;
        int stepsFilled = 0;

        std::array<uint32_t, numHistogramBins> histogram {};
        double histogramWeightedSum = 0.0; // sum of block energies passing the absolute gate
    };

    Engine engine;
    TruePeakDetector truePeak;

    std::atomic<float> momentaryLufs { silenceLufs };
    std::atomic<float> shortTermLufs { silenceLufs };
    std::atomic<float> integratedLufs { silenceLufs };
    std::atomic<float> truePeakDb { silenceLufs };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (LoudnessAnalyzer)
};
