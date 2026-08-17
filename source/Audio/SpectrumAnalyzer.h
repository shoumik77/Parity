#pragma once

#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_dsp/juce_dsp.h>

#include <array>
#include <atomic>

//==============================================================================
/**
    FFT spectrum tap. The audio thread pushes mono-summed samples into a
    lock-free FIFO; the UI thread calls computeSpectrum() periodically to
    window, transform, and smooth the magnitudes.

    Two smoothing modes:
      - Realtime: per-bin exponential smoothing (fast attack, slow release)
      - Average:  running mean over everything since the last resetAverage()
*/
class SpectrumAnalyzer
{
public:
    static constexpr int fftOrder = 12;
    static constexpr int fftSize = 1 << fftOrder;         // 4096
    static constexpr int numBins = fftSize / 2;
    static constexpr float floorDb = -100.0f;

    enum class Mode { realtime, average };

    SpectrumAnalyzer();

    void prepare (double sampleRate);
    void reset();

    /** Pushes a block into the FIFO. Audio thread safe, lock-free. */
    void process (const juce::AudioBuffer<float>& buffer) noexcept;

    /** Runs the FFT over the most recent samples and updates the smoothed
        magnitudes. Call from the UI thread (e.g. a 30 Hz timer).
        Returns true if a new spectrum was produced. */
    bool computeSpectrum();

    void setMode (Mode newMode);
    Mode getMode() const noexcept  { return mode; }

    /** Clears the long-term average accumulator. */
    void resetAverage();

    /** Smoothed magnitude in dB for each bin. UI thread only. */
    const std::array<float, numBins>& getMagnitudesDb() const noexcept  { return smoothedDb; }

    double getSampleRate() const noexcept  { return sampleRate; }

    /** Frequency of a bin centre in Hz. */
    float binFrequency (int bin) const noexcept
    {
        return (float) (bin * sampleRate / fftSize);
    }

private:
    juce::dsp::FFT fft { fftOrder };
    juce::dsp::WindowingFunction<float> window { fftSize, juce::dsp::WindowingFunction<float>::hann };

    static constexpr int fifoSize = fftSize * 4;
    juce::AbstractFifo abstractFifo { fifoSize };
    std::array<float, fifoSize> fifoBuffer {};

    std::array<float, fftSize * 2> fftData {};
    std::array<float, fftSize> sampleWindow {};

    std::array<float, numBins> smoothedDb;
    std::array<double, numBins> averageAccumulator {};
    int averageCount = 0;

    Mode mode = Mode::realtime;
    double sampleRate = 44100.0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SpectrumAnalyzer)
};
