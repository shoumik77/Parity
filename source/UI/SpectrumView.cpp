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
bool SpectrumView::hasData (const std::array<float, SpectrumAnalyzer::numBins>& magnitudesDb) noexcept
{
    for (auto db : magnitudesDb)
        if (db > SpectrumAnalyzer::floorDb + 6.0f)
            return true;

    return false;
}

void SpectrumView::paint (juce::Graphics& g)
{
    g.fillAll (ParityLookAndFeel::background);

    // Reserve a right gutter for dBFS labels and a bottom row for frequencies.
    auto bounds = getLocalBounds().toFloat();
    bounds.removeFromRight (40.0f);
    auto plotArea = bounds.withTrimmedBottom (20.0f);

    paintGrid (g, plotArea);

    if (display == Display::overlay)
        paintOverlay (g, plotArea);
    else
        paintDifference (g, plotArea);

    paintHint (g, plotArea);
}

void SpectrumView::paintHint (juce::Graphics& g, juce::Rectangle<float> plotArea)
{
    const auto mixHasData = hasData (mix.getMagnitudesDb());
    const auto refHasData = hasData (reference.getMagnitudesDb());

    juce::String hint;

    if (! mixHasData && ! refHasData)
        hint = "Press play in your DAW";
    else if (! refHasData)
        hint = "Load a reference to compare";
    else
        return;

    g.setColour (ParityLookAndFeel::inkDim);
    g.setFont (ParityLookAndFeel::getFont (12.0f));
    g.drawText (hint, plotArea, juce::Justification::centred);
}

void SpectrumView::paintGrid (juce::Graphics& g, juce::Rectangle<float> plotArea)
{
    g.setFont (ParityLookAndFeel::getFont (10.0f));

    // Labelled octave divisions matching the design.
    for (auto hz : { 20.0f, 200.0f, 500.0f, 1000.0f, 2000.0f, 5000.0f, 10000.0f, 20000.0f })
    {
        const auto x = frequencyToX (hz, plotArea);

        g.setColour (ParityLookAndFeel::gridLine);
        g.drawVerticalLine ((int) x, plotArea.getY(), plotArea.getBottom());

        const auto label = hz >= 1000.0f ? juce::String ((int) (hz / 1000.0f)) + "k"
                                         : juce::String ((int) hz);

        g.setColour (ParityLookAndFeel::inkFaint);
        g.drawText (label, juce::jlimit ((int) plotArea.getX(), (int) plotArea.getRight() - 40,
                                         (int) x - 20),
                    (int) plotArea.getBottom() + 4, 40, 12, juce::Justification::centred);
    }

    const auto labelGutter = getLocalBounds().toFloat().withX (plotArea.getRight() + 10.0f);

    if (display == Display::overlay)
    {
        // 12 dB horizontal lines with right-gutter labels (-12 .. -72).
        for (float db = -12.0f; db > minDb; db -= 12.0f)
        {
            const auto y = juce::jmap (db, minDb, maxDb, plotArea.getBottom(), plotArea.getY());

            g.setColour (ParityLookAndFeel::gridLine);
            g.drawHorizontalLine ((int) y, plotArea.getX(), plotArea.getRight());

            g.setColour (ParityLookAndFeel::inkDim);
            g.drawText (juce::String ((int) db), (int) labelGutter.getX(), (int) y - 6, 30, 12,
                        juce::Justification::centredLeft);
        }

        g.drawText ("dBFS", (int) labelGutter.getX(), (int) plotArea.getBottom() + 4, 30, 12,
                    juce::Justification::centredLeft);
    }
    else
    {
        // Difference mode: lines every 3 dB with a strong zero line.
        for (float db = -diffRangeDb + 3.0f; db < diffRangeDb; db += 3.0f)
        {
            const auto y = juce::jmap (db, -diffRangeDb, diffRangeDb, plotArea.getBottom(), plotArea.getY());

            g.setColour (db == 0.0f ? ParityLookAndFeel::inkFaint.withAlpha (0.5f)
                                    : ParityLookAndFeel::gridLine);
            g.drawHorizontalLine ((int) y, plotArea.getX(), plotArea.getRight());
        }

        g.setColour (ParityLookAndFeel::inkDim);
        g.drawText ("MIX - REF dB", (int) labelGutter.getX(), (int) plotArea.getY(),
                    38, 12, juce::Justification::centredLeft);
    }
}

void SpectrumView::paintOverlay (juce::Graphics& g, juce::Rectangle<float> plotArea)
{
    auto paintCurve = [&] (const SpectrumAnalyzer& analyzer, juce::Colour colour, float fillAlpha)
    {
        auto path = buildCurve (analyzer.getMagnitudesDb(), plotArea, analyzer);

        if (path.isEmpty())
            return;

        if (fillAlpha > 0.0f)
        {
            auto fillPath = path;
            fillPath.lineTo (plotArea.getRight(), plotArea.getBottom());
            fillPath.lineTo (plotArea.getX(), plotArea.getBottom());
            fillPath.closeSubPath();

            g.setColour (colour.withAlpha (fillAlpha));
            g.fillPath (fillPath);
        }

        g.setColour (colour);
        g.strokePath (path, juce::PathStrokeType (1.6f));
    };

    // Reference behind (orange fill), mix in front (pale fill).
    paintCurve (reference, ParityLookAndFeel::accent, 0.18f);
    paintCurve (mix, ParityLookAndFeel::inkSoft, 0.12f);
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

    g.setColour (ParityLookAndFeel::inkSoft);
    g.strokePath (path, juce::PathStrokeType (1.8f));
}
