#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
ParityAudioProcessorEditor::ParityAudioProcessorEditor (ParityAudioProcessor& p)
    : AudioProcessorEditor (&p), processorRef (p),
      spectrumView (p.getMixSpectrum(), p.getReferenceSpectrum())
{
    setLookAndFeel (&lookAndFeel);

    titleLabel.setText ("PARITY", juce::dontSendNotification);
    titleLabel.setFont (ParityLookAndFeel::getLabelFont (22.0f).boldened());
    titleLabel.setJustificationType (juce::Justification::centredLeft);
    addAndMakeVisible (titleLabel);

    addAndMakeVisible (loadButton);
    loadButton.onClick = [this] { loadButtonClicked(); };
    loadButton.setTooltip ("Load an audio file (wav, aiff, flac, mp3) to compare your mix against");

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
    abSwitch.setTooltip ("Switch between listening to your mix and the reference track");

    spectrumSectionLabel.setText ("SPECTRUM", juce::dontSendNotification);
    spectrumSectionLabel.setFont (ParityLookAndFeel::getLabelFont (12.0f));
    spectrumSectionLabel.setColour (juce::Label::textColourId, ParityLookAndFeel::inkFaint);
    addAndMakeVisible (spectrumSectionLabel);

    addAndMakeVisible (spectrumView);

    auto setUpModePair = [this] (juce::TextButton& first, juce::TextButton& second, int radioGroup)
    {
        for (auto* button : { &first, &second })
        {
            button->setRadioGroupId (radioGroup);
            button->setClickingTogglesState (true);
            addAndMakeVisible (*button);
        }
    };

    setUpModePair (overlayButton, differenceButton, 1001);
    setUpModePair (realtimeButton, averageButton, 1002);

    overlayButton.setToggleState (true, juce::dontSendNotification);
    realtimeButton.setToggleState (true, juce::dontSendNotification);

    overlayButton.onClick = [this] { spectrumView.setDisplay (SpectrumView::Display::overlay); };
    differenceButton.onClick = [this] { spectrumView.setDisplay (SpectrumView::Display::difference); };
    realtimeButton.onClick = [this] { spectrumView.setAveraging (false); };
    averageButton.onClick = [this] { spectrumView.setAveraging (true); };

    overlayButton.setTooltip ("Show mix and reference curves together");
    differenceButton.setTooltip ("Show the dB difference per frequency (mix minus reference)");
    realtimeButton.setTooltip ("Fast-moving spectrum that follows the audio");
    averageButton.setTooltip ("Slow long-term average, best for judging overall tonal balance");

    for (auto* modeLabel : { &viewModeLabel, &meterModeLabel })
    {
        modeLabel->setFont (ParityLookAndFeel::getLabelFont (11.0f));
        modeLabel->setColour (juce::Label::textColourId, ParityLookAndFeel::inkFaint);
        modeLabel->setJustificationType (juce::Justification::centredRight);
        addAndMakeVisible (*modeLabel);
    }

    viewModeLabel.setText ("VIEW", juce::dontSendNotification);
    meterModeLabel.setText ("METER", juce::dontSendNotification);

    loudnessSectionLabel.setText ("LOUDNESS", juce::dontSendNotification);
    loudnessSectionLabel.setFont (ParityLookAndFeel::getLabelFont (12.0f));
    loudnessSectionLabel.setColour (juce::Label::textColourId, ParityLookAndFeel::inkFaint);
    addAndMakeVisible (loudnessSectionLabel);

    const char* rowNames[numLoudnessRows] = { "Momentary", "Short-term", "Integrated", "Peak" };
    const char* rowTooltips[numLoudnessRows] = {
        "Loudness over the last 400 ms",
        "Loudness over the last 3 seconds",
        "Gated loudness of the whole program (EBU R128). Reference shows the full file",
        "Highest sample level"
    };

    for (int row = 0; row < numLoudnessRows; ++row)
    {
        auto& name = rowLabels[(size_t) row];
        name.setText (rowNames[row], juce::dontSendNotification);
        name.setFont (ParityLookAndFeel::getLabelFont (14.0f));
        name.setTooltip (rowTooltips[row]);
        addAndMakeVisible (name);

        for (auto* label : { &mixValueLabels[(size_t) row], &refValueLabels[(size_t) row], &deltaLabels[(size_t) row] })
        {
            label->setJustificationType (juce::Justification::centredRight);
            label->setFont (ParityLookAndFeel::getMonoFont (14.0f));
            addAndMakeVisible (*label);
        }
    }

    auto setUpHeaderTrio = [this] (juce::Label& mixH, juce::Label& refH, juce::Label& deltaH)
    {
        mixH.setText ("MIX", juce::dontSendNotification);
        refH.setText ("REF", juce::dontSendNotification);
        deltaH.setText ("MIX-REF", juce::dontSendNotification);

        for (auto* header : { &mixH, &refH, &deltaH })
        {
            header->setJustificationType (juce::Justification::centredRight);
            header->setFont (ParityLookAndFeel::getLabelFont (12.0f));
            addAndMakeVisible (*header);
        }

        // Colors match the spectrum curves and the A/B switch.
        mixH.setColour (juce::Label::textColourId, ParityLookAndFeel::ink);
        refH.setColour (juce::Label::textColourId, ParityLookAndFeel::accent);
        deltaH.setColour (juce::Label::textColourId, ParityLookAndFeel::inkFaint);
        deltaH.setTooltip ("Mix value minus reference value. Positive means the mix is higher");
    };

    setUpHeaderTrio (mixHeader, refHeader, deltaHeader);
    setUpHeaderTrio (stereoMixHeader, stereoRefHeader, stereoDeltaHeader);

    stereoSectionLabel.setText ("STEREO", juce::dontSendNotification);
    stereoSectionLabel.setFont (ParityLookAndFeel::getLabelFont (12.0f));
    stereoSectionLabel.setColour (juce::Label::textColourId, ParityLookAndFeel::inkFaint);
    addAndMakeVisible (stereoSectionLabel);

    const char* stereoRowNames[numStereoRows] = { "Correlation", "Width" };
    const char* stereoRowTooltips[numStereoRows] = {
        "L/R phase relationship: +1 is mono-compatible, 0 is fully wide, negative means phase problems",
        "Side-to-mid energy ratio: lower is narrower, 0 dB means equal mid and side energy"
    };

    for (int row = 0; row < numStereoRows; ++row)
    {
        auto& name = stereoRowLabels[(size_t) row];
        name.setText (stereoRowNames[row], juce::dontSendNotification);
        name.setFont (ParityLookAndFeel::getLabelFont (14.0f));
        name.setTooltip (stereoRowTooltips[row]);
        addAndMakeVisible (name);

        for (auto* label : { &stereoMixLabels[(size_t) row], &stereoRefLabels[(size_t) row], &stereoDeltaLabels[(size_t) row] })
        {
            label->setJustificationType (juce::Justification::centredRight);
            label->setFont (ParityLookAndFeel::getMonoFont (14.0f));
            addAndMakeVisible (*label);
        }
    }

    updateLoudnessLabels();
    updateStereoLabels();
    startTimerHz (10);

    setResizable (true, true);
    setResizeLimits (560, 584, 1100, 920);
    setSize (700, 644);
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

    if (! stereoTableArea.isEmpty())
    {
        const auto rowHeight = stereoTableArea.getHeight() / (numStereoRows + 1);

        for (int row = 0; row <= numStereoRows; ++row)
        {
            const auto y = stereoTableArea.getY() + rowHeight * (row + 1);
            g.drawHorizontalLine (y, (float) stereoTableArea.getX(), (float) stereoTableArea.getRight());
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
    abSwitch.setBounds (area.removeFromTop (48));
    area.removeFromTop (16);

    // Spectrum section: label + mode buttons on one strip, view below.
    auto spectrumStrip = area.removeFromTop (22);
    spectrumSectionLabel.setBounds (spectrumStrip.removeFromLeft (110));
    averageButton.setBounds (spectrumStrip.removeFromRight (46));
    realtimeButton.setBounds (spectrumStrip.removeFromRight (46));
    meterModeLabel.setBounds (spectrumStrip.removeFromRight (48));
    spectrumStrip.removeFromRight (12);
    differenceButton.setBounds (spectrumStrip.removeFromRight (52));
    overlayButton.setBounds (spectrumStrip.removeFromRight (74));
    viewModeLabel.setBounds (spectrumStrip.removeFromRight (42));
    area.removeFromTop (6);

    // Fixed-height loudness + stereo tables at the bottom; spectrum takes the rest.
    constexpr int rowHeightFixed = 24;
    constexpr int loudnessSectionHeight = 18 + 6 + (numLoudnessRows + 1) * rowHeightFixed;
    constexpr int stereoSectionHeight = 18 + 6 + (numStereoRows + 1) * rowHeightFixed;
    auto bottomSection = area.removeFromBottom (loudnessSectionHeight + 10 + stereoSectionHeight);

    spectrumView.setBounds (area.withTrimmedBottom (16));

    auto tableSection = bottomSection.removeFromTop (loudnessSectionHeight);
    bottomSection.removeFromTop (10);
    auto stereoSection = bottomSection;

    loudnessSectionLabel.setBounds (tableSection.removeFromTop (18));
    tableSection.removeFromTop (6);

    // Loudness table.
    tableArea = tableSection;
    auto tableRows = tableSection;
    const auto rowHeight = tableRows.getHeight() / (numLoudnessRows + 1);
    const auto nameWidth = juce::jmax (100, tableRows.getWidth() / 4);
    const auto valueWidth = (tableRows.getWidth() - nameWidth) / 3;

    auto headerRow = tableRows.removeFromTop (rowHeight);
    headerRow.removeFromLeft (nameWidth);
    mixHeader.setBounds (headerRow.removeFromLeft (valueWidth));
    refHeader.setBounds (headerRow.removeFromLeft (valueWidth));
    deltaHeader.setBounds (headerRow.removeFromLeft (valueWidth));

    for (int row = 0; row < numLoudnessRows; ++row)
    {
        auto rowArea = tableRows.removeFromTop (rowHeight);
        rowLabels[(size_t) row].setBounds (rowArea.removeFromLeft (nameWidth));
        mixValueLabels[(size_t) row].setBounds (rowArea.removeFromLeft (valueWidth));
        refValueLabels[(size_t) row].setBounds (rowArea.removeFromLeft (valueWidth));
        deltaLabels[(size_t) row].setBounds (rowArea.removeFromLeft (valueWidth));
    }

    // Stereo table.
    stereoSectionLabel.setBounds (stereoSection.removeFromTop (18));
    stereoSection.removeFromTop (6);
    stereoTableArea = stereoSection;

    auto stereoHeaderRow = stereoSection.removeFromTop (rowHeightFixed);
    stereoHeaderRow.removeFromLeft (nameWidth);
    stereoMixHeader.setBounds (stereoHeaderRow.removeFromLeft (valueWidth));
    stereoRefHeader.setBounds (stereoHeaderRow.removeFromLeft (valueWidth));
    stereoDeltaHeader.setBounds (stereoHeaderRow.removeFromLeft (valueWidth));

    for (int row = 0; row < numStereoRows; ++row)
    {
        auto rowArea = stereoSection.removeFromTop (rowHeightFixed);
        stereoRowLabels[(size_t) row].setBounds (rowArea.removeFromLeft (nameWidth));
        stereoMixLabels[(size_t) row].setBounds (rowArea.removeFromLeft (valueWidth));
        stereoRefLabels[(size_t) row].setBounds (rowArea.removeFromLeft (valueWidth));
        stereoDeltaLabels[(size_t) row].setBounds (rowArea.removeFromLeft (valueWidth));
    }
}

void ParityAudioProcessorEditor::timerCallback()
{
    abSwitch.setEnabledForReference (processorRef.getReferencePlayer().hasFileLoaded());
    abSwitch.setReferenceActive (processorRef.isReferenceActive(), juce::dontSendNotification);
    updateLoudnessLabels();
    updateStereoLabels();
}

void ParityAudioProcessorEditor::updateStereoLabels()
{
    const auto& mix = processorRef.getMixStereo();
    const auto& ref = processorRef.getReferenceStereo();

    struct RowValues { float mix, ref; };

    const RowValues values[numStereoRows] = {
        { mix.getCorrelation(), ref.getCorrelation() },
        { mix.getWidthDb(), ref.getWidthDb() }
    };

    // Delta thresholds tuned per metric: correlation +-0.1/+-0.3, width +-1.5/+-4 dB
    // (mapped onto colourForDelta's 1 LU / 3 LU scale).
    const float deltaScales[numStereoRows] = { 10.0f, 0.75f };

    auto hasValue = [] (float v) { return v > StereoAnalyzer::noValue + 1.0f; };

    for (int row = 0; row < numStereoRows; ++row)
    {
        auto formatValue = [row, &hasValue] (float value) -> juce::String
        {
            if (! hasValue (value))
                return "-";

            return row == 0 ? juce::String (value, 2)
                            : juce::String (value, 1) + " dB";
        };

        stereoMixLabels[(size_t) row].setText (formatValue (values[row].mix), juce::dontSendNotification);
        stereoRefLabels[(size_t) row].setText (formatValue (values[row].ref), juce::dontSendNotification);

        auto& delta = stereoDeltaLabels[(size_t) row];

        if (! hasValue (values[row].mix) || ! hasValue (values[row].ref))
        {
            delta.setText ("-", juce::dontSendNotification);
            delta.setColour (juce::Label::textColourId, ParityLookAndFeel::inkFaint);
        }
        else
        {
            const auto deltaValue = values[row].mix - values[row].ref;
            delta.setText (row == 0 ? juce::String (deltaValue, 2)
                                    : juce::String (deltaValue, 1) + " dB",
                           juce::dontSendNotification);
            delta.setColour (juce::Label::textColourId,
                             ParityLookAndFeel::colourForDelta (deltaValue * deltaScales[row]));
        }
    }
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
    fileLabel.setText (file != juce::File() ? file.getFileName()
                                            : "No reference loaded - click LOAD REFERENCE",
                       juce::dontSendNotification);
}
