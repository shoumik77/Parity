#pragma once

#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_gui_basics/juce_gui_basics.h>

//==============================================================================
/**
    Master-section vertical fader: a recessed level meter alongside a slim
    track with a raised cap, labelled dB ticks, and accent fill.

    The meter is drawn on the *fader's own dB scale* (the slider range), so the
    cap position doubles as the threshold line: any meter level that rises
    above the cap is the part of the signal the processor is acting on, and
    is painted in the warning colour. The meter animates (instant rise, timed
    fall) and keeps a 1.5 s peak-hold tick.

    Interaction: drag, mouse wheel, shift-drag for fine control, double-click
    to reset. Value readout/caption live in the editor (ValueReadout).
*/
class VerticalFader final : public juce::Slider,
                            private juce::Timer
{
public:
    /** showMeter: draw the level meter well next to the track. */
    explicit VerticalFader (bool showMeter = true, int meterWidth = 10);

    /** Meter level as linear gain (0..1+), typically the processor's *input* level. */
    void setMeterLevel (float linearLevel);

    /** When false (section bypassed) the overshoot is drawn muted rather than red. */
    void setProcessingActive (bool shouldBeActive);

    void paint (juce::Graphics&) override;
    void mouseEnter (const juce::MouseEvent&) override;
    void mouseExit (const juce::MouseEvent&) override;

private:
    void timerCallback() override;
    float dbToY (float db, float height) const noexcept;

    bool meterVisible;
    int meterBarWidth;

    float targetDb = -100.0f, displayedDb = -100.0f;
    float peakHoldDb = -100.0f;
    juce::uint32 peakHoldUntil = 0;
    bool processingActive = false, hovered = false;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (VerticalFader)
};
