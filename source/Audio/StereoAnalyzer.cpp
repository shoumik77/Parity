#include "StereoAnalyzer.h"

namespace
{
    constexpr double silenceThreshold = 1.0e-8; // mean-square gate
}

void StereoAnalyzer::prepare (double sampleRate)
{
    currentSampleRate = sampleRate;
    reset();
}

void StereoAnalyzer::reset()
{
    sumLL = sumRR = sumLR = sumMM = sumSS = 0.0;
    correlation.store (noValue);
    widthDb.store (noValue);
}

void StereoAnalyzer::process (const juce::AudioBuffer<float>& buffer) noexcept
{
    const auto numSamples = buffer.getNumSamples();

    if (buffer.getNumChannels() < 2 || numSamples == 0)
    {
        correlation.store (noValue);
        widthDb.store (noValue);
        return;
    }

    const auto* left = buffer.getReadPointer (0);
    const auto* right = buffer.getReadPointer (1);

    double blockLL = 0.0, blockRR = 0.0, blockLR = 0.0, blockMM = 0.0, blockSS = 0.0;

    for (int i = 0; i < numSamples; ++i)
    {
        const double l = left[i];
        const double r = right[i];
        const double mid = 0.5 * (l + r);
        const double side = 0.5 * (l - r);

        blockLL += l * l;
        blockRR += r * r;
        blockLR += l * r;
        blockMM += mid * mid;
        blockSS += side * side;
    }

    const auto invNumSamples = 1.0 / numSamples;

    // Exponential smoothing sized so the window is ~400 ms regardless of
    // the host block size (alpha = blockDuration / windowDuration, capped).
    const auto alpha = juce::jlimit (0.01, 1.0, (double) numSamples / (0.4 * currentSampleRate));

    auto smooth = [alpha] (double& state, double blockValue)
    {
        state += alpha * (blockValue - state);
    };

    smooth (sumLL, blockLL * invNumSamples);
    smooth (sumRR, blockRR * invNumSamples);
    smooth (sumLR, blockLR * invNumSamples);
    smooth (sumMM, blockMM * invNumSamples);
    smooth (sumSS, blockSS * invNumSamples);

    const auto totalEnergy = sumLL + sumRR;

    if (totalEnergy < silenceThreshold)
    {
        correlation.store (noValue);
        widthDb.store (noValue);
        return;
    }

    const auto denominator = std::sqrt (sumLL * sumRR);
    correlation.store (denominator > 0.0 ? (float) juce::jlimit (-1.0, 1.0, sumLR / denominator)
                                         : (float) noValue);

    if (sumMM > 0.0)
    {
        const auto ratioDb = 10.0 * std::log10 (juce::jmax (1.0e-10, sumSS / sumMM));
        widthDb.store ((float) juce::jmax ((double) widthFloorDb, ratioDb));
    }
    else
    {
        // Pure side signal (out-of-phase): maximum width.
        widthDb.store (0.0f);
    }
}
