#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

#include "../Audio/LoudnessAnalyzer.h"

//==============================================================================
/**
    Scrolling short-term loudness graph (Youlean-style) for the mix and the
    reference over the last 60 s, with the mix's integrated loudness and the
    reference's full-file integrated loudness as horizontal guide lines.
*/
class LoudnessHistoryView final : public juce::Component,
                                  public juce::SettableTooltipClient,
                                  private juce::Timer
{
public:
    LoudnessHistoryView (const LoudnessAnalyzer& mixAnalyzer,
                         const LoudnessAnalyzer& referenceAnalyzer);

    /** Full-file integrated loudness of the loaded reference (silenceLufs if none). */
    void setReferenceIntegratedLufs (float lufs) noexcept  { referenceIntegrated = lufs; }

    void paint (juce::Graphics&) override;

private:
    void timerCallback() override;

    float lufsToY (float lufs, juce::Rectangle<float> plot) const noexcept;
    juce::Path buildTrace (const std::array<LoudnessAnalyzer::HistorySample, LoudnessAnalyzer::historyLength>& data,
                           int count, juce::Rectangle<float> plot, bool shortTerm) const;

    const LoudnessAnalyzer& mix;
    const LoudnessAnalyzer& reference;

    std::array<LoudnessAnalyzer::HistorySample, LoudnessAnalyzer::historyLength> mixData {}, refData {};
    int mixCount = 0, refCount = 0;
    float referenceIntegrated = LoudnessAnalyzer::silenceLufs;

    static constexpr float minLufs = -40.0f;
    static constexpr float maxLufs = 0.0f;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (LoudnessHistoryView)
};
