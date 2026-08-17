#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

#include <functional>

//==============================================================================
/**
    Large two-segment MIX | REF switch. The active segment is filled
    (ink for MIX, signal orange for REF); the inactive segment stays
    cream with ink text.
*/
class ABSwitch final : public juce::Component
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

    bool referenceActive = false;
    bool referenceAvailable = false;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ABSwitch)
};
