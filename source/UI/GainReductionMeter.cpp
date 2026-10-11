#include "GainReductionMeter.h"
#include "ParityLookAndFeel.h"

void GainReductionMeter::setReductionDb (float reductionDb)
{
    if (std::abs (reductionDb - reduction) > 0.05f)
    {
        reduction = juce::jmin (0.0f, reductionDb);
        repaint();
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

void GainReductionMeter::paint (juce::Graphics& g)
{
    using LF = ParityLookAndFeel;

    auto bounds = getLocalBounds().toFloat();
    auto readout = bounds.removeFromBottom (12.0f);
    bounds.removeFromBottom (3.0f);
    auto caption = bounds.removeFromTop (10.0f);
    bounds.removeFromTop (3.0f);

    auto bar = bounds.withSizeKeepingCentre (6.0f, bounds.getHeight());

    g.setColour (LF::control);
    g.fillRoundedRectangle (bar, 3.0f);

    const auto norm = juce::jlimit (0.0f, 1.0f, -reduction / rangeDb);

    if (norm > 0.0f)
    {
        g.setColour (active ? LF::warn : LF::inkFaint.withAlpha (0.4f));
        g.fillRoundedRectangle (bar.withHeight (juce::jmax (3.0f, bar.getHeight() * norm)), 3.0f);
    }

    g.setColour (LF::line);
    g.drawRoundedRectangle (bar.reduced (0.5f), 3.0f, 1.0f);

    // Scale ticks every quarter of the range.
    for (int tick = 1; tick < 4; ++tick)
    {
        const auto y = bar.getY() + bar.getHeight() * (float) tick / 4.0f;
        g.fillRect (bar.getRight() + 2.0f, y, 3.0f, 1.0f);
    }

    g.setFont (LF::getFont (8.0f, juce::Font::bold));
    g.setColour (LF::inkFaint);
    g.drawText ("GR", caption, juce::Justification::centred);

    g.setFont (LF::getFont (9.0f));
    g.setColour (reduction < -0.05f && active ? LF::warn : LF::inkDim);
    g.drawText (juce::String (reduction, 1), readout, juce::Justification::centred);
}
