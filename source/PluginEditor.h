#pragma once

#include "PluginProcessor.h"

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

    // This reference is provided as a quick way for your editor to
    // access the processor object that created it.
    ParityAudioProcessor& processorRef;

    juce::TextButton loadButton { "Load Reference..." };
    juce::Label fileLabel;
    juce::ToggleButton referenceToggle { "Reference" };

    static constexpr int numLoudnessRows = 4; // momentary, short-term, integrated, peak
    std::array<juce::Label, numLoudnessRows> rowLabels;
    std::array<juce::Label, numLoudnessRows> mixValueLabels, refValueLabels, deltaLabels;
    juce::Label mixHeader, refHeader, deltaHeader;

    std::unique_ptr<juce::FileChooser> fileChooser;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ParityAudioProcessorEditor)
};
