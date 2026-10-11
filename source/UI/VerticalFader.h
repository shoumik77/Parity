#pragma once

#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_gui_basics/juce_gui_basics.h>

//==============================================================================
/**
    Master-section vertical fader (Figma "Master processors"): a level meter
    alongside a slim track with accent fill, tick marks, and a bordered handle.

    The meter is drawn on the *fader's own dB scale* (the slider range), so the
    handle position doubles as the threshold line: any meter level that rises
    above the handle is the part of the signal the processor is acting on, and
    is painted in the warning colour. The component bounds cover only the fader
    itself; value/caption labels live in the editor below it.
*/
class VerticalFader final : public juce::Slider
{
public:
    /** showMeter: draw the level meter bar next to the track. */
    explicit VerticalFader (bool showMeter = true, int meterWidth = 10);

    /** Meter level as linear gain (0..1+), typically the processor's *input* level. */
    void setMeterLevel (float linearLevel);

    /** When false (section bypassed) the overshoot is drawn muted rather than red. */
    void setProcessingActive (bool shouldBeActive);

    void paint (juce::Graphics&) override;

private:
    bool meterVisible;
    int meterBarWidth;
    float meterLevel = 0.0f;
    bool processingActive = false;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (VerticalFader)
};
