#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

//==============================================================================
/**
    Narrow downward gain-reduction meter: fills from the top as reduction
    increases (0 dB at the top, `rangeDb` at the bottom), with a peak-hold
    tick and the held reduction printed underneath. Ballistics are animated
    here (instant attack, fixed-rate release) so 10 Hz data looks continuous.
    Reduction is given as a non-positive dB value.
*/
class GainReductionMeter final : public juce::Component,
                                 public juce::SettableTooltipClient,
                                 private juce::Timer
{
public:
    GainReductionMeter();

    void setReductionDb (float reductionDb);
    void setRangeDb (float newRangeDb) noexcept  { rangeDb = newRangeDb; }
    void setActive (bool shouldBeActive);

    void paint (juce::Graphics&) override;

private:
    void timerCallback() override;

    float target = 0.0f, displayed = 0.0f, peakHold = 0.0f;
    juce::uint32 peakHoldUntil = 0;
    float rangeDb = 12.0f;
    bool active = false;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (GainReductionMeter)
};
