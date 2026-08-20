#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

#include "../Audio/SpectrumAnalyzer.h"

//==============================================================================
/**
    Draws the mix and reference spectra in Parity's retro instrument style.

    Display modes:
      - Overlay:    mix (ink) and reference (orange) curves on shared axes
      - Difference: single curve of (mix - reference) dB around a zero line

    Smoothing modes map straight onto SpectrumAnalyzer::Mode.
*/
class SpectrumView final : public juce::Component,
                           private juce::Timer
{
public:
    enum class Display { overlay, difference };

    SpectrumView (SpectrumAnalyzer& mixAnalyzer, SpectrumAnalyzer& referenceAnalyzer);

    void setDisplay (Display newDisplay);
    Display getDisplay() const noexcept  { return display; }

    void setAveraging (bool shouldAverage);
    bool isAveraging() const noexcept;

    //==============================================================================
    void paint (juce::Graphics&) override;

private:
    void timerCallback() override;

    juce::Path buildCurve (const std::array<float, SpectrumAnalyzer::numBins>& magnitudesDb,
                           juce::Rectangle<float> plotArea, const SpectrumAnalyzer& analyzer) const;
    void paintOverlay (juce::Graphics&, juce::Rectangle<float> plotArea);
    void paintDifference (juce::Graphics&, juce::Rectangle<float> plotArea);
    void paintGrid (juce::Graphics&, juce::Rectangle<float> plotArea);
    void paintLegend (juce::Graphics&, juce::Rectangle<float> plotArea);
    void paintHint (juce::Graphics&, juce::Rectangle<float> plotArea);

    static bool hasData (const std::array<float, SpectrumAnalyzer::numBins>& magnitudesDb) noexcept;

    float frequencyToX (float hz, juce::Rectangle<float> plotArea) const;

    SpectrumAnalyzer& mix;
    SpectrumAnalyzer& reference;
    Display display = Display::overlay;

    static constexpr float minFrequency = 20.0f;
    static constexpr float maxFrequency = 20000.0f;
    static constexpr float minDb = -90.0f;
    static constexpr float maxDb = 0.0f;
    static constexpr float diffRangeDb = 12.0f;
    static constexpr float tiltDbPerOctave = 4.5f; // visual slope compensation

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SpectrumView)
};
