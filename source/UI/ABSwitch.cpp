#include "ABSwitch.h"
#include "ParityLookAndFeel.h"

#include "BinaryData.h"

ABSwitch::ABSwitch()
{
    setWantsKeyboardFocus (true);
    setMouseCursor (juce::MouseCursor::PointingHandCursor);

    checkDark = ParityLookAndFeel::loadIcon (BinaryData::checkdark_svg, BinaryData::checkdark_svgSize);
    checkLight = ParityLookAndFeel::loadIcon (BinaryData::checklight_svg, BinaryData::checklight_svgSize);
}

void ABSwitch::setReferenceActive (bool shouldBeRef, juce::NotificationType notification)
{
    applyChange (shouldBeRef, notification);
}

void ABSwitch::setEnabledForReference (bool available)
{
    if (referenceAvailable == available)
        return;

    referenceAvailable = available;

    if (! available && referenceActive)
        applyChange (false, juce::sendNotification);
    else
        repaint();
}

void ABSwitch::applyChange (bool shouldBeRef, juce::NotificationType notification)
{
    if (shouldBeRef && ! referenceAvailable)
        return;

    if (referenceActive == shouldBeRef)
        return;

    referenceActive = shouldBeRef;
    startTimerHz (60);

    if (notification != juce::dontSendNotification && onChange != nullptr)
        onChange (referenceActive);
}

void ABSwitch::timerCallback()
{
    const auto target = referenceActive ? 1.0f : 0.0f;
    slidePosition += (target - slidePosition) * 0.35f;

    if (std::abs (target - slidePosition) < 0.01f)
    {
        slidePosition = target;
        stopTimer();
    }

    repaint();
}

juce::Rectangle<float> ABSwitch::segmentBounds (bool refSegment) const
{
    auto tray = getLocalBounds().toFloat().reduced (4.0f);
    const auto segmentWidth = (tray.getWidth() - 4.0f) * 0.5f;

    return refSegment ? tray.removeFromRight (segmentWidth)
                      : tray.removeFromLeft (segmentWidth);
}

//==============================================================================
void ABSwitch::paint (juce::Graphics& g)
{
    using LF = ParityLookAndFeel;
    constexpr float cornerRadius = 6.0f;

    LF::paintRecessed (g, getLocalBounds().toFloat(), cornerRadius + 2.0f);

    const auto mixArea = segmentBounds (false);
    const auto refArea = segmentBounds (true);

    // Hover hint on the inactive segment.
    if (hoveredSegment >= 0 && (hoveredSegment == 1) != referenceActive
        && (hoveredSegment == 0 || referenceAvailable))
    {
        g.setColour (juce::Colours::white.withAlpha (0.03f));
        g.fillRoundedRectangle (hoveredSegment == 1 ? refArea : mixArea, cornerRadius);
    }

    // Sliding raised thumb; blends from pale (MIX) to accent (REF) as it moves.
    const auto thumb = mixArea.withX (juce::jmap (slidePosition, mixArea.getX(), refArea.getX()));
    const auto accentMix = slidePosition;

    {
        juce::Path shape;
        shape.addRoundedRectangle (thumb, cornerRadius);
        juce::DropShadow (juce::Colours::black.withAlpha (0.5f), 6, { 0, 2 }).drawForPath (g, shape);
    }

    if (accentMix > 0.01f)
    {
        juce::ColourGradient fill (LF::accent.brighter (0.08f), 0.0f, thumb.getY(),
                                   LF::accent.darker (0.08f), 0.0f, thumb.getBottom(), false);
        g.setGradientFill (fill);
        g.setOpacity (accentMix);
        g.fillRoundedRectangle (thumb, cornerRadius);
        g.setOpacity (1.0f);
    }

    if (accentMix < 0.99f)
    {
        g.saveState();
        g.setOpacity (1.0f - accentMix);
        LF::paintRaised (g, thumb, cornerRadius, false, false, true);
        g.restoreState();
    }
    else
    {
        g.setColour (juce::Colours::white.withAlpha (0.18f));
        g.fillRect (thumb.getX() + 1.0f, thumb.getY() + 1.0f, thumb.getWidth() - 2.0f, 1.0f);
        g.setColour (LF::accent.darker (0.3f));
        g.drawRoundedRectangle (thumb.reduced (0.5f), cornerRadius, 1.0f);
    }

    auto drawLabel = [&] (bool refSegment)
    {
        const auto area = refSegment ? refArea : mixArea;
        const auto active = refSegment == referenceActive;
        const auto onAccent = refSegment && active && accentMix > 0.5f;

        auto textColour = onAccent ? LF::accentDark : active ? LF::ink : LF::inkFaint;

        if (refSegment && ! referenceAvailable)
            textColour = LF::inkMuted;

        const auto label = refSegment ? "REF" : "MIX";
        g.setFont (active ? LF::getLabelFont (11.0f) : LF::getMediumFont (11.0f).withExtraKerningFactor (0.08f));

        auto* check = onAccent ? checkDark.get() : checkLight.get();

        if (active && check != nullptr)
        {
            const auto textWidth = juce::GlyphArrangement::getStringWidth (g.getCurrentFont(), label);
            const auto contentWidth = 12.0f + 8.0f + textWidth;
            const auto left = area.getCentreX() - contentWidth * 0.5f;
            const auto iconArea = juce::Rectangle<float> (left, area.getCentreY() - 6.0f, 12.0f, 12.0f);

            check->drawWithin (g, iconArea, juce::RectanglePlacement::centred, 1.0f);
            g.setColour (textColour);
            g.drawText (label, juce::Rectangle<float> (left + 20.0f, area.getY(), textWidth + 2.0f, area.getHeight()),
                        juce::Justification::centredLeft);
        }
        else
        {
            g.setColour (textColour);
            g.drawText (label, area, juce::Justification::centred);
        }
    };

    drawLabel (false);
    drawLabel (true);

    if (hasKeyboardFocus (true))
    {
        g.setColour (LF::accent.withAlpha (0.6f));
        g.drawRoundedRectangle (getLocalBounds().toFloat().reduced (0.5f), cornerRadius + 2.0f, 1.0f);
    }
}

void ABSwitch::mouseDown (const juce::MouseEvent& e)
{
    applyChange (segmentBounds (true).contains (e.position), juce::sendNotification);
}

void ABSwitch::mouseMove (const juce::MouseEvent& e)
{
    const auto segment = segmentBounds (true).contains (e.position) ? 1
                       : segmentBounds (false).contains (e.position) ? 0 : -1;

    if (segment != hoveredSegment)
    {
        hoveredSegment = segment;
        repaint();
    }
}

void ABSwitch::mouseExit (const juce::MouseEvent&)
{
    hoveredSegment = -1;
    repaint();
}

bool ABSwitch::keyPressed (const juce::KeyPress& key)
{
    if (key == juce::KeyPress::spaceKey || key == juce::KeyPress::returnKey)
    {
        applyChange (! referenceActive, juce::sendNotification);
        return true;
    }

    return false;
}
