#include "ParityLookAndFeel.h"

const juce::Colour ParityLookAndFeel::background { 0xff0f0e0b };
const juce::Colour ParityLookAndFeel::panel      { 0xff151412 };
const juce::Colour ParityLookAndFeel::control    { 0xff1a1916 };
const juce::Colour ParityLookAndFeel::line       { 0xff2a2824 };
const juce::Colour ParityLookAndFeel::gridLine   { 0xff252521 };
const juce::Colour ParityLookAndFeel::ink        { 0xfff0ede4 };
const juce::Colour ParityLookAndFeel::inkSoft    { 0xffe9e5d9 };
const juce::Colour ParityLookAndFeel::inkFaint   { 0xff9b998f };
const juce::Colour ParityLookAndFeel::inkDim     { 0xff696960 };
const juce::Colour ParityLookAndFeel::inkGhost   { 0xff7a7870 };
const juce::Colour ParityLookAndFeel::inkMuted   { 0xff555450 };
const juce::Colour ParityLookAndFeel::accent     { 0xffd7804f };
const juce::Colour ParityLookAndFeel::accentDark { 0xff11110f };
const juce::Colour ParityLookAndFeel::warn       { 0xffe8756a };
const juce::Colour ParityLookAndFeel::meterHot   { 0xffff6b3d };
const juce::Colour ParityLookAndFeel::meterTop   { 0xfff0c080 };
const juce::Colour ParityLookAndFeel::meterMid   { 0xffe8a86a };
const juce::Colour ParityLookAndFeel::handleFill { 0xff3a3a35 };

ParityLookAndFeel::ParityLookAndFeel()
{
    setColour (juce::ResizableWindow::backgroundColourId, background);
    setColour (juce::Label::textColourId, inkSoft);
    setColour (juce::TextButton::buttonColourId, control);
    setColour (juce::TextButton::buttonOnColourId, control);
    setColour (juce::TextButton::textColourOffId, inkSoft);
    setColour (juce::TextButton::textColourOnId, ink);
    setColour (juce::ComboBox::outlineColourId, line);
    setColour (juce::AlertWindow::backgroundColourId, panel);
    setColour (juce::AlertWindow::textColourId, ink);
    setColour (juce::TooltipWindow::backgroundColourId, control);
    setColour (juce::TooltipWindow::textColourId, inkSoft);
    setColour (juce::TooltipWindow::outlineColourId, line);
}

//==============================================================================
juce::Font ParityLookAndFeel::getFont (float height, juce::Font::FontStyleFlags style)
{
    // The design uses Inter; fall back to the system sans when unavailable.
    return { juce::FontOptions { "Inter", height, style } };
}

juce::Font ParityLookAndFeel::getMonoFont (float height)
{
    return { juce::FontOptions { juce::Font::getDefaultMonospacedFontName(), height, juce::Font::plain } };
}

juce::Colour ParityLookAndFeel::colourForDelta (float deltaLu)
{
    return std::abs (deltaLu) > 3.0f ? accent : inkFaint;
}

std::unique_ptr<juce::Drawable> ParityLookAndFeel::loadIcon (const char* data, int size)
{
    return juce::Drawable::createFromImageData (data, (size_t) size);
}

//==============================================================================
void ParityLookAndFeel::drawButtonBackground (juce::Graphics& g, juce::Button& button,
                                              const juce::Colour& backgroundColour,
                                              bool shouldDrawButtonAsHighlighted,
                                              bool shouldDrawButtonAsDown)
{
    juce::ignoreUnused (backgroundColour);

    auto bounds = button.getLocalBounds().toFloat().reduced (0.5f);
    constexpr float cornerRadius = 6.0f;

    auto fill = control;

    if (shouldDrawButtonAsHighlighted || shouldDrawButtonAsDown)
        fill = fill.brighter (0.06f);

    g.setColour (fill);
    g.fillRoundedRectangle (bounds, cornerRadius);

    // Toggled controls get the pale active border from the design.
    g.setColour (button.getToggleState() ? inkFaint : line);
    g.drawRoundedRectangle (bounds, cornerRadius, 1.0f);
}

juce::Font ParityLookAndFeel::getTextButtonFont (juce::TextButton& button, int buttonHeight)
{
    juce::ignoreUnused (buttonHeight);
    return getFont (11.0f, button.getToggleState() ? juce::Font::bold : juce::Font::plain);
}
