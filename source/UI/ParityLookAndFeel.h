#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

//==============================================================================
/**
    Parity's visual theme: clean, minimal, light, with a retro
    test-instrument feel. Flat cream panels, warm near-black ink,
    a single signal-orange accent, 1 px rules, monospaced readouts.
*/
class ParityLookAndFeel final : public juce::LookAndFeel_V4
{
public:
    //==============================================================================
    // Palette
    static const juce::Colour cream;        // panel background
    static const juce::Colour ink;          // primary text / fills
    static const juce::Colour inkFaint;     // secondary text
    static const juce::Colour panelLine;    // rules and borders
    static const juce::Colour accent;       // signal orange
    static const juce::Colour deltaGood;    // within +-1 LU
    static const juce::Colour deltaWarn;    // within +-3 LU
    static const juce::Colour deltaBad;     // beyond +-3 LU

    ParityLookAndFeel();

    //==============================================================================
    static juce::Font getMonoFont (float height);
    static juce::Font getLabelFont (float height);

    /** Picks the delta color for a loudness difference in LU. */
    static juce::Colour colourForDelta (float deltaLu);

    //==============================================================================
    void drawButtonBackground (juce::Graphics&, juce::Button&,
                               const juce::Colour& backgroundColour,
                               bool shouldDrawButtonAsHighlighted,
                               bool shouldDrawButtonAsDown) override;

    juce::Font getTextButtonFont (juce::TextButton&, int buttonHeight) override;
};
