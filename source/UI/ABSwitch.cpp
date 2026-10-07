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
    repaint();

    if (notification != juce::dontSendNotification && onChange != nullptr)
        onChange (referenceActive);
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
    constexpr float cornerRadius = 6.0f;

    // Dark tray behind the segments.
    g.setColour (ParityLookAndFeel::background);
    g.fillRoundedRectangle (getLocalBounds().toFloat(), cornerRadius);

    auto drawSegment = [&] (bool refSegment)
    {
        const auto area = segmentBounds (refSegment);
        const auto active = refSegment == referenceActive;
        const auto accentFill = refSegment && active;

        g.setColour (accentFill ? ParityLookAndFeel::accent : ParityLookAndFeel::control);
        g.fillRoundedRectangle (area, cornerRadius);
        g.setColour (active && ! accentFill ? ParityLookAndFeel::inkFaint : ParityLookAndFeel::line);
        g.drawRoundedRectangle (area.reduced (0.5f), cornerRadius, 1.0f);

        auto textColour = accentFill ? ParityLookAndFeel::accentDark : ParityLookAndFeel::inkSoft;

        if (refSegment && ! referenceAvailable)
            textColour = ParityLookAndFeel::inkDim;

        const auto label = refSegment ? "REF" : "MIX";
        g.setFont (ParityLookAndFeel::getFont (11.0f, active ? juce::Font::bold : juce::Font::plain));

        auto* check = accentFill ? checkDark.get() : checkLight.get();

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

    drawSegment (false);
    drawSegment (true);
}

void ABSwitch::mouseDown (const juce::MouseEvent& e)
{
    applyChange (segmentBounds (true).contains (e.position), juce::sendNotification);
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
