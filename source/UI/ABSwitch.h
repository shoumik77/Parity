#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

#include <functional>

//==============================================================================
/**
    Segmented MIX | REF source switch (Figma "Source selection"): two 80 px
    rounded segments in a dark 4 px-padded tray. The active segment shows a
    check mark; REF fills signal orange with dark text, MIX stays a dark
    control with the pale active border.
*/
class ABSwitch final : public juce::Component,
                       public juce::SettableTooltipClient
{
public:
    ABSwitch();

    /** Called with true when REF becomes active, false for MIX. */
    std::function<void (bool)> onChange;

    void setReferenceActive (bool shouldBeRef, juce::NotificationType notification);
    bool isReferenceActive() const noexcept  { return referenceActive; }

    void setEnabledForReference (bool referenceAvailable);

    //==============================================================================
    void paint (juce::Graphics&) override;
    void mouseDown (const juce::MouseEvent&) override;
    bool keyPressed (const juce::KeyPress&) override;

private:
    void applyChange (bool shouldBeRef, juce::NotificationType notification);
    juce::Rectangle<float> segmentBounds (bool refSegment) const;

    bool referenceActive = false;
    bool referenceAvailable = false;

    std::unique_ptr<juce::Drawable> checkDark, checkLight;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ABSwitch)
};
