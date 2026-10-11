#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

//==============================================================================
/**
    Numeric readout for a fader: a recessed chip showing the slider's value in
    JetBrains Mono with a small unit, and a caption underneath.

    Interaction mirrors what a hardware-style plugin expects:
      - vertical drag on the chip adjusts the slider (shift = fine)
      - click without dragging opens inline text entry
      - double-click resets to the slider's double-click value
      - mouse wheel adjusts
*/
class ValueReadout final : public juce::Component,
                           public juce::SettableTooltipClient,
                           private juce::Slider::Listener,
                           private juce::Label::Listener
{
public:
    ValueReadout (juce::Slider& sliderToControl, juce::String unit, juce::String caption,
                  int decimals = 1);
    ~ValueReadout() override;

    /** Dims the readout when the owning section is bypassed. */
    void setSectionActive (bool shouldBeActive);

    void paint (juce::Graphics&) override;
    void resized() override;

    void mouseEnter (const juce::MouseEvent&) override;
    void mouseExit (const juce::MouseEvent&) override;
    void mouseDown (const juce::MouseEvent&) override;
    void mouseDrag (const juce::MouseEvent&) override;
    void mouseUp (const juce::MouseEvent&) override;
    void mouseDoubleClick (const juce::MouseEvent&) override;
    void mouseWheelMove (const juce::MouseEvent&, const juce::MouseWheelDetails&) override;

private:
    void sliderValueChanged (juce::Slider*) override;
    void labelTextChanged (juce::Label*) override;
    void editorHidden (juce::Label*, juce::TextEditor&) override;

    juce::String formatValue() const;
    juce::Rectangle<float> chipBounds() const;

    juce::Slider& slider;
    const juce::String unit, caption;
    const int decimals;

    juce::Label editor; // only visible while editing
    bool hovered = false, dragging = false, sectionActive = true;
    double dragStartValue = 0.0;
    juce::Point<float> dragStart;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ValueReadout)
};
