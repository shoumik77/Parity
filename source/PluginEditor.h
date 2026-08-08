#pragma once

#include "PluginProcessor.h"

//==============================================================================
class ParityAudioProcessorEditor final : public juce::AudioProcessorEditor
{
public:
    explicit ParityAudioProcessorEditor (ParityAudioProcessor&);
    ~ParityAudioProcessorEditor() override;

    //==============================================================================
    void paint (juce::Graphics&) override;
    void resized() override;

private:
    void loadButtonClicked();
    void updateFileLabel();

    // This reference is provided as a quick way for your editor to
    // access the processor object that created it.
    ParityAudioProcessor& processorRef;

    juce::TextButton loadButton { "Load Reference..." };
    juce::Label fileLabel;
    juce::ToggleButton referenceToggle { "Reference" };

    std::unique_ptr<juce::FileChooser> fileChooser;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ParityAudioProcessorEditor)
};
