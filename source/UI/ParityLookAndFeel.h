#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

//==============================================================================
/**
    Parity's visual theme (Figma "Parity · Comparison" design): near-black
    panels, warm off-white ink, a single signal-orange accent, hairline
    rules, rounded 6 px controls.
*/
class ParityLookAndFeel final : public juce::LookAndFeel_V4
{
public:
    //==============================================================================
    // Palette (hex values straight from the Figma design)
    static const juce::Colour background;   // #0f0e0b window / plot background
    static const juce::Colour panel;        // #151412 strip / section background
    static const juce::Colour control;      // #1a1916 control background
    static const juce::Colour line;         // #2a2824 borders, dividers, rules
    static const juce::Colour gridLine;     // #252521 plot grid
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
    static juce::Font getFont (float height, juce::Font::FontStyleFlags style = juce::Font::plain);
    static juce::Font getMonoFont (float height);

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

    juce::Font getTextButtonFont (juce::TextButton&, int buttonHeight) override;
};
