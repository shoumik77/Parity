#include "PowerButton.h"
#include "ParityLookAndFeel.h"

void PowerButton::paintButton (juce::Graphics& g, bool highlighted, bool down)
{
    using LF = ParityLookAndFeel;

    auto area = getLocalBounds().toFloat().reduced (0.5f);
    const auto on = getToggleState();

    LF::paintRecessed (g, area, area.getHeight() * 0.5f - 1.0f);

    if (highlighted && isEnabled())
    {
        g.setColour (juce::Colours::white.withAlpha (0.03f));
        g.fillRoundedRectangle (area, area.getHeight() * 0.5f - 1.0f);
    }

    if (down)
    {
        g.setColour (juce::Colours::black.withAlpha (0.15f));
        g.fillRoundedRectangle (area, area.getHeight() * 0.5f - 1.0f);
    }

    // LED centred in the pill.
    LF::paintLed (g, area.getCentre(), juce::jmin (4.0f, area.getHeight() * 0.22f),
                  on ? LF::accent : LF::inkMuted, on && isEnabled());

    // Subtle "I" power glyph to the right of the LED.
    const auto glyphX = area.getRight() - 12.0f;
    if (glyphX > area.getCentreX() + 6.0f && isEnabled())
    {
        g.setColour (on ? LF::accent.withAlpha (0.85f) : LF::inkMuted.withAlpha (0.7f));
        g.drawLine (glyphX, area.getCentreY() - 3.5f, glyphX, area.getCentreY() + 3.5f, 1.4f);
    }

    if (hasKeyboardFocus (true))
    {
        g.setColour (LF::accent.withAlpha (0.5f));
        g.drawRoundedRectangle (area, area.getHeight() * 0.5f - 1.0f, 1.0f);
    }
}
