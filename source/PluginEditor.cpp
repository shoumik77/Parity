#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
ParityAudioProcessorEditor::ParityAudioProcessorEditor (ParityAudioProcessor& p)
    : AudioProcessorEditor (&p), processorRef (p)
{
    addAndMakeVisible (loadButton);
    loadButton.onClick = [this] { loadButtonClicked(); };

    addAndMakeVisible (fileLabel);
    fileLabel.setJustificationType (juce::Justification::centredLeft);
    updateFileLabel();

    addAndMakeVisible (referenceToggle);
    referenceToggle.setToggleState (processorRef.isReferenceActive(), juce::dontSendNotification);
    referenceToggle.onClick = [this]
    {
        processorRef.setReferenceActive (referenceToggle.getToggleState());
    };

    setSize (400, 150);
}

ParityAudioProcessorEditor::~ParityAudioProcessorEditor()
{
}

//==============================================================================
void ParityAudioProcessorEditor::paint (juce::Graphics& g)
{
    g.fillAll (getLookAndFeel().findColour (juce::ResizableWindow::backgroundColourId));
}

void ParityAudioProcessorEditor::resized()
{
    auto area = getLocalBounds().reduced (16);

    loadButton.setBounds (area.removeFromTop (32));
    area.removeFromTop (8);
    fileLabel.setBounds (area.removeFromTop (24));
    area.removeFromTop (8);
    referenceToggle.setBounds (area.removeFromTop (28));
}

void ParityAudioProcessorEditor::loadButtonClicked()
{
    fileChooser = std::make_unique<juce::FileChooser> (
        "Select a reference track",
        juce::File::getSpecialLocation (juce::File::userMusicDirectory),
        processorRef.getReferencePlayer().getWildcardPattern());

    const auto flags = juce::FileBrowserComponent::openMode
                     | juce::FileBrowserComponent::canSelectFiles;

    fileChooser->launchAsync (flags, [this] (const juce::FileChooser& chooser)
    {
        const auto file = chooser.getResult();

        if (file.existsAsFile())
        {
            if (! processorRef.loadReferenceFile (file))
                juce::AlertWindow::showMessageBoxAsync (juce::MessageBoxIconType::WarningIcon,
                                                        "Parity",
                                                        "Couldn't load the selected file.");
        }

        updateFileLabel();
    });
}

void ParityAudioProcessorEditor::updateFileLabel()
{
    const auto file = processorRef.getReferencePlayer().getFile();
    fileLabel.setText (file != juce::File() ? file.getFileName() : "No reference loaded",
                       juce::dontSendNotification);
}
