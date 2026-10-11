#include "GainReductionMeter.h"
#include "ParityLookAndFeel.h"

namespace
{
    constexpr int frameRateHz = 30;
    constexpr float fallDbPerSecond = 16.0f;
    constexpr juce::uint32 peakHoldMs = 1500;
}

GainReductionMeter::GainReductionMeter()
{
    setInterceptsMouseClicks (false, false);
    startTimerHz (frameRateHz);
}

void GainReductionMeter::setReductionDb (float reductionDb)
{
    target = juce::jmin (0.0f, reductionDb);

    const auto now = juce::Time::getMillisecondCounter();

    if (target <= peakHold || now > peakHoldUntil)
    {
        peakHold = target;
        peakHoldUntil = now + peakHoldMs;
    }
}

void GainReductionMeter::setActive (bool shouldBeActive)
{
    if (active != shouldBeActive)
    {
        active = shouldBeActive;
        repaint();
    }
}

void GainReductionMeter::timerCallback()
{
    // Reduction grows instantly, releases at a fixed rate (more reduction = more negative).
    if (target < displayed)
        displayed = target;
    else
        displayed = juce::jmin (target, displayed + fallDbPerSecond / (float) frameRateHz);

    if (juce::Time::getMillisecondCounter() > peakHoldUntil)
        peakHold = juce::jmin (displayed, peakHold + fallDbPerSecond / (float) frameRateHz);

    repaint();
}

void GainReductionMeter::paint (juce::Graphics& g)
{
    using LF = ParityLookAndFeel;

    auto bounds = getLocalBounds().toFloat();
    auto readout = bounds.removeFromBottom (12.0f);
    bounds.removeFromBottom (4.0f);
    auto caption = bounds.removeFromTop (10.0f);
    bounds.removeFromTop (4.0f);

    auto bar = bounds.withSizeKeepingCentre (8.0f, bounds.getHeight());
    LF::paintRecessed (g, bar, 3.0f);

    const auto inner = bar.reduced (1.5f);
    const auto norm = juce::jlimit (0.0f, 1.0f, -displayed / rangeDb);

    if (norm > 0.0f)
    {
        const auto fill = inner.withHeight (juce::jmax (2.0f, inner.getHeight() * norm));
        juce::ColourGradient gradient (LF::warn.withAlpha (active ? 1.0f : 0.35f), 0.0f, fill.getY(),
                                       LF::accent.withAlpha (active ? 0.9f : 0.3f), 0.0f, fill.getBottom(), false);
        g.setGradientFill (gradient);
        g.fillRoundedRectangle (fill, 1.5f);

        g.setColour (LF::warn.withAlpha (active ? 0.35f : 0.1f));
        g.fillRect (inner.getX(), fill.getBottom() - 1.0f, inner.getWidth(), 2.0f);
    }

    if (peakHold < -0.05f)
    {
        const auto y = inner.getY() + inner.getHeight() * juce::jlimit (0.0f, 1.0f, -peakHold / rangeDb);
        g.setColour ((active ? LF::ink : LF::inkDim).withAlpha (0.9f));
        g.fillRect (inner.getX(), y - 0.5f, inner.getWidth(), 1.0f);
    }

    // Scale ticks every quarter of the range.
    g.setColour (LF::line.brighter (0.2f));

    for (int tick = 1; tick < 4; ++tick)
    {
        const auto y = bar.getY() + bar.getHeight() * (float) tick / 4.0f;
        g.fillRect (bar.getRight() + 2.0f, y, 3.0f, 1.0f);
    }

    g.setFont (LF::getLabelFont (8.0f));
    g.setColour (LF::inkFaint);
    g.drawText ("GR", caption, juce::Justification::centred);

    g.setFont (LF::getMonoFont (9.0f, true));
    g.setColour (peakHold < -0.05f && active ? LF::warn : LF::inkDim);
    g.drawText (juce::String (peakHold, 1), readout, juce::Justification::centred);
}
