#include "ParityLookAndFeel.h"

#include "BinaryData.h"

const juce::Colour ParityLookAndFeel::background { 0xff0f0e0b };
const juce::Colour ParityLookAndFeel::well       { 0xff0a0907 };
const juce::Colour ParityLookAndFeel::panel      { 0xff151412 };
const juce::Colour ParityLookAndFeel::control    { 0xff1a1916 };
const juce::Colour ParityLookAndFeel::controlHi  { 0xff23221e };
const juce::Colour ParityLookAndFeel::line       { 0xff2a2824 };
const juce::Colour ParityLookAndFeel::gridLine   { 0xff252521 };
const juce::Colour ParityLookAndFeel::gridMinor  { 0xff1a1a17 };
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

//==============================================================================
struct ParityLookAndFeel::Typefaces
{
    Typefaces()
    {
        auto load = [] (const char* data, int size)
        {
            return juce::Typeface::createSystemTypefaceFor (data, (size_t) size);
        };

        regular  = load (BinaryData::InterRegular_ttf,  BinaryData::InterRegular_ttfSize);
        medium   = load (BinaryData::InterMedium_ttf,   BinaryData::InterMedium_ttfSize);
        semiBold = load (BinaryData::InterSemiBold_ttf, BinaryData::InterSemiBold_ttfSize);
        bold     = load (BinaryData::InterBold_ttf,     BinaryData::InterBold_ttfSize);
        mono     = load (BinaryData::JetBrainsMonoRegular_ttf, BinaryData::JetBrainsMonoRegular_ttfSize);
        monoMed  = load (BinaryData::JetBrainsMonoMedium_ttf,  BinaryData::JetBrainsMonoMedium_ttfSize);
    }

    juce::Typeface::Ptr regular, medium, semiBold, bold, mono, monoMed;
};

const ParityLookAndFeel::Typefaces& ParityLookAndFeel::typefaces()
{
    static const Typefaces instance;
    return instance;
}

// Family names JUCE may hand us when a widget builds a font by name;
// getTypefaceForFont() maps them back onto the embedded files.
static const char* const interMediumName  = "Inter Medium";
static const char* const interSemiName    = "Inter SemiBold";
static const char* const monoFamily       = "JetBrains Mono";
static const char* const monoMediumName   = "JetBrains Mono Medium";

ParityLookAndFeel::ParityLookAndFeel()
{
    setDefaultSansSerifTypeface (typefaces().regular);

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
    setColour (juce::TextEditor::textColourId, ink);
    setColour (juce::TextEditor::highlightColourId, accent.withAlpha (0.35f));
    setColour (juce::TextEditor::highlightedTextColourId, ink);
    setColour (juce::CaretComponent::caretColourId, accent);
    setColour (juce::Label::textWhenEditingColourId, ink);
}

//==============================================================================
juce::Typeface::Ptr ParityLookAndFeel::getTypefaceForFont (const juce::Font& font)
{
    const auto& tf = typefaces();
    const auto name = font.getTypefaceName();

    if (name == monoMediumName)  return tf.monoMed;
    if (name == monoFamily)      return font.isBold() ? tf.monoMed : tf.mono;
    if (name == interMediumName) return tf.medium;
    if (name == interSemiName)   return tf.semiBold;

    // Anything else (including JUCE's default sans) gets Inter.
    return font.isBold() ? tf.bold : tf.regular;
}

// Fonts are built straight from the embedded typefaces (rather than by name)
// so they resolve correctly even when this isn't the default LookAndFeel.
static juce::Font fontFor (const juce::Typeface::Ptr& typeface, float height)
{
    return juce::Font (juce::FontOptions { typeface }.withHeight (height));
}

juce::Font ParityLookAndFeel::getFont (float height, juce::Font::FontStyleFlags style)
{
    const auto& tf = typefaces();
    return fontFor ((style & juce::Font::bold) != 0 ? tf.bold : tf.regular, height);
}

juce::Font ParityLookAndFeel::getMediumFont (float height)
{
    return fontFor (typefaces().medium, height);
}

juce::Font ParityLookAndFeel::getSemiBoldFont (float height)
{
    return fontFor (typefaces().semiBold, height);
}

juce::Font ParityLookAndFeel::getMonoFont (float height, bool medium)
{
    return fontFor (medium ? typefaces().monoMed : typefaces().mono, height);
}

juce::Font ParityLookAndFeel::getLabelFont (float height)
{
    return getSemiBoldFont (height).withExtraKerningFactor (0.08f);
}

//==============================================================================
void ParityLookAndFeel::paintBackground (juce::Graphics& g, juce::Rectangle<int> area)
{
    // Barely-there warm vignette: lifts the centre slightly above pure flat.
    juce::ColourGradient grad (background.brighter (0.035f),
                               area.getCentreX(), area.getY(),
                               background.darker (0.15f),
                               area.getCentreX(), (float) area.getBottom(), false);
    g.setGradientFill (grad);
    g.fillRect (area);
}

void ParityLookAndFeel::paintRecessed (juce::Graphics& g, juce::Rectangle<float> bounds,
                                       float cornerRadius, juce::Colour fill)
{
    g.setColour (fill);
    g.fillRoundedRectangle (bounds, cornerRadius);

    // Inner shadow along the top edge, faint light catching the bottom lip.
    {
        juce::Graphics::ScopedSaveState state (g);
        juce::Path clip;
        clip.addRoundedRectangle (bounds, cornerRadius);
        g.reduceClipRegion (clip);

        juce::ColourGradient shade (juce::Colours::black.withAlpha (0.45f), 0.0f, bounds.getY(),
                                    juce::Colours::transparentBlack, 0.0f, bounds.getY() + 6.0f, false);
        g.setGradientFill (shade);
        g.fillRect (bounds.withHeight (6.0f));

        g.setColour (juce::Colours::white.withAlpha (0.035f));
        g.fillRect (bounds.getX(), bounds.getBottom() - 1.0f, bounds.getWidth(), 1.0f);
    }

    g.setColour (line.withAlpha (0.9f));
    g.drawRoundedRectangle (bounds.reduced (0.5f), cornerRadius, 1.0f);
}

void ParityLookAndFeel::paintRaised (juce::Graphics& g, juce::Rectangle<float> bounds,
                                     float cornerRadius, bool hovered, bool pressed, bool active)
{
    auto top = controlHi, bottom = control;

    if (hovered)
    {
        top = top.brighter (0.10f);
        bottom = bottom.brighter (0.08f);
    }

    if (pressed)
        std::swap (top, bottom);

    juce::ColourGradient fill (top, 0.0f, bounds.getY(), bottom, 0.0f, bounds.getBottom(), false);
    g.setGradientFill (fill);
    g.fillRoundedRectangle (bounds, cornerRadius);

    // Drop edge under the control.
    g.setColour (juce::Colours::black.withAlpha (0.45f));
    g.drawRoundedRectangle (bounds.translated (0.0f, 1.0f).reduced (0.5f), cornerRadius, 1.0f);

    // Top highlight.
    if (! pressed)
    {
        juce::Graphics::ScopedSaveState state (g);
        juce::Path clip;
        clip.addRoundedRectangle (bounds.reduced (1.0f), cornerRadius - 1.0f);
        g.reduceClipRegion (clip);
        g.setColour (juce::Colours::white.withAlpha (0.07f));
        g.fillRect (bounds.getX() + 1.0f, bounds.getY() + 1.0f, bounds.getWidth() - 2.0f, 1.0f);
    }

    g.setColour (active ? inkFaint.withAlpha (0.8f) : line);
    g.drawRoundedRectangle (bounds.reduced (0.5f), cornerRadius, 1.0f);
}

void ParityLookAndFeel::paintCard (juce::Graphics& g, juce::Rectangle<float> bounds, float cornerRadius)
{
    juce::Path shape;
    shape.addRoundedRectangle (bounds, cornerRadius);

    juce::DropShadow (juce::Colours::black.withAlpha (0.5f), 18, { 0, 6 }).drawForPath (g, shape);

    g.setColour (panel);
    g.fillPath (shape);

    {
        juce::Graphics::ScopedSaveState state (g);
        g.reduceClipRegion (shape);
        g.setColour (juce::Colours::white.withAlpha (0.045f));
        g.fillRect (bounds.getX(), bounds.getY(), bounds.getWidth(), 1.0f);
    }

    g.setColour (line);
    g.drawRoundedRectangle (bounds.reduced (0.5f), cornerRadius, 1.0f);
}

void ParityLookAndFeel::paintRule (juce::Graphics& g, juce::Rectangle<float> r, bool vertical)
{
    g.setColour (juce::Colours::black.withAlpha (0.55f));
    g.fillRect (r);
    g.setColour (juce::Colours::white.withAlpha (0.04f));
    g.fillRect (vertical ? r.translated (1.0f, 0.0f) : r.translated (0.0f, 1.0f));
}

void ParityLookAndFeel::paintLed (juce::Graphics& g, juce::Point<float> centre, float radius,
                                  juce::Colour colour, bool lit)
{
    if (lit)
    {
        juce::ColourGradient glow (colour.withAlpha (0.55f), centre.x, centre.y,
                                   colour.withAlpha (0.0f), centre.x + radius * 3.2f, centre.y, true);
        g.setGradientFill (glow);
        g.fillEllipse (juce::Rectangle<float> (radius * 6.4f, radius * 6.4f).withCentre (centre));
    }

    g.setColour (lit ? colour : juce::Colour (0xff2b2a26));
    g.fillEllipse (juce::Rectangle<float> (radius * 2.0f, radius * 2.0f).withCentre (centre));

    g.setColour (lit ? juce::Colours::white.withAlpha (0.5f) : juce::Colours::black.withAlpha (0.4f));
    g.drawEllipse (juce::Rectangle<float> (radius * 2.0f, radius * 2.0f).withCentre (centre).reduced (0.5f), 1.0f);
}

void ParityLookAndFeel::strokeWithGlow (juce::Graphics& g, const juce::Path& path, juce::Colour colour,
                                        float strokeWidth, float glowWidth, float glowAlpha)
{
    g.setColour (colour.withAlpha (glowAlpha * 0.5f));
    g.strokePath (path, juce::PathStrokeType (strokeWidth + glowWidth * 2.0f,
                                              juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    g.setColour (colour.withAlpha (glowAlpha));
    g.strokePath (path, juce::PathStrokeType (strokeWidth + glowWidth,
                                              juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    g.setColour (colour);
    g.strokePath (path, juce::PathStrokeType (strokeWidth, juce::PathStrokeType::curved,
                                              juce::PathStrokeType::rounded));
}

void ParityLookAndFeel::paintEmptyState (juce::Graphics& g, juce::Rectangle<float> area,
                                         const juce::String& glyph, const juce::String& message)
{
    const auto font = getMediumFont (12.0f);
    const auto glyphFont = getMonoFont (12.0f, true);
    const auto textWidth = (float) juce::GlyphArrangement::getStringWidthInt (font, message);
    const auto pill = juce::Rectangle<float> (textWidth + 56.0f, 32.0f).withCentre (area.getCentre());

    juce::Path shape;
    shape.addRoundedRectangle (pill, 16.0f);
    juce::DropShadow (juce::Colours::black.withAlpha (0.4f), 10, { 0, 3 }).drawForPath (g, shape);

    g.setColour (control);
    g.fillPath (shape);
    g.setColour (line);
    g.drawRoundedRectangle (pill.reduced (0.5f), 16.0f, 1.0f);

    auto content = pill.reduced (16.0f, 0.0f);
    g.setFont (glyphFont);
    g.setColour (accent);
    g.drawText (glyph, content.removeFromLeft (16.0f), juce::Justification::centred);
    content.removeFromLeft (8.0f);

    g.setFont (font);
    g.setColour (inkFaint);
    g.drawText (message, content, juce::Justification::centredLeft);
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

    auto bounds = button.getLocalBounds().toFloat();
    constexpr float cornerRadius = 6.0f;
    const auto on = button.getToggleState();

    if (on && button.getRadioGroupId() != 0)
    {
        // Selected tab: sunken with an accent underline.
        paintRecessed (g, bounds, cornerRadius, well.brighter (0.02f));
        g.setColour (accent);
        g.fillRoundedRectangle (bounds.getX() + 10.0f, bounds.getBottom() - 3.0f,
                                bounds.getWidth() - 20.0f, 2.0f, 1.0f);
        return;
    }

    paintRaised (g, bounds, cornerRadius, shouldDrawButtonAsHighlighted && button.isEnabled(),
                 shouldDrawButtonAsDown, on);
}

void ParityLookAndFeel::drawButtonText (juce::Graphics& g, juce::TextButton& button,
                                        bool shouldDrawButtonAsHighlighted, bool shouldDrawButtonAsDown)
{
    juce::ignoreUnused (shouldDrawButtonAsHighlighted);

    g.setFont (getTextButtonFont (button, button.getHeight()));

    auto colour = button.getToggleState() ? ink : inkSoft;

    if (! button.isEnabled())
        colour = inkDim;

    g.setColour (colour);

    auto area = button.getLocalBounds();

    if (shouldDrawButtonAsDown)
        area.translate (0, 1);

    // Leave room for a leading icon when the editor paints one (Load Reference).
    const auto leftInset = button.getProperties().getWithDefault ("iconInset", 0);
    g.drawText (button.getButtonText(), area.withTrimmedLeft ((int) leftInset), juce::Justification::centred);
}

juce::Font ParityLookAndFeel::getTextButtonFont (juce::TextButton& button, int buttonHeight)
{
    juce::ignoreUnused (buttonHeight);
    return button.getToggleState() ? getSemiBoldFont (11.0f) : getMediumFont (11.0f);
}

//==============================================================================
void ParityLookAndFeel::drawTooltip (juce::Graphics& g, const juce::String& text, int width, int height)
{
    auto bounds = juce::Rectangle<float> (0.0f, 0.0f, (float) width, (float) height);

    g.setColour (control);
    g.fillRoundedRectangle (bounds, 5.0f);
    g.setColour (line.brighter (0.2f));
    g.drawRoundedRectangle (bounds.reduced (0.5f), 5.0f, 1.0f);

    juce::AttributedString s;
    s.setJustification (juce::Justification::centredLeft);
    s.append (text, getFont (12.0f), inkSoft);

    juce::TextLayout layout;
    layout.createLayoutWithBalancedLineLengths (s, (float) width - 20.0f);
    layout.draw (g, bounds.reduced (10.0f, 6.0f));
}

juce::Rectangle<int> ParityLookAndFeel::getTooltipBounds (const juce::String& tipText,
                                                          juce::Point<int> screenPos,
                                                          juce::Rectangle<int> parentArea)
{
    juce::AttributedString s;
    s.append (tipText, getFont (12.0f), inkSoft);

    juce::TextLayout layout;
    layout.createLayoutWithBalancedLineLengths (s, 300.0f);

    const auto w = (int) (layout.getWidth() + 22.0f);
    const auto h = (int) (layout.getHeight() + 14.0f);

    return juce::Rectangle<int> (screenPos.x > parentArea.getCentreX() ? screenPos.x - (w + 12) : screenPos.x + 24,
                                 screenPos.y > parentArea.getCentreY() ? screenPos.y - (h + 6) : screenPos.y + 6,
                                 w, h).constrainedWithin (parentArea);
}

void ParityLookAndFeel::fillTextEditorBackground (juce::Graphics& g, int width, int height, juce::TextEditor&)
{
    g.setColour (well);
    g.fillRoundedRectangle (0.0f, 0.0f, (float) width, (float) height, 4.0f);
}

void ParityLookAndFeel::drawTextEditorOutline (juce::Graphics& g, int width, int height, juce::TextEditor& editor)
{
    g.setColour (editor.hasKeyboardFocus (true) ? accent : line);
    g.drawRoundedRectangle (juce::Rectangle<float> (0.0f, 0.0f, (float) width, (float) height).reduced (0.5f), 4.0f, 1.0f);
}
