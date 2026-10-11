#include "VerticalFader.h"
#include "ParityLookAndFeel.h"

VerticalFader::VerticalFader (bool showMeter, int meterWidth)
    : meterVisible (showMeter), meterBarWidth (meterWidth)
{
    setSliderStyle (juce::Slider::LinearVertical);
    setTextBoxStyle (juce::Slider::NoTextBox, true, 0, 0);
    setMouseCursor (juce::MouseCursor::PointingHandCursor);
}

void VerticalFader::setMeterLevel (float linearLevel)
{
    if (std::abs (linearLevel - meterLevel) > 0.002f)
    {
        meterLevel = linearLevel;
        repaint();
    }
}

void VerticalFader::setProcessingActive (bool shouldBeActive)
{
    if (processingActive != shouldBeActive)
    {
        processingActive = shouldBeActive;
        repaint();
    }
}

void VerticalFader::paint (juce::Graphics& g)
{
    using LF = ParityLookAndFeel;

    auto bounds = getLocalBounds().toFloat();
    const auto height = bounds.getHeight();

    // Layout: meter bar | 3 px gap | 4 px track centred in the remaining width.
    auto meterArea = meterVisible ? bounds.removeFromLeft ((float) meterBarWidth)
                                  : juce::Rectangle<float>();

    if (meterVisible)
        bounds.removeFromLeft (3.0f);

    const auto trackArea = juce::Rectangle<float> (bounds.getX() + bounds.getWidth() * 0.5f - 2.0f,
                                                   0.0f, 4.0f, height);

    const auto norm = (float) valueToProportionOfLength (getValue());
    const auto handleY = height * (1.0f - norm);

    //==========================================================================
    if (meterVisible)
    {
        g.setColour (LF::control);
        g.fillRoundedRectangle (meterArea, 3.0f);

        // Map the meter onto the slider's own dB range so it lines up with the handle.
        const auto levelDb = juce::Decibels::gainToDecibels (meterLevel, -100.0f);
        const auto minDb = (float) getMinimum();
        const auto maxDb = (float) getMaximum();
        const auto levelNorm = juce::jlimit (0.0f, 1.0f, (levelDb - minDb) / (maxDb - minDb));

        if (levelNorm > 0.0f)
        {
            const auto fillTop = height * (1.0f - levelNorm);
            auto fill = meterArea.withTop (fillTop);

            // Below the handle: the signal that passes through untouched.
            const auto passing = fill.withTop (juce::jmax (fillTop, handleY));

            juce::ColourGradient gradient (LF::accent.withAlpha (0.9f), fill.getX(), height,
                                           LF::meterTop.withAlpha (0.9f), fill.getX(), 0.0f, false);
            gradient.addColour (0.6, LF::meterMid.withAlpha (0.9f));
            g.setGradientFill (gradient);
            g.fillRect (passing);

            // Above the handle: the part being clipped / limited away.
            if (fillTop < handleY)
            {
                g.setColour (processingActive ? LF::warn : LF::inkFaint.withAlpha (0.45f));
                g.fillRect (fill.withBottom (handleY));
            }
        }

        // Hot cap when the input exceeds the top of the scale.
        if (levelDb > maxDb)
        {
            g.setColour (LF::meterHot);
            g.fillRoundedRectangle (meterArea.withHeight (4.0f), 2.0f);
        }

        g.setColour (LF::line);
        g.drawRoundedRectangle (meterArea.reduced (0.5f), 3.0f, 1.0f);
    }

    //==========================================================================
    // Tick marks to the right of the track.
    for (int tick = 1; tick <= 5; ++tick)
    {
        const auto y = height * (float) tick / 6.0f;
        const auto major = tick == 3;

        g.setColour (major ? LF::inkFaint.withAlpha (0.6f) : LF::line);
        g.fillRect (trackArea.getRight() + 3.0f, y, major ? 7.0f : 5.0f, 1.0f);
    }

    // Track with accent fill from the handle down.
    g.setColour (juce::Colour (0xff141412));
    g.fillRoundedRectangle (trackArea, 2.0f);

    g.setColour (LF::accent.withAlpha (isEnabled() ? 0.5f : 0.25f));
    g.fillRect (trackArea.withTop (handleY));

    g.setColour (LF::line);
    g.drawRoundedRectangle (trackArea.reduced (0.5f), 2.0f, 1.0f);

    // Threshold line extending across the meter so the cut-off point is explicit.
    if (meterVisible)
    {
        g.setColour (processingActive ? LF::warn.withAlpha (0.9f) : LF::inkFaint.withAlpha (0.5f));
        g.fillRect (meterArea.getX(), handleY - 0.5f, meterArea.getWidth(), 1.0f);
    }

    //==========================================================================
    // Handle: bordered block with three accent lines.
    const auto handle = juce::Rectangle<float> (18.0f, 22.0f)
                            .withCentre ({ trackArea.getCentreX(),
                                           juce::jlimit (11.0f, height - 11.0f, handleY) });

    g.setColour (LF::handleFill);
    g.fillRoundedRectangle (handle, 3.0f);
    g.setColour (isEnabled() ? LF::accent : LF::inkDim);
    g.drawRoundedRectangle (handle.reduced (0.5f), 3.0f, 1.0f);

    for (int lineIndex = 0; lineIndex < 3; ++lineIndex)
    {
        g.setColour (LF::accent.withAlpha (lineIndex == 1 ? 0.5f : 0.8f));
        g.fillRect (handle.getCentreX() - 4.0f,
                    handle.getCentreY() - 4.0f + (float) lineIndex * 4.0f, 8.0f, 1.0f);
    }
}
