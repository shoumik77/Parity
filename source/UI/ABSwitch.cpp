#include "ABSwitch.h"
#include "ParityLookAndFeel.h"

ABSwitch::ABSwitch()
{
    setWantsKeyboardFocus (true);
    setMouseCursor (juce::MouseCursor::PointingHandCursor);
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

//==============================================================================
void ABSwitch::paint (juce::Graphics& g)
{
    auto bounds = getLocalBounds().toFloat().reduced (0.5f);
    constexpr float cornerRadius = 2.0f;

    const auto mixArea = bounds.removeFromLeft (bounds.getWidth() * 0.5f);
    const auto refArea = bounds;

    // Active segment fill.
    g.setColour (referenceActive ? ParityLookAndFeel::accent : ParityLookAndFeel::ink);

    {
        juce::Path activePath;
        activePath.addRoundedRectangle (referenceActive ? refArea : mixArea, cornerRadius);
        g.fillPath (activePath);
    }

    // Labels.
    g.setFont (ParityLookAndFeel::getLabelFont (juce::jmin (18.0f, (float) getHeight() * 0.36f)));

    g.setColour (referenceActive ? ParityLookAndFeel::ink : ParityLookAndFeel::cream);
    g.drawText ("MIX", mixArea, juce::Justification::centred);

    const auto refInk = referenceAvailable ? ParityLookAndFeel::ink : ParityLookAndFeel::inkFaint;
    g.setColour (referenceActive ? ParityLookAndFeel::cream : refInk);
    g.drawText ("REF", refArea, juce::Justification::centred);

    // Outline and centre divider.
    g.setColour (ParityLookAndFeel::ink);
    g.drawRoundedRectangle (getLocalBounds().toFloat().reduced (0.5f), cornerRadius, 1.0f);
    g.drawVerticalLine (getWidth() / 2, 1.0f, (float) getHeight() - 1.0f);
}

void ABSwitch::mouseDown (const juce::MouseEvent& e)
{
    applyChange (e.position.x >= (float) getWidth() * 0.5f, juce::sendNotification);
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
