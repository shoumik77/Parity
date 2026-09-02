#pragma once

#include "PluginProcessor.h"
#include "UI/ABSwitch.h"
#include "UI/ParityLookAndFeel.h"
#include "UI/SpectrumView.h"

//==============================================================================
class ParityAudioProcessorEditor final : public juce::AudioProcessorEditor,
                                         private juce::Timer
{
public:
    explicit ParityAudioProcessorEditor (ParityAudioProcessor&);
    ~ParityAudioProcessorEditor() override;

    //==============================================================================
    void paint (juce::Graphics&) override;
    void resized() override;

private:
    void timerCallback() override;
    void loadButtonClicked();
    void updateFileLabel();
    void updateLoudnessLabels();
    void updateStereoLabels();

    // This reference is provided as a quick way for your editor to
    // access the processor object that created it.
    ParityAudioProcessor& processorRef;

    ParityLookAndFeel lookAndFeel;

    juce::Label titleLabel;
    juce::TextButton loadButton { "LOAD REFERENCE" };
    juce::Label fileLabel;
    ABSwitch abSwitch;

    juce::Label spectrumSectionLabel;
    SpectrumView spectrumView;
    juce::Label viewModeLabel, meterModeLabel;
    juce::TextButton overlayButton { "OVERLAY" }, differenceButton { "DIFF" };
    juce::TextButton realtimeButton { "LIVE" }, averageButton { "AVG" };

    juce::Label loudnessSectionLabel;

    static constexpr int numLoudnessRows = 4; // momentary, short-term, integrated, peak
    std::array<juce::Label, numLoudnessRows> rowLabels;
    std::array<juce::Label, numLoudnessRows> mixValueLabels, refValueLabels, deltaLabels;
    juce::Label mixHeader, refHeader, deltaHeader;

    juce::Label stereoSectionLabel;
    static constexpr int numStereoRows = 2; // correlation, width
    std::array<juce::Label, numStereoRows> stereoRowLabels;
    std::array<juce::Label, numStereoRows> stereoMixLabels, stereoRefLabels, stereoDeltaLabels;
    juce::Label stereoMixHeader, stereoRefHeader, stereoDeltaHeader;

    juce::TooltipWindow tooltipWindow { this };

    juce::Rectangle<int> tableArea;       // for painting row rules
    juce::Rectangle<int> stereoTableArea; // for painting row rules

    std::unique_ptr<juce::FileChooser> fileChooser;
    ParityAudioProcessor::ReferenceLoadStatus lastLoadStatus = ParityAudioProcessor::ReferenceLoadStatus::idle;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ParityAudioProcessorEditor)
};
