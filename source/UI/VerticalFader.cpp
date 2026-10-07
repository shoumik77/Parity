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

    //==========================================================================
    if (meterVisible)
    {
        g.setColour (LF::control);
        g.fillRoundedRectangle (meterArea, 3.0f);

        const auto levelDb = juce::Decibels::gainToDecibels (meterLevel, -60.0f);
        const auto levelNorm = juce::jlimit (0.0f, 1.0f, (levelDb + 60.0f) / 60.0f);

        if (levelNorm > 0.0f)
        {
            auto fill = meterArea.withTrimmedTop (height * (1.0f - levelNorm));

            juce::ColourGradient gradient (LF::accent.withAlpha (0.9f), fill.getX(), fill.getBottom(),
                                           LF::meterTop.withAlpha (0.9f), fill.getX(), 0.0f, false);
            gradient.addColour (0.6, LF::meterMid.withAlpha (0.9f));
            g.setGradientFill (gradient);
            g.fillRect (fill);
        }

        if (meterLevel > overLevel)
        {
            g.setColour (LF::meterHot.withAlpha (0.7f));
            g.fillRect (meterArea.removeFromTop (16.0f));
        }

        g.setColour (LF::line);
        g.drawRoundedRectangle (meterArea.reduced (0.5f).withHeight (height - 1.0f), 3.0f, 1.0f);
    }

    //==========================================================================
    // Tick marks to the right of the track.
    g.setColour (LF::line);

    for (int tick = 1; tick <= 5; ++tick)
    {
        const auto y = height * (float) tick / 6.0f;
        const auto major = tick == 3;

        g.setColour (major ? LF::inkFaint.withAlpha (0.6f) : LF::line);
        g.fillRect (trackArea.getRight() + 3.0f, y, major ? 7.0f : 5.0f, 1.0f);
    }

    // Track with accent fill from the handle down.
    const auto norm = (float) valueToProportionOfLength (getValue());
    const auto handleY = height * (1.0f - norm);

    g.setColour (juce::Colour (0xff141412));
    g.fillRoundedRectangle (trackArea, 2.0f);

    g.setColour (LF::accent.withAlpha (isEnabled() ? 0.5f : 0.25f));
    g.fillRect (trackArea.withTop (handleY));

    g.setColour (LF::line);
    g.drawRoundedRectangle (trackArea.reduced (0.5f), 2.0f, 1.0f);

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
