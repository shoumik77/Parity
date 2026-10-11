#include "MasterHistoryView.h"
#include "ParityLookAndFeel.h"

MasterHistoryView::MasterHistoryView (const MasterChain& chain, Mode viewMode)
    : masterChain (chain), mode (viewMode),
      minDb (viewMode == Mode::compact ? -18.0f : -30.0f),
      maxDb (viewMode == Mode::compact ? 3.0f : 6.0f),
      slotsToShow (viewMode == Mode::compact ? 300 : MasterChain::historyLength)
{
    setOpaque (false);
    setInterceptsMouseClicks (viewMode == Mode::detailed, false);
    startTimerHz (30);
}

void MasterHistoryView::timerCallback()
{
    numSlots = masterChain.getHistory().read (snapshot);
    repaint();
}

float MasterHistoryView::dbToY (float db, juce::Rectangle<float> plot) const noexcept
{
    return juce::jmap (juce::jlimit (minDb, maxDb, db), minDb, maxDb, plot.getBottom(), plot.getY());
}

void MasterHistoryView::paint (juce::Graphics& g)
{
    using LF = ParityLookAndFeel;

    const auto detailed = mode == Mode::detailed;
    auto bounds = getLocalBounds().toFloat();

    if (detailed)
    {
        bounds.removeFromRight (40.0f);
        bounds.removeFromBottom (20.0f);
        bounds.removeFromTop (22.0f); // readout row
    }

    const auto plot = bounds;

    // Background + grid.
    g.setColour (detailed ? LF::background : LF::control);
    g.fillRoundedRectangle (plot, detailed ? 0.0f : 4.0f);

    g.setFont (LF::getFont (10.0f));

    for (float db = 0.0f; db >= minDb; db -= 6.0f)
    {
        const auto y = dbToY (db, plot);
        g.setColour (db == 0.0f ? LF::line.brighter (0.25f) : LF::gridLine);
        g.drawHorizontalLine ((int) y, plot.getX(), plot.getRight());

        if (detailed)
        {
            g.setColour (LF::inkDim);
            g.drawText (juce::String ((int) db), (int) plot.getRight() + 10, (int) y - 6, 30, 12,
                        juce::Justification::centredLeft);
        }
    }

    if (detailed)
    {
        g.setColour (LF::inkDim);
        g.drawText ("dBFS", (int) plot.getRight() + 10, (int) plot.getBottom() + 4, 30, 12,
                    juce::Justification::centredLeft);

        const auto seconds = (float) slotsToShow * 0.01f;

        for (int s = 0; s <= (int) seconds; ++s)
        {
            const auto x = plot.getRight() - plot.getWidth() * (float) s / seconds;
            g.setColour (LF::gridLine);
            g.drawVerticalLine ((int) x, plot.getY(), plot.getBottom());
            g.setColour (LF::inkFaint);
            g.drawText (s == 0 ? juce::String ("now") : "-" + juce::String (s) + " s",
                        juce::jlimit ((int) plot.getX(), (int) plot.getRight() - 40, (int) x - 20),
                        (int) plot.getBottom() + 4, 40, 12,
                        s == 0 ? juce::Justification::centredRight
                               : (int) seconds == s ? juce::Justification::centredLeft
                                                    : juce::Justification::centred);
        }
    }

    //==========================================================================
    const auto shown = juce::jmin (numSlots, slotsToShow);

    if (shown < 2)
    {
        g.setColour (LF::inkDim);
        g.setFont (LF::getFont (detailed ? 12.0f : 10.0f));
        g.drawText (detailed ? "Press play in your DAW to see what the clipper and limiter are doing"
                             : "no signal",
                    plot, juce::Justification::centred);
        return;
    }

    const auto first = numSlots - shown;
    const auto slotWidth = plot.getWidth() / (float) slotsToShow;

    auto xForIndex = [&] (int i) { return plot.getRight() - (float) (shown - i) * slotWidth; };

    auto toDb = [] (float linear) { return juce::Decibels::gainToDecibels (linear, -100.0f); };

    const auto latest = snapshot[(size_t) (numSlots - 1)];

    {
    // Clip the painting to the plot so highlights never spill into the axes.
    juce::Graphics::ScopedSaveState saved (g);
    g.reduceClipRegion (plot.toNearestInt());

    // 1) Input envelope as a filled area (what arrives at the clipper).
    juce::Path input;
    input.startNewSubPath (xForIndex (0), plot.getBottom());

    for (int i = 0; i < shown; ++i)
        input.lineTo (xForIndex (i), dbToY (toDb (snapshot[(size_t) (first + i)].inputPeak), plot));

    input.lineTo (xForIndex (shown - 1) + slotWidth, plot.getBottom());
    input.closeSubPath();

    g.setColour (LF::inkFaint.withAlpha (0.16f));
    g.fillPath (input);

    // 2) Per-slot highlights: clipped-away (red) and limited-away (amber) slices,
    //    plus threshold / ceiling lines that follow parameter changes over time.
    juce::Path thresholdLine, ceilingLine, outputLine;

    for (int i = 0; i < shown; ++i)
    {
        const auto& slot = snapshot[(size_t) (first + i)];
        const auto x0 = xForIndex (i);
        const auto x1 = x0 + slotWidth;

        const auto inDb = toDb (slot.inputPeak);
        const auto thresholdDb = toDb (slot.clipThreshold);
        const auto postClipDb = toDb (slot.postClipPeak);
        const auto ceilingDb = toDb (slot.ceiling);
        const auto outDb = toDb (slot.outputPeak);

        if (slot.clipActive && inDb > thresholdDb + 0.01f)
        {
            g.setColour (LF::warn.withAlpha (0.85f));
            g.fillRect (x0, dbToY (inDb, plot), juce::jmax (1.0f, slotWidth), dbToY (thresholdDb, plot) - dbToY (inDb, plot));
        }

        if (slot.limitActive && postClipDb > outDb + 0.01f)
        {
            g.setColour (LF::meterTop.withAlpha (0.75f));
            g.fillRect (x0, dbToY (postClipDb, plot), juce::jmax (1.0f, slotWidth), dbToY (outDb, plot) - dbToY (postClipDb, plot));
        }

        const auto tY = dbToY (thresholdDb, plot);
        const auto cY = dbToY (ceilingDb, plot);
        const auto oY = dbToY (outDb, plot);

        if (i == 0)
        {
            thresholdLine.startNewSubPath (x0, tY);
            ceilingLine.startNewSubPath (x0, cY);
            outputLine.startNewSubPath (x0, oY);
        }
        else
        {
            thresholdLine.lineTo (x0, tY);
            ceilingLine.lineTo (x0, cY);
            outputLine.lineTo (x0, oY);
        }

        thresholdLine.lineTo (x1, tY);
        ceilingLine.lineTo (x1, cY);
        outputLine.lineTo (x1, oY);
    }

    // Output envelope on top so the delivered level is always readable.
    g.setColour (LF::inkSoft.withAlpha (0.9f));
    g.strokePath (outputLine, juce::PathStrokeType (1.0f));

    if (latest.clipActive)
    {
        const float dashes[] = { 3.0f, 2.0f };
        juce::Path dashed;
        juce::PathStrokeType (1.0f).createDashedStroke (dashed, thresholdLine, dashes, 2);
        g.setColour (LF::warn);
        g.fillPath (dashed);
    }

    if (latest.limitActive)
    {
        g.setColour (LF::accent);
        g.strokePath (ceilingLine, juce::PathStrokeType (1.0f));
    }

    }

    //==========================================================================
    if (! detailed)
        return;

    auto readout = getLocalBounds().toFloat().removeFromTop (22.0f).withTrimmedBottom (6.0f);

    auto drawChip = [&] (juce::Colour colour, const juce::String& text, bool lineStyle)
    {
        g.setColour (colour);

        if (lineStyle)
            g.fillRect (readout.getX(), readout.getCentreY() - 1.0f, 14.0f, 2.0f);
        else
            g.fillRect (readout.getX(), readout.getY() + 2.0f, 10.0f, readout.getHeight() - 4.0f);

        readout.removeFromLeft (lineStyle ? 20.0f : 16.0f);
        g.setFont (LF::getFont (10.0f, juce::Font::bold));
        const auto width = (float) juce::GlyphArrangement::getStringWidthInt (g.getCurrentFont(), text) + 18.0f;
        g.drawText (text, readout.removeFromLeft (width), juce::Justification::centredLeft);
    };

    drawChip (LF::inkFaint.withAlpha (0.5f), "INPUT", false);
    drawChip (LF::warn, "CLIPPED", false);
    drawChip (LF::meterTop, "LIMITED", false);
    drawChip (LF::inkSoft, "OUTPUT", true);

    const auto clippedPercent = masterChain.getRecentClippedFraction() * 100.0f;
    const auto dot = juce::String::fromUTF8 ("  \xc2\xb7  ");
    juce::String status;

    if (latest.clipActive)
        status << "Clipping " << juce::String (clippedPercent, clippedPercent < 10.0f ? 2 : 1) << "% of samples"
               << dot << "clip GR " << juce::String (masterChain.getClipReductionDb(), 1) << " dB";
    else
        status << "Clipper bypassed";

    status << "      ";

    if (latest.limitActive)
        status << "Limiter GR " << juce::String (masterChain.getLimitReductionDb(), 1) << " dB"
               << dot << "ceiling " << juce::String (toDb (latest.ceiling), 1) << " dB";
    else
        status << "Limiter bypassed";

    g.setFont (LF::getFont (11.0f));
    g.setColour (LF::inkFaint);
    g.drawText (status, readout, juce::Justification::centredRight);
}
