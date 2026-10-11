#pragma once

#include <juce_gui_extra/juce_gui_extra.h>

//==============================================================================
/**
    Textless power toggle in the style of commercial plugin bypass buttons:
    a recessed tray with a glowing LED. Derived from ToggleButton so it
    attaches to an APVTS bool parameter directly. Click toggles.
*/
class PowerButton final : public juce::ToggleButton
{
public:
    explicit PowerButton (const juce::String& tooltip = {})
    {
        setClickingTogglesState (true);
        setTooltip (tooltip);
        setMouseCursor (juce::MouseCursor::PointingHandCursor);
    }

    void paintButton (juce::Graphics& g, bool highlighted, bool down) override;

private:
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PowerButton)
};
