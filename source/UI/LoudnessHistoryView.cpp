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

    LF::paintRecessed (g, plot, 4.0f);

    g.setFont (LF::getMonoFont (9.5f));

    // Horizontal LUFS grid every 6 LU, minor lines every 3.
    for (float lufs = maxLufs - 3.0f; lufs > minLufs; lufs -= 3.0f)
    {
        if (std::fmod (-lufs, 6.0f) > 0.01f)
        {
            g.setColour (LF::gridMinor);
            g.drawHorizontalLine ((int) lufsToY (lufs, plot), plot.getX() + 1.0f, plot.getRight() - 1.0f);
        }
    }

    for (float lufs = maxLufs - 6.0f; lufs > minLufs; lufs -= 6.0f)
    {
        const auto y = lufsToY (lufs, plot);
        g.setColour (LF::gridLine);
        g.drawHorizontalLine ((int) y, plot.getX() + 1.0f, plot.getRight() - 1.0f);
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
        g.drawVerticalLine ((int) x, plot.getY() + 1.0f, plot.getBottom() - 1.0f);
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
        g.setFont (LF::getMonoFont (9.0f, true));
        g.drawText (label + " " + juce::String (lufs, 1), (int) plot.getX() + 6, (int) y - 13, 120, 12,
                    juce::Justification::centredLeft);
    };

    drawGuide (referenceIntegrated, LF::accent, "REF INT");
    drawGuide (mix.getIntegratedLufs(), LF::inkSoft, "MIX INT");

    // Momentary traces faint, short-term traces solid with a soft glow.
    g.setColour (LF::accent.withAlpha (0.25f));
    g.strokePath (buildTrace (refData, refCount, plot, false), juce::PathStrokeType (1.0f));
    g.setColour (LF::inkSoft.withAlpha (0.25f));
    g.strokePath (buildTrace (mixData, mixCount, plot, false), juce::PathStrokeType (1.0f));

    auto refShort = buildTrace (refData, refCount, plot, true);
    auto mixShort = buildTrace (mixData, mixCount, plot, true);

    LF::strokeWithGlow (g, refShort, LF::accent, 1.6f, 3.0f, 0.20f);
    LF::strokeWithGlow (g, mixShort, LF::inkSoft, 1.6f, 3.0f, 0.16f);

    if (mixShort.isEmpty() && refShort.isEmpty())
        LF::paintEmptyState (g, plot, juce::String::fromUTF8 ("\xe2\x96\xb6"),
                             mixCount == 0 ? "Press play in your DAW" : "Short-term loudness appears after 3 s of audio");
}
