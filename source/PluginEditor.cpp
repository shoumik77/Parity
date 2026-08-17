#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
ParityAudioProcessorEditor::ParityAudioProcessorEditor (ParityAudioProcessor& p)
    : AudioProcessorEditor (&p), processorRef (p)
{
    setLookAndFeel (&lookAndFeel);

    titleLabel.setText ("PARITY", juce::dontSendNotification);
    titleLabel.setFont (ParityLookAndFeel::getLabelFont (22.0f).boldened());
    titleLabel.setJustificationType (juce::Justification::centredLeft);
    addAndMakeVisible (titleLabel);

    addAndMakeVisible (loadButton);
    loadButton.onClick = [this] { loadButtonClicked(); };

    fileLabel.setJustificationType (juce::Justification::centredRight);
    fileLabel.setFont (ParityLookAndFeel::getMonoFont (13.0f));
    fileLabel.setColour (juce::Label::textColourId, ParityLookAndFeel::inkFaint);
    addAndMakeVisible (fileLabel);
    updateFileLabel();

    abSwitch.setEnabledForReference (processorRef.getReferencePlayer().hasFileLoaded());
    abSwitch.setReferenceActive (processorRef.isReferenceActive(), juce::dontSendNotification);
    abSwitch.onChange = [this] (bool refActive)
    {
        processorRef.setReferenceActive (refActive);
    };
    addAndMakeVisible (abSwitch);

    loudnessSectionLabel.setText ("LOUDNESS", juce::dontSendNotification);
    loudnessSectionLabel.setFont (ParityLookAndFeel::getLabelFont (12.0f));
    loudnessSectionLabel.setColour (juce::Label::textColourId, ParityLookAndFeel::inkFaint);
    addAndMakeVisible (loudnessSectionLabel);

    const char* rowNames[numLoudnessRows] = { "Momentary", "Short-term", "Integrated", "Peak" };

    for (int row = 0; row < numLoudnessRows; ++row)
    {
        auto& name = rowLabels[(size_t) row];
        name.setText (rowNames[row], juce::dontSendNotification);
        name.setFont (ParityLookAndFeel::getLabelFont (14.0f));
        addAndMakeVisible (name);

        for (auto* label : { &mixValueLabels[(size_t) row], &refValueLabels[(size_t) row], &deltaLabels[(size_t) row] })
        {
            label->setJustificationType (juce::Justification::centredRight);
            label->setFont (ParityLookAndFeel::getMonoFont (14.0f));
            addAndMakeVisible (*label);
        }
    }

    mixHeader.setText ("MIX", juce::dontSendNotification);
    refHeader.setText ("REF", juce::dontSendNotification);
    deltaHeader.setText ("DELTA", juce::dontSendNotification);

    for (auto* header : { &mixHeader, &refHeader, &deltaHeader })
    {
        header->setJustificationType (juce::Justification::centredRight);
        header->setFont (ParityLookAndFeel::getLabelFont (12.0f));
        header->setColour (juce::Label::textColourId, ParityLookAndFeel::inkFaint);
        addAndMakeVisible (*header);
    }

    updateLoudnessLabels();
    startTimerHz (10);

    setResizable (true, true);
    setResizeLimits (420, 340, 880, 680);
    setSize (460, 380);
}

ParityAudioProcessorEditor::~ParityAudioProcessorEditor()
{
    setLookAndFeel (nullptr);
}

//==============================================================================
void ParityAudioProcessorEditor::paint (juce::Graphics& g)
{
    g.fillAll (ParityLookAndFeel::cream);

    g.setColour (ParityLookAndFeel::panelLine);

    // Rule under the header strip.
    const auto headerBottom = titleLabel.getBottom() + 10;
    g.drawHorizontalLine (headerBottom, 16.0f, (float) getWidth() - 16.0f);

    // Rules between table rows.
    if (! tableArea.isEmpty())
    {
        const auto rowHeight = tableArea.getHeight() / (numLoudnessRows + 1);

        for (int row = 0; row <= numLoudnessRows; ++row)
        {
            const auto y = tableArea.getY() + rowHeight * (row + 1);
            g.drawHorizontalLine (y, (float) tableArea.getX(), (float) tableArea.getRight());
        }
    }
}

void ParityAudioProcessorEditor::resized()
{
    auto area = getLocalBounds().reduced (16);

    // Header strip: wordmark left, load button + file name right.
    auto header = area.removeFromTop (32);
    titleLabel.setBounds (header.removeFromLeft (110));
    loadButton.setBounds (header.removeFromRight (150).reduced (0, 2));
    header.removeFromRight (10);
    fileLabel.setBounds (header);
    area.removeFromTop (20);

    // The centerpiece A/B switch.
    abSwitch.setBounds (area.removeFromTop (56));
    area.removeFromTop (20);

    loudnessSectionLabel.setBounds (area.removeFromTop (18));
    area.removeFromTop (6);

    // Loudness table.
    tableArea = area;
    const auto rowHeight = area.getHeight() / (numLoudnessRows + 1);
    const auto nameWidth = juce::jmax (100, area.getWidth() / 4);
    const auto valueWidth = (area.getWidth() - nameWidth) / 3;

    auto headerRow = area.removeFromTop (rowHeight);
    headerRow.removeFromLeft (nameWidth);
    mixHeader.setBounds (headerRow.removeFromLeft (valueWidth));
    refHeader.setBounds (headerRow.removeFromLeft (valueWidth));
    deltaHeader.setBounds (headerRow.removeFromLeft (valueWidth));

    for (int row = 0; row < numLoudnessRows; ++row)
    {
        auto rowArea = area.removeFromTop (rowHeight);
        rowLabels[(size_t) row].setBounds (rowArea.removeFromLeft (nameWidth));
        mixValueLabels[(size_t) row].setBounds (rowArea.removeFromLeft (valueWidth));
        refValueLabels[(size_t) row].setBounds (rowArea.removeFromLeft (valueWidth));
        deltaLabels[(size_t) row].setBounds (rowArea.removeFromLeft (valueWidth));
    }
}

void ParityAudioProcessorEditor::timerCallback()
{
    abSwitch.setEnabledForReference (processorRef.getReferencePlayer().hasFileLoaded());
    abSwitch.setReferenceActive (processorRef.isReferenceActive(), juce::dontSendNotification);
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

        auto& delta = deltaLabels[(size_t) row];

        if (mixValues[row] <= LoudnessAnalyzer::silenceLufs + 1.0f
         || refValues[row] <= LoudnessAnalyzer::silenceLufs + 1.0f)
        {
            delta.setText ("-", juce::dontSendNotification);
            delta.setColour (juce::Label::textColourId, ParityLookAndFeel::inkFaint);
        }
        else
        {
            const auto deltaValue = mixValues[row] - refValues[row];
            delta.setText (juce::String (deltaValue, 1) + " LU", juce::dontSendNotification);
            delta.setColour (juce::Label::textColourId, ParityLookAndFeel::colourForDelta (deltaValue));
        }
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
        abSwitch.setEnabledForReference (processorRef.getReferencePlayer().hasFileLoaded());
    });
}

void ParityAudioProcessorEditor::updateFileLabel()
{
    const auto file = processorRef.getReferencePlayer().getFile();
    fileLabel.setText (file != juce::File() ? file.getFileName() : "No reference loaded",
                       juce::dontSendNotification);
}
