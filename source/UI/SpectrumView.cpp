#include "SpectrumView.h"
#include "ParityLookAndFeel.h"

SpectrumView::SpectrumView (SpectrumAnalyzer& mixAnalyzer, SpectrumAnalyzer& referenceAnalyzer)
    : mix (mixAnalyzer), reference (referenceAnalyzer)
{
    setOpaque (true);
    startTimerHz (30);
}

void SpectrumView::setDisplay (Display newDisplay)
{
    if (display != newDisplay)
    {
        display = newDisplay;
        repaint();
    }
}

void SpectrumView::setAveraging (bool shouldAverage)
{
    const auto newMode = shouldAverage ? SpectrumAnalyzer::Mode::average
                                       : SpectrumAnalyzer::Mode::realtime;
    mix.setMode (newMode);
    reference.setMode (newMode);
}

bool SpectrumView::isAveraging() const noexcept
{
    return mix.getMode() == SpectrumAnalyzer::Mode::average;
}

void SpectrumView::timerCallback()
{
    auto updated = mix.computeSpectrum();
    updated = reference.computeSpectrum() || updated;

    if (updated)
        repaint();
}

//==============================================================================
float SpectrumView::frequencyToX (float hz, juce::Rectangle<float> plotArea) const
{
    const auto normalised = std::log (hz / minFrequency) / std::log (maxFrequency / minFrequency);
    return plotArea.getX() + normalised * plotArea.getWidth();
}

juce::Path SpectrumView::buildCurve (const std::array<float, SpectrumAnalyzer::numBins>& magnitudesDb,
                                     juce::Rectangle<float> plotArea,
                                     const SpectrumAnalyzer& analyzer) const
{
    juce::Path path;
    bool started = false;

    for (int bin = 1; bin < SpectrumAnalyzer::numBins; ++bin)
    {
        const auto hz = analyzer.binFrequency (bin);

        if (hz < minFrequency)
            continue;

        if (hz > maxFrequency)
            break;

        // Visual tilt so pink-noise-like material reads roughly flat.
        const auto tilt = tiltDbPerOctave * std::log2 (hz / 1000.0f);
        const auto db = juce::jlimit (minDb, maxDb, magnitudesDb[(size_t) bin] + tilt);

        const auto x = frequencyToX (hz, plotArea);
        const auto y = juce::jmap (db, minDb, maxDb, plotArea.getBottom(), plotArea.getY());

        if (! started)
        {
            path.startNewSubPath (x, y);
            started = true;
        }
        else
        {
            path.lineTo (x, y);
        }
    }

    return path;
}

//==============================================================================
void SpectrumView::paint (juce::Graphics& g)
{
    g.fillAll (ParityLookAndFeel::cream);

    auto plotArea = getLocalBounds().toFloat().reduced (1.0f);

    paintGrid (g, plotArea);

    if (display == Display::overlay)
        paintOverlay (g, plotArea);
    else
        paintDifference (g, plotArea);

    g.setColour (ParityLookAndFeel::ink);
    g.drawRect (getLocalBounds(), 1);
}

void SpectrumView::paintGrid (juce::Graphics& g, juce::Rectangle<float> plotArea)
{
    g.setColour (ParityLookAndFeel::panelLine);
    g.setFont (ParityLookAndFeel::getMonoFont (10.0f));

    // Octave frequency lines with labels.
    for (auto hz : { 31.5f, 63.0f, 125.0f, 250.0f, 500.0f, 1000.0f, 2000.0f, 4000.0f, 8000.0f, 16000.0f })
    {
        const auto x = frequencyToX (hz, plotArea);

        g.setColour (ParityLookAndFeel::panelLine);
        g.drawVerticalLine ((int) x, plotArea.getY(), plotArea.getBottom());

        const auto label = hz >= 1000.0f ? juce::String (hz / 1000.0f, hz == 1000.0f ? 0 : 0) + "k"
                                         : juce::String ((int) hz);

        g.setColour (ParityLookAndFeel::inkFaint);
        g.drawText (label, (int) x + 3, (int) plotArea.getBottom() - 14, 40, 12,
                    juce::Justification::centredLeft);
    }

    if (display == Display::overlay)
    {
        // 10 dB horizontal lines.
        for (float db = maxDb - 10.0f; db > minDb; db -= 10.0f)
        {
            const auto y = juce::jmap (db, minDb, maxDb, plotArea.getBottom(), plotArea.getY());
            g.setColour (ParityLookAndFeel::panelLine.withAlpha (0.6f));
            g.drawHorizontalLine ((int) y, plotArea.getX(), plotArea.getRight());
        }
    }
    else
    {
        // Difference mode: lines every 3 dB with a strong zero line.
        for (float db = -diffRangeDb + 3.0f; db < diffRangeDb; db += 3.0f)
        {
            const auto y = juce::jmap (db, -diffRangeDb, diffRangeDb, plotArea.getBottom(), plotArea.getY());

            g.setColour (db == 0.0f ? ParityLookAndFeel::ink.withAlpha (0.5f)
                                    : ParityLookAndFeel::panelLine.withAlpha (0.6f));
            g.drawHorizontalLine ((int) y, plotArea.getX(), plotArea.getRight());
        }
    }
}

void SpectrumView::paintOverlay (juce::Graphics& g, juce::Rectangle<float> plotArea)
{
    // Reference behind, mix in front.
    auto referencePath = buildCurve (reference.getMagnitudesDb(), plotArea, reference);
    g.setColour (ParityLookAndFeel::accent);
    g.strokePath (referencePath, juce::PathStrokeType (1.6f));

    auto mixPath = buildCurve (mix.getMagnitudesDb(), plotArea, mix);

    // Soft fill under the mix curve.
    if (! mixPath.isEmpty())
    {
        auto fillPath = mixPath;
        fillPath.lineTo (plotArea.getRight(), plotArea.getBottom());
        fillPath.lineTo (plotArea.getX(), plotArea.getBottom());
        fillPath.closeSubPath();

        g.setColour (ParityLookAndFeel::ink.withAlpha (0.08f));
        g.fillPath (fillPath);
    }

    g.setColour (ParityLookAndFeel::ink);
    g.strokePath (mixPath, juce::PathStrokeType (1.8f));
}

void SpectrumView::paintDifference (juce::Graphics& g, juce::Rectangle<float> plotArea)
{
    const auto& mixDb = mix.getMagnitudesDb();
    const auto& refDb = reference.getMagnitudesDb();

    juce::Path path;
    bool started = false;

    for (int bin = 1; bin < SpectrumAnalyzer::numBins; ++bin)
    {
        const auto hz = mix.binFrequency (bin);

        if (hz < minFrequency)
            continue;

        if (hz > maxFrequency)
            break;

        // Treat near-floor bins as "no data" and pin them to zero difference.
        const auto mixValue = mixDb[(size_t) bin];
        const auto refValue = refDb[(size_t) bin];

        const auto hasData = mixValue > SpectrumAnalyzer::floorDb + 6.0f
                          && refValue > SpectrumAnalyzer::floorDb + 6.0f;

        const auto difference = hasData ? juce::jlimit (-diffRangeDb, diffRangeDb, mixValue - refValue)
                                        : 0.0f;

        const auto x = frequencyToX (hz, plotArea);
        const auto y = juce::jmap (difference, -diffRangeDb, diffRangeDb,
                                   plotArea.getBottom(), plotArea.getY());

        if (! started)
        {
            path.startNewSubPath (x, y);
            started = true;
        }
        else
        {
            path.lineTo (x, y);
        }
    }

    // Fill between the curve and the zero line.
    if (! path.isEmpty())
    {
        const auto zeroY = juce::jmap (0.0f, -diffRangeDb, diffRangeDb,
                                       plotArea.getBottom(), plotArea.getY());

        auto fillPath = path;
        fillPath.lineTo (plotArea.getRight(), zeroY);
        fillPath.lineTo (plotArea.getX(), zeroY);
        fillPath.closeSubPath();

        g.setColour (ParityLookAndFeel::accent.withAlpha (0.15f));
        g.fillPath (fillPath);
    }

    g.setColour (ParityLookAndFeel::ink);
    g.strokePath (path, juce::PathStrokeType (1.8f));
}
