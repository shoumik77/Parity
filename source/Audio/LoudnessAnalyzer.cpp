#include "LoudnessAnalyzer.h"

namespace
{
    constexpr double lufsOffset = -0.691; // BS.1770 K-weighted loudness offset

    float energyToLoudness (double energy) noexcept
    {
        if (energy <= 0.0)
            return LoudnessAnalyzer::silenceLufs;

        return (float) (lufsOffset + 10.0 * std::log10 (energy));
    }

    double loudnessToEnergy (double lufs) noexcept
    {
        return std::pow (10.0, (lufs - lufsOffset) / 10.0);
    }
}

//==============================================================================
// K-weighting filter coefficients re-derived for arbitrary sample rates using
// the parametric definitions behind the BS.1770-4 48 kHz reference tables.
LoudnessAnalyzer::Biquad LoudnessAnalyzer::makeShelf (double sampleRate)
{
    constexpr double f0 = 1681.974450955533;
    constexpr double gainDb = 3.999843853973347;
    constexpr double q = 0.7071752369554196;

    const double k = std::tan (juce::MathConstants<double>::pi * f0 / sampleRate);
    const double vh = std::pow (10.0, gainDb / 20.0);
    const double vb = std::pow (vh, 0.4996667741545416);
    const double norm = 1.0 + k / q + k * k;

    Biquad bq;
    bq.b0 = (vh + vb * k / q + k * k) / norm;
    bq.b1 = 2.0 * (k * k - vh) / norm;
    bq.b2 = (vh - vb * k / q + k * k) / norm;
    bq.a1 = 2.0 * (k * k - 1.0) / norm;
    bq.a2 = (1.0 - k / q + k * k) / norm;
    return bq;
}

LoudnessAnalyzer::Biquad LoudnessAnalyzer::makeHighPass (double sampleRate)
{
    constexpr double f0 = 38.13547087602444;
    constexpr double q = 0.5003270373238773;

    const double k = std::tan (juce::MathConstants<double>::pi * f0 / sampleRate);
    const double norm = 1.0 + k / q + k * k;

    Biquad bq;
    bq.b0 = 1.0;
    bq.b1 = -2.0;
    bq.b2 = 1.0;
    bq.a1 = 2.0 * (k * k - 1.0) / norm;
    bq.a2 = (1.0 - k / q + k * k) / norm;
    return bq;
}

//==============================================================================
void LoudnessAnalyzer::Engine::prepare (double sampleRate)
{
    shelf = makeShelf (sampleRate);
    highPass = makeHighPass (sampleRate);
    stepSizeSamples = juce::jmax (1, (int) std::round (sampleRate * 0.1));
    reset();
}

void LoudnessAnalyzer::Engine::reset()
{
    shelfState.fill ({});
    highPassState.fill ({});
    samplesIntoStep = 0;
    stepSumSquares = 0.0;
    stepEnergies.fill (0.0);
    stepWriteIndex = 0;
    stepsFilled = 0;
    histogram.fill (0);
    histogramWeightedSum = 0.0;
}

template <typename StepCallback>
void LoudnessAnalyzer::Engine::processSamples (const float* const* channels, int numChannels,
                                               int numSamples, StepCallback&& onStep) noexcept
{
    const auto channelsToUse = juce::jmin (numChannels, maxChannels);

    for (int i = 0; i < numSamples; ++i)
    {
        double sumSquares = 0.0;

        for (int ch = 0; ch < channelsToUse; ++ch)
        {
            auto sample = shelf.processSample (shelfState[(size_t) ch], channels[ch][i]);
            sample = highPass.processSample (highPassState[(size_t) ch], sample);
            sumSquares += (double) sample * (double) sample;
        }

        stepSumSquares += sumSquares;

        if (++samplesIntoStep >= stepSizeSamples)
        {
            const auto stepEnergy = stepSumSquares / stepSizeSamples;

            stepEnergies[(size_t) stepWriteIndex] = stepEnergy;
            stepWriteIndex = (stepWriteIndex + 1) % stepsPerShortTermWindow;
            stepsFilled = juce::jmin (stepsFilled + 1, stepsPerShortTermWindow);

            samplesIntoStep = 0;
            stepSumSquares = 0.0;

            onStep();
        }
    }
}

float LoudnessAnalyzer::Engine::computeIntegrated() const noexcept
{
    uint64_t totalCount = 0;

    for (auto count : histogram)
        totalCount += count;

    if (totalCount == 0)
        return silenceLufs;

    // Relative gate: -10 LU below the loudness of all blocks passing the absolute gate.
    const auto relativeThreshold = energyToLoudness (histogramWeightedSum / (double) totalCount) - 10.0f;

    double gatedEnergy = 0.0;
    uint64_t gatedCount = 0;

    for (int bin = 0; bin < numHistogramBins; ++bin)
    {
        if (histogram[(size_t) bin] == 0)
            continue;

        const auto binLoudness = histogramMinLufs + (bin + 0.5f) * histogramBinWidth;

        if (binLoudness >= relativeThreshold)
        {
            gatedEnergy += loudnessToEnergy (binLoudness) * histogram[(size_t) bin];
            gatedCount += histogram[(size_t) bin];
        }
    }

    if (gatedCount == 0)
        return silenceLufs;

    return energyToLoudness (gatedEnergy / (double) gatedCount);
}

//==============================================================================
void LoudnessAnalyzer::prepare (double sampleRate, int maxBlockSize)
{
    juce::ignoreUnused (maxBlockSize);
    engine.prepare (sampleRate);
    reset();
}

void LoudnessAnalyzer::reset()
{
    engine.reset();
    currentPeak = 0.0f;
    momentaryLufs.store (silenceLufs);
    shortTermLufs.store (silenceLufs);
    integratedLufs.store (silenceLufs);
    peakDb.store (silenceLufs);
}

void LoudnessAnalyzer::process (const juce::AudioBuffer<float>& buffer) noexcept
{
    const auto numChannels = buffer.getNumChannels();
    const auto numSamples = buffer.getNumSamples();

    for (int ch = 0; ch < juce::jmin (numChannels, maxChannels); ++ch)
        currentPeak = juce::jmax (currentPeak, buffer.getMagnitude (ch, 0, numSamples));

    peakDb.store (currentPeak > 0.0f ? juce::Decibels::gainToDecibels (currentPeak)
                                     : silenceLufs);

    engine.processSamples (buffer.getArrayOfReadPointers(), numChannels, numSamples, [this]
    {
        auto& e = engine;

        // Mean square over the most recent N steps of the ring buffer.
        auto windowEnergy = [&e] (int numSteps)
        {
            double sum = 0.0;

            for (int i = 0; i < numSteps; ++i)
            {
                const auto index = (e.stepWriteIndex - 1 - i + stepsPerShortTermWindow) % stepsPerShortTermWindow;
                sum += e.stepEnergies[(size_t) index];
            }

            return sum / numSteps;
        };

        if (e.stepsFilled >= stepsPerMomentaryWindow)
        {
            const auto momentaryEnergy = windowEnergy (stepsPerMomentaryWindow);
            momentaryLufs.store (energyToLoudness (momentaryEnergy));

            // Gating for the integrated measurement (absolute gate at -70 LUFS).
            const auto blockLoudness = energyToLoudness (momentaryEnergy);

            if (blockLoudness > histogramMinLufs)
            {
                const auto bin = juce::jlimit (0, numHistogramBins - 1,
                                               (int) ((blockLoudness - histogramMinLufs) / histogramBinWidth));
                ++e.histogram[(size_t) bin];
                e.histogramWeightedSum += momentaryEnergy;
                integratedLufs.store (e.computeIntegrated());
            }
        }

        if (e.stepsFilled >= stepsPerShortTermWindow)
            shortTermLufs.store (energyToLoudness (windowEnergy (stepsPerShortTermWindow)));
    });
}

//==============================================================================
LoudnessAnalyzer::Stats LoudnessAnalyzer::analyzeBuffer (const juce::AudioBuffer<float>& buffer,
                                                         double sampleRate)
{
    LoudnessAnalyzer analyzer;
    analyzer.prepare (sampleRate, buffer.getNumSamples());
    analyzer.process (buffer);

    Stats stats;
    stats.integratedLufs = analyzer.getIntegratedLufs();
    stats.peakDb = analyzer.getPeakDb();
    return stats;
}
