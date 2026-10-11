#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

//==============================================================================
/**
    Parity's visual theme (Figma "Parity · Comparison" design): near-black
    panels, warm off-white ink, a single signal-orange accent, hairline
    rules, rounded 6 px controls.

    Typography is bundled: Inter for labels, JetBrains Mono for every numeric
    readout (tabular digits, so values don't jitter). The static paint helpers
    give the whole UI one consistent depth language - recessed wells for
    plots/meters, raised surfaces for buttons/caps, a shadowed card for the
    master panel.
*/
class ParityLookAndFeel final : public juce::LookAndFeel_V4
{
public:
    //==============================================================================
    // Palette (hex values straight from the Figma design)
    static const juce::Colour background;   // #0f0e0b window / plot background
    static const juce::Colour well;         // #0a0907 recessed surfaces
    static const juce::Colour panel;        // #151412 strip / section background
    static const juce::Colour control;      // #1a1916 control background
    static const juce::Colour controlHi;    // #23221e raised control top edge
    static const juce::Colour line;         // #2a2824 borders, dividers, rules
    static const juce::Colour gridLine;     // #252521 plot grid
    static const juce::Colour gridMinor;    // #1a1a17 minor plot grid
    static const juce::Colour ink;          // #f0ede4 primary text
    static const juce::Colour inkSoft;      // #e9e5d9 secondary text / mix trace
    static const juce::Colour inkFaint;     // #9b998f muted text / active borders
    static const juce::Colour inkDim;       // #696960 axis captions
    static const juce::Colour inkGhost;     // #7a7870 delta header
    static const juce::Colour inkMuted;     // #555450 MASTER caption
    static const juce::Colour accent;       // #d7804f signal orange / reference
    static const juce::Colour accentDark;   // #11110f text on accent fills
    static const juce::Colour warn;         // #e8756a over-ceiling warning
    static const juce::Colour meterHot;     // #ff6b3d meter over segment
    static const juce::Colour meterTop;     // #f0c080 meter gradient top
    static const juce::Colour meterMid;     // #e8a86a meter gradient middle
    static const juce::Colour handleFill;   // #3a3a35 fader handle

    ParityLookAndFeel();

    //==============================================================================
    // Typography
    static juce::Font getFont (float height, juce::Font::FontStyleFlags style = juce::Font::plain);
    static juce::Font getMediumFont (float height);
    static juce::Font getSemiBoldFont (float height);
    static juce::Font getMonoFont (float height, bool medium = false);

    /** Uppercase micro-label: semibold Inter with letter-spacing. */
    static juce::Font getLabelFont (float height);

    juce::Typeface::Ptr getTypefaceForFont (const juce::Font&) override;

    //==============================================================================
    // Depth helpers
    /** Window background: near-black with a faint warm vignette. */
    static void paintBackground (juce::Graphics&, juce::Rectangle<int> area);

    /** Sunken surface: plots, meter wells, segmented trays. */
    static void paintRecessed (juce::Graphics&, juce::Rectangle<float> bounds, float cornerRadius,
                               juce::Colour fill = well);

    /** Raised surface: buttons, fader caps. */
    static void paintRaised (juce::Graphics&, juce::Rectangle<float> bounds, float cornerRadius,
                             bool hovered = false, bool pressed = false, bool active = false);

    /** Shadowed card: the master panel. */
    static void paintCard (juce::Graphics&, juce::Rectangle<float> bounds, float cornerRadius);

    /** Two-tone engraved hairline. */
    static void paintRule (juce::Graphics&, juce::Rectangle<float> line, bool vertical = false);

    /** Small LED dot with optional glow. */
    static void paintLed (juce::Graphics&, juce::Point<float> centre, float radius, juce::Colour colour, bool lit);

    /** Soft glow behind a path (two-pass stroke, cheap enough for 30 Hz). */
    static void strokeWithGlow (juce::Graphics&, const juce::Path&, juce::Colour colour,
                                float strokeWidth, float glowWidth = 4.0f, float glowAlpha = 0.22f);

    /** Centred empty-state pill with an icon glyph and message. */
    static void paintEmptyState (juce::Graphics&, juce::Rectangle<float> area,
                                 const juce::String& glyph, const juce::String& message);

    /** Mix-minus-reference delta colour: faint when small, accent when large,
        warn used separately for over-ceiling peaks. */
    static juce::Colour colourForDelta (float deltaLu);

    /** Loads one of the embedded icon SVGs by BinaryData name. */
    static std::unique_ptr<juce::Drawable> loadIcon (const char* data, int size);

    //==============================================================================
    void drawButtonBackground (juce::Graphics&, juce::Button&,
                               const juce::Colour& backgroundColour,
                               bool shouldDrawButtonAsHighlighted,
                               bool shouldDrawButtonAsDown) override;

    void drawButtonText (juce::Graphics&, juce::TextButton&,
                         bool shouldDrawButtonAsHighlighted,
                         bool shouldDrawButtonAsDown) override;

    juce::Font getTextButtonFont (juce::TextButton&, int buttonHeight) override;

    void drawTooltip (juce::Graphics&, const juce::String& text, int width, int height) override;
    juce::Rectangle<int> getTooltipBounds (const juce::String& tipText, juce::Point<int> screenPos,
                                           juce::Rectangle<int> parentArea) override;

    void fillTextEditorBackground (juce::Graphics&, int width, int height, juce::TextEditor&) override;
    void drawTextEditorOutline (juce::Graphics&, int width, int height, juce::TextEditor&) override;

private:
    struct Typefaces;
    static const Typefaces& typefaces();
};
