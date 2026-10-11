#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

//==============================================================================
/**
    Narrow downward gain-reduction meter: fills from the top as reduction
    increases (0 dB at the top, `rangeDb` at the bottom), with the current
    reduction printed underneath. Reduction is given as a non-positive dB value.
*/
class GainReductionMeter final : public juce::Component,
                                 public juce::SettableTooltipClient
{
public:
    GainReductionMeter() { setInterceptsMouseClicks (false, false); }

    void setReductionDb (float reductionDb);
    void setRangeDb (float newRangeDb) noexcept  { rangeDb = newRangeDb; }
    void setActive (bool shouldBeActive);

    void paint (juce::Graphics&) override;

private:
    float reduction = 0.0f;
    float rangeDb = 12.0f;
    bool active = false;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (GainReductionMeter)
};
