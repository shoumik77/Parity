#include "SpectrumAnalyzer.h"

SpectrumAnalyzer::SpectrumAnalyzer()
{
    smoothedDb.fill (floorDb);
}

void SpectrumAnalyzer::prepare (double newSampleRate)
{
    sampleRate = newSampleRate;
    reset();
}

void SpectrumAnalyzer::reset()
{
    abstractFifo.reset();
    fifoBuffer.fill (0.0f);
    smoothedDb.fill (floorDb);
    resetAverage();
}

void SpectrumAnalyzer::setMode (Mode newMode)
{
    if (mode != newMode)
    {
        mode = newMode;
        resetAverage();
    }
}

void SpectrumAnalyzer::resetAverage()
{
    averageAccumulator.fill (0.0);
    averageCount = 0;
}

//==============================================================================
void SpectrumAnalyzer::process (const juce::AudioBuffer<float>& buffer) noexcept
{
    const auto numChannels = buffer.getNumChannels();
    const auto numSamples = buffer.getNumSamples();

    if (numChannels == 0)
        return;

    const auto channelGain = 1.0f / (float) numChannels;

    int start1, size1, start2, size2;
    abstractFifo.prepareToWrite (numSamples, start1, size1, start2, size2);

    auto writeRegion = [&] (int start, int size, int sourceOffset)
    {
        for (int i = 0; i < size; ++i)
        {
            float mono = 0.0f;

            for (int ch = 0; ch < numChannels; ++ch)
                mono += buffer.getSample (ch, sourceOffset + i);

            fifoBuffer[(size_t) (start + i)] = mono * channelGain;
        }
    };

    writeRegion (start1, size1, 0);
    writeRegion (start2, size2, size1);

    abstractFifo.finishedWrite (size1 + size2);
}

bool SpectrumAnalyzer::computeSpectrum()
{
    // Require at least one full FFT frame of new samples... but to keep the
    // display responsive with overlap, consume whatever is available and
    // maintain a sliding window.
    const auto available = abstractFifo.getNumReady();

    if (available <= 0)
        return false;

    const auto toRead = juce::jmin (available, fftSize);

    // Slide the analysis window left and append the new samples.
    if (toRead < fftSize)
        std::memmove (sampleWindow.data(), sampleWindow.data() + toRead,
                      (size_t) (fftSize - toRead) * sizeof (float));

    int start1, size1, start2, size2;
    abstractFifo.prepareToRead (toRead, start1, size1, start2, size2);

    auto* dest = sampleWindow.data() + (fftSize - toRead);
    std::memcpy (dest, fifoBuffer.data() + start1, (size_t) size1 * sizeof (float));

    if (size2 > 0)
        std::memcpy (dest + size1, fifoBuffer.data() + start2, (size_t) size2 * sizeof (float));

    abstractFifo.finishedRead (size1 + size2);

    // Window + FFT.
    std::memcpy (fftData.data(), sampleWindow.data(), fftSize * sizeof (float));
    std::fill (fftData.begin() + fftSize, fftData.end(), 0.0f);
    window.multiplyWithWindowingTable (fftData.data(), fftSize);
    fft.performFrequencyOnlyForwardTransform (fftData.data());

    // Normalisation: 2/N for one-sided spectrum, ~2 for the Hann window's
    // coherent gain of 0.5.
    constexpr float normalisation = 4.0f / fftSize;

    for (int bin = 0; bin < numBins; ++bin)
    {
        const auto magnitude = fftData[(size_t) bin] * normalisation;
        const auto db = juce::jmax (floorDb, juce::Decibels::gainToDecibels (magnitude, floorDb));

        if (mode == Mode::average)
        {
            averageAccumulator[(size_t) bin] += db;

            if (averageCount == 0)
                smoothedDb[(size_t) bin] = db;
        }
        else
        {
            // Fast attack, slow release.
            auto& smoothed = smoothedDb[(size_t) bin];
            const auto coefficient = db > smoothed ? 0.6f : 0.12f;
            smoothed += coefficient * (db - smoothed);
        }
    }

    if (mode == Mode::average)
    {
        ++averageCount;

        for (int bin = 0; bin < numBins; ++bin)
            smoothedDb[(size_t) bin] = (float) (averageAccumulator[(size_t) bin] / averageCount);
    }

    return true;
}
