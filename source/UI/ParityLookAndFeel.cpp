#include "ParityLookAndFeel.h"

const juce::Colour ParityLookAndFeel::cream     { 0xffefeae0 };
const juce::Colour ParityLookAndFeel::ink       { 0xff26221c };
const juce::Colour ParityLookAndFeel::inkFaint  { 0xff77705f };
const juce::Colour ParityLookAndFeel::panelLine { 0xffc8c0b0 };
const juce::Colour ParityLookAndFeel::accent    { 0xffd96c2c };
const juce::Colour ParityLookAndFeel::deltaGood { 0xff557c4f };
const juce::Colour ParityLookAndFeel::deltaWarn { 0xffb8860b };
const juce::Colour ParityLookAndFeel::deltaBad  { 0xffa93f2e };

ParityLookAndFeel::ParityLookAndFeel()
{
    setColour (juce::ResizableWindow::backgroundColourId, cream);
    setColour (juce::Label::textColourId, ink);
    setColour (juce::TextButton::buttonColourId, cream);
    setColour (juce::TextButton::buttonOnColourId, ink);
    setColour (juce::TextButton::textColourOffId, ink);
    setColour (juce::TextButton::textColourOnId, cream);
    setColour (juce::ComboBox::outlineColourId, ink);
    setColour (juce::AlertWindow::backgroundColourId, cream);
    setColour (juce::AlertWindow::textColourId, ink);
}

//==============================================================================
juce::Font ParityLookAndFeel::getMonoFont (float height)
{
    return { juce::FontOptions { juce::Font::getDefaultMonospacedFontName(), height, juce::Font::plain } };
}

juce::Font ParityLookAndFeel::getLabelFont (float height)
{
    return juce::Font (juce::FontOptions { height, juce::Font::plain }).withExtraKerningFactor (0.08f);
}

juce::Colour ParityLookAndFeel::colourForDelta (float deltaLu)
{
    const auto magnitude = std::abs (deltaLu);

    if (magnitude <= 1.0f)
        return deltaGood;

    if (magnitude <= 3.0f)
        return deltaWarn;

    return deltaBad;
}

//==============================================================================
void ParityLookAndFeel::drawButtonBackground (juce::Graphics& g, juce::Button& button,
                                              const juce::Colour& backgroundColour,
                                              bool shouldDrawButtonAsHighlighted,
                                              bool shouldDrawButtonAsDown)
{
    juce::ignoreUnused (backgroundColour);

    auto bounds = button.getLocalBounds().toFloat().reduced (0.5f);
    constexpr float cornerRadius = 2.0f;

    auto fill = button.getToggleState() || shouldDrawButtonAsDown ? ink : cream;

    if (shouldDrawButtonAsHighlighted && ! shouldDrawButtonAsDown && ! button.getToggleState())
        fill = cream.darker (0.05f);

    g.setColour (fill);
    g.fillRoundedRectangle (bounds, cornerRadius);

    g.setColour (ink);
    g.drawRoundedRectangle (bounds, cornerRadius, 1.0f);
}

juce::Font ParityLookAndFeel::getTextButtonFont (juce::TextButton&, int buttonHeight)
{
    return getLabelFont (juce::jmin (15.0f, (float) buttonHeight * 0.55f));
}
