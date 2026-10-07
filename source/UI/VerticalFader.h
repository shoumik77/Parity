#pragma once

#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_gui_basics/juce_gui_basics.h>

//==============================================================================
/**
    Master-section vertical fader (Figma "Master processors"): a gradient
    level meter alongside a slim track with accent fill, tick marks, and a
    bordered handle. The component bounds cover only the fader itself;
    value/caption labels live in the editor below it.
*/
class VerticalFader final : public juce::Slider
{
public:
    /** showMeter: draw the gradient meter bar next to the track. */
    explicit VerticalFader (bool showMeter = true, int meterWidth = 10);

    /** Meter level as linear gain (0..1+); values over `overLevel` light the hot cap. */
    void setMeterLevel (float linearLevel);

    /** Level (linear) above which the hot cap at the top of the meter lights up. */
    void setOverLevel (float newOverLevel) noexcept  { overLevel = newOverLevel; }

    void paint (juce::Graphics&) override;

private:
    bool meterVisible;
    int meterBarWidth;
    float meterLevel = 0.0f;
    float overLevel = 1.0f;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (VerticalFader)
};
