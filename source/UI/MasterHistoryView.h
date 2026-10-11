#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

#include "../Audio/MasterChain.h"

//==============================================================================
/**
    Scrolling activity display for the master chain, in the spirit of
    StandardCLIP's waveform view: the input peak envelope drawn against the
    clip threshold and limiter ceiling, with the slices that get clipped or
    limited away highlighted.

    Two modes share one implementation:
      - compact:  small strip for the master panel (last ~3 s, no axes)
      - detailed: full tab view (whole history, dB/time axes, readouts)
*/
class MasterHistoryView final : public juce::Component,
                                public juce::SettableTooltipClient,
                                private juce::Timer
{
public:
    enum class Mode { compact, detailed };

    MasterHistoryView (const MasterChain& chain, Mode mode);

    void paint (juce::Graphics&) override;

private:
    void timerCallback() override;

    float dbToY (float db, juce::Rectangle<float> plot) const noexcept;

    const MasterChain& masterChain;
    const Mode mode;

    std::array<MasterChain::Slot, MasterChain::historyLength> snapshot {};
    int numSlots = 0;

    float minDb, maxDb;
    int slotsToShow;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MasterHistoryView)
};
