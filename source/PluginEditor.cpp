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

    const char* rowNames[numLoudnessRows] = { "Momentary", "Short-term", "Integrated", "Peak" };

    for (int row = 0; row < numLoudnessRows; ++row)
    {
        rowLabels[(size_t) row].setText (rowNames[row], juce::dontSendNotification);
        addAndMakeVisible (rowLabels[(size_t) row]);

        for (auto* label : { &mixValueLabels[(size_t) row], &refValueLabels[(size_t) row], &deltaLabels[(size_t) row] })
        {
            label->setJustificationType (juce::Justification::centredRight);
            addAndMakeVisible (*label);
        }
    }

    mixHeader.setText ("Mix", juce::dontSendNotification);
    refHeader.setText ("Reference", juce::dontSendNotification);
    deltaHeader.setText ("Delta", juce::dontSendNotification);

    for (auto* header : { &mixHeader, &refHeader, &deltaHeader })
    {
        header->setJustificationType (juce::Justification::centredRight);
        addAndMakeVisible (*header);
    }

    updateLoudnessLabels();
    startTimerHz (10);

    setSize (440, 320);
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
    area.removeFromTop (12);

    const auto nameWidth = 100;
    const auto valueWidth = (area.getWidth() - nameWidth) / 3;

    auto headerRow = area.removeFromTop (24);
    headerRow.removeFromLeft (nameWidth);
    mixHeader.setBounds (headerRow.removeFromLeft (valueWidth));
    refHeader.setBounds (headerRow.removeFromLeft (valueWidth));
    deltaHeader.setBounds (headerRow.removeFromLeft (valueWidth));

    for (int row = 0; row < numLoudnessRows; ++row)
    {
        auto rowArea = area.removeFromTop (24);
        rowLabels[(size_t) row].setBounds (rowArea.removeFromLeft (nameWidth));
        mixValueLabels[(size_t) row].setBounds (rowArea.removeFromLeft (valueWidth));
        refValueLabels[(size_t) row].setBounds (rowArea.removeFromLeft (valueWidth));
        deltaLabels[(size_t) row].setBounds (rowArea.removeFromLeft (valueWidth));
    }
}

void ParityAudioProcessorEditor::timerCallback()
{
    updateLoudnessLabels();
}

void ParityAudioProcessorEditor::updateLoudnessLabels()
{
    auto format = [] (float value, const char* suffix) -> juce::String
    {
        if (value <= LoudnessAnalyzer::silenceLufs + 1.0f)
            return "-";

        return juce::String (value, 1) + " " + suffix;
    };

    auto formatDelta = [] (float mix, float ref) -> juce::String
    {
        if (mix <= LoudnessAnalyzer::silenceLufs + 1.0f || ref <= LoudnessAnalyzer::silenceLufs + 1.0f)
            return "-";

        return juce::String (mix - ref, 1) + " LU";
    };

    const auto& mix = processorRef.getMixLoudness();
    const auto& ref = processorRef.getReferenceLoudness();
    const auto fileStats = processorRef.getReferenceFileStats();

    const float mixValues[numLoudnessRows] = { mix.getMomentaryLufs(), mix.getShortTermLufs(),
                                               mix.getIntegratedLufs(), mix.getPeakDb() };

    // Integrated for the reference uses the offline full-file value.
    const float refValues[numLoudnessRows] = { ref.getMomentaryLufs(), ref.getShortTermLufs(),
                                               fileStats.integratedLufs, ref.getPeakDb() };

    const char* suffixes[numLoudnessRows] = { "LUFS", "LUFS", "LUFS", "dB" };

    for (int row = 0; row < numLoudnessRows; ++row)
    {
        mixValueLabels[(size_t) row].setText (format (mixValues[row], suffixes[row]), juce::dontSendNotification);
        refValueLabels[(size_t) row].setText (format (refValues[row], suffixes[row]), juce::dontSendNotification);
        deltaLabels[(size_t) row].setText (formatDelta (mixValues[row], refValues[row]), juce::dontSendNotification);
    }
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
