#include "LoudnessHistoryView.h"
#include "ParityLookAndFeel.h"

LoudnessHistoryView::LoudnessHistoryView (const LoudnessAnalyzer& mixAnalyzer,
                                          const LoudnessAnalyzer& referenceAnalyzer)
    : mix (mixAnalyzer), reference (referenceAnalyzer)
{
    setOpaque (true);
    startTimerHz (10);
}

void LoudnessHistoryView::timerCallback()
{
    mixCount = mix.getHistory().read (mixData);
    refCount = reference.getHistory().read (refData);
    repaint();
}

float LoudnessHistoryView::lufsToY (float lufs, juce::Rectangle<float> plot) const noexcept
{
    return juce::jmap (juce::jlimit (minLufs, maxLufs, lufs), minLufs, maxLufs, plot.getBottom(), plot.getY());
}

juce::Path LoudnessHistoryView::buildTrace (const std::array<LoudnessAnalyzer::HistorySample, LoudnessAnalyzer::historyLength>& data,
                                            int count, juce::Rectangle<float> plot, bool shortTerm) const
{
    juce::Path path;
    const auto stepWidth = plot.getWidth() / (float) LoudnessAnalyzer::historyLength;
    bool started = false;

    for (int i = 0; i < count; ++i)
    {
        const auto& sample = data[(size_t) i];
        const auto lufs = shortTerm ? sample.shortTermLufs : sample.momentaryLufs;
        const auto x = plot.getRight() - (float) (count - i) * stepWidth;

        if (lufs <= LoudnessAnalyzer::silenceLufs + 1.0f)
        {
            started = false; // gaps in the data break the line
            continue;
        }

        const auto y = lufsToY (lufs, plot);

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

void LoudnessHistoryView::paint (juce::Graphics& g)
{
    using LF = ParityLookAndFeel;

    g.fillAll (LF::background);

    auto bounds = getLocalBounds().toFloat();
    bounds.removeFromRight (40.0f);
    const auto plot = bounds.withTrimmedBottom (20.0f);

    g.setFont (LF::getFont (10.0f));

    // Horizontal LUFS grid every 6 LU, with streaming-target band at -14.
    for (float lufs = maxLufs - 6.0f; lufs > minLufs; lufs -= 6.0f)
    {
        const auto y = lufsToY (lufs, plot);
        g.setColour (LF::gridLine);
        g.drawHorizontalLine ((int) y, plot.getX(), plot.getRight());
        g.setColour (LF::inkDim);
        g.drawText (juce::String ((int) lufs), (int) plot.getRight() + 10, (int) y - 6, 30, 12,
                    juce::Justification::centredLeft);
    }

    g.setColour (LF::inkDim);
    g.drawText ("LUFS", (int) plot.getRight() + 10, (int) plot.getBottom() + 4, 30, 12,
                juce::Justification::centredLeft);

    // Time grid every 10 s.
    for (int s = 0; s <= 60; s += 10)
    {
        const auto x = plot.getRight() - plot.getWidth() * (float) s / 60.0f;
        g.setColour (LF::gridLine);
        g.drawVerticalLine ((int) x, plot.getY(), plot.getBottom());
        g.setColour (LF::inkFaint);
        g.drawText (s == 0 ? juce::String ("now") : "-" + juce::String (s) + " s",
                    juce::jlimit ((int) plot.getX(), (int) plot.getRight() - 40, (int) x - 20),
                    (int) plot.getBottom() + 4, 40, 12,
                    s == 0 ? juce::Justification::centredRight
                           : s == 60 ? juce::Justification::centredLeft
                                     : juce::Justification::centred);
    }

    //==========================================================================
    juce::Graphics::ScopedSaveState saved (g);
    g.reduceClipRegion (plot.toNearestInt());

    // Integrated guide lines.
    auto drawGuide = [&] (float lufs, juce::Colour colour, const juce::String& label)
    {
        if (lufs <= LoudnessAnalyzer::silenceLufs + 1.0f)
            return;

        const auto y = lufsToY (lufs, plot);
        const float dashes[] = { 4.0f, 3.0f };
        juce::Path line, dashed;
        line.startNewSubPath (plot.getX(), y);
        line.lineTo (plot.getRight(), y);
        juce::PathStrokeType (1.0f).createDashedStroke (dashed, line, dashes, 2);

        g.setColour (colour.withAlpha (0.7f));
        g.fillPath (dashed);
        g.setFont (LF::getFont (9.0f, juce::Font::bold));
        g.drawText (label + " " + juce::String (lufs, 1), (int) plot.getX() + 6, (int) y - 13, 120, 12,
                    juce::Justification::centredLeft);
    };

    drawGuide (referenceIntegrated, LF::accent, "REF INT");
    drawGuide (mix.getIntegratedLufs(), LF::inkSoft, "MIX INT");

    // Momentary traces faint, short-term traces solid.
    g.setColour (LF::accent.withAlpha (0.25f));
    g.strokePath (buildTrace (refData, refCount, plot, false), juce::PathStrokeType (1.0f));
    g.setColour (LF::inkSoft.withAlpha (0.25f));
    g.strokePath (buildTrace (mixData, mixCount, plot, false), juce::PathStrokeType (1.0f));

    auto refShort = buildTrace (refData, refCount, plot, true);
    auto mixShort = buildTrace (mixData, mixCount, plot, true);

    g.setColour (LF::accent);
    g.strokePath (refShort, juce::PathStrokeType (1.8f));
    g.setColour (LF::inkSoft);
    g.strokePath (mixShort, juce::PathStrokeType (1.8f));

    if (mixShort.isEmpty() && refShort.isEmpty())
    {
        g.setColour (LF::inkDim);
        g.setFont (LF::getFont (12.0f));
        g.drawText (mixCount == 0 ? "Press play in your DAW" : "Short-term loudness appears after 3 s of audio",
                    plot, juce::Justification::centred);
    }
}
