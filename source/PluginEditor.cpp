#include "PluginProcessor.h"
#include "PluginEditor.h"

#include "BinaryData.h"

//==============================================================================
ParityAudioProcessorEditor::ParityAudioProcessorEditor (ParityAudioProcessor& p)
    : AudioProcessorEditor (&p), processorRef (p),
      spectrumView (p.getMixSpectrum(), p.getReferenceSpectrum())
{
    setLookAndFeel (&lookAndFeel);

    audioLinesIcon = ParityLookAndFeel::loadIcon (BinaryData::audiolines_svg, BinaryData::audiolines_svgSize);
    folderIcon     = ParityLookAndFeel::loadIcon (BinaryData::folderopen_svg, BinaryData::folderopen_svgSize);
    settingsIcon   = ParityLookAndFeel::loadIcon (BinaryData::settings_svg, BinaryData::settings_svgSize);
    pauseIcon      = ParityLookAndFeel::loadIcon (BinaryData::pause_svg, BinaryData::pause_svgSize);
    checkIcon      = ParityLookAndFeel::loadIcon (BinaryData::checklight_svg, BinaryData::checklight_svgSize);

    //==============================================================================
    // Header
    titleLabel.setText ("PARITY", juce::dontSendNotification);
    titleLabel.setFont (ParityLookAndFeel::getFont (20.0f, juce::Font::bold));
    titleLabel.setJustificationType (juce::Justification::centredLeft);
    addAndMakeVisible (titleLabel);

    fileLabel.setJustificationType (juce::Justification::centredLeft);
    fileLabel.setFont (ParityLookAndFeel::getFont (12.0f));
    addAndMakeVisible (fileLabel);
    updateFileLabel();

    addAndMakeVisible (loadButton);
    loadButton.onClick = [this] { loadButtonClicked(); };
    loadButton.setTooltip ("Load an audio file (wav, aiff, flac, mp3) to compare your mix against");

    //==============================================================================
    // Monitoring strip
    abSwitch.setEnabledForReference (processorRef.getReferencePlayer().hasFileLoaded());
    abSwitch.setReferenceActive (processorRef.isReferenceActive(), juce::dontSendNotification);
    abSwitch.onChange = [this] (bool refActive) { processorRef.setReferenceActive (refActive); };
    addAndMakeVisible (abSwitch);
    abSwitch.setTooltip ("Switch between listening to your mix and the reference track");

    matchButton.setClickingTogglesState (true);
    matchButton.setTooltip ("Level-match the reference to your mix's integrated loudness for fair A/B comparisons");
    addAndMakeVisible (matchButton);

    //==============================================================================
    // Spectrum toolbar
    spectrumTitleLabel.setText ("Spectrum", juce::dontSendNotification);
    spectrumTitleLabel.setFont (ParityLookAndFeel::getFont (13.0f, juce::Font::bold));
    spectrumTitleLabel.setColour (juce::Label::textColourId, ParityLookAndFeel::ink);
    addAndMakeVisible (spectrumTitleLabel);

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

    addAndMakeVisible (spectrumView);

    //==============================================================================
    // Loudness table
    loudnessTitleLabel.setText ("Loudness", juce::dontSendNotification);
    loudnessTitleLabel.setFont (ParityLookAndFeel::getFont (13.0f, juce::Font::bold));
    loudnessTitleLabel.setColour (juce::Label::textColourId, ParityLookAndFeel::ink);
    addAndMakeVisible (loudnessTitleLabel);

    const char* rowNames[numLoudnessRows] = { "Momentary LUFS", "Short-Term LUFS",
                                              "Integrated LUFS", "True Peak dBTP" };
    const char* rowTooltips[numLoudnessRows] = {
        "Loudness over the last 400 ms",
        "Loudness over the last 3 seconds",
        "Gated loudness of the whole program (EBU R128). Reference shows the full file",
        "Inter-sample peak (4x oversampled, BS.1770). Can exceed sample peak - keep below -1 dBTP for streaming"
    };

    for (int row = 0; row < numLoudnessRows; ++row)
    {
        auto& name = rowLabels[(size_t) row];
        name.setText (rowNames[row], juce::dontSendNotification);
        name.setFont (ParityLookAndFeel::getFont (12.0f));
        name.setTooltip (rowTooltips[row]);
        addAndMakeVisible (name);

        for (auto* label : { &mixValueLabels[(size_t) row], &refValueLabels[(size_t) row], &deltaLabels[(size_t) row] })
        {
            label->setJustificationType (juce::Justification::centredRight);
            label->setFont (ParityLookAndFeel::getFont (18.0f));
            addAndMakeVisible (*label);
        }

        deltaLabels[(size_t) row].setFont (ParityLookAndFeel::getFont (17.0f));
    }

    auto setUpHeader = [this] (juce::Label& label, const juce::String& text, juce::Colour colour)
    {
        label.setText (text, juce::dontSendNotification);
        label.setFont (ParityLookAndFeel::getFont (10.0f, juce::Font::bold));
        label.setJustificationType (juce::Justification::centredRight);
        label.setColour (juce::Label::textColourId, colour);
        addAndMakeVisible (label);
    };

    setUpHeader (mixHeader, "MIX", ParityLookAndFeel::ink);
    setUpHeader (refHeader, "REF", ParityLookAndFeel::accent);
    setUpHeader (deltaHeader, juce::String::fromUTF8 ("Δ LU / dB"), ParityLookAndFeel::inkGhost);
    deltaHeader.setTooltip ("Difference: mix minus reference. Positive means the mix is higher");

    peakWarningLabel.setText ("", juce::dontSendNotification);
    addAndMakeVisible (peakWarningLabel);

    //==============================================================================
    // Stereo section
    stereoTitleLabel.setText ("Stereo", juce::dontSendNotification);
    stereoTitleLabel.setFont (ParityLookAndFeel::getFont (13.0f, juce::Font::bold));
    stereoTitleLabel.setColour (juce::Label::textColourId, ParityLookAndFeel::ink);
    addAndMakeVisible (stereoTitleLabel);

    setUpHeader (stereoMixHeader, "MIX", ParityLookAndFeel::ink);
    setUpHeader (stereoRefHeader, "REF", ParityLookAndFeel::accent);
    setUpHeader (stereoDeltaHeader, juce::String::fromUTF8 ("Δ"), ParityLookAndFeel::inkGhost);

    correlationNameLabel.setText ("Correlation", juce::dontSendNotification);
    correlationNameLabel.setTooltip ("Stereo correlation: +1 = mono-compatible, 0 = fully decorrelated, -1 = out of phase");
    widthNameLabel.setText (juce::String::fromUTF8 ("Width \xc2\xb7 dB"), juce::dontSendNotification);
    widthNameLabel.setTooltip ("Mid/side level difference: higher = wider stereo image");

    for (auto* label : { &correlationNameLabel, &widthNameLabel })
    {
        label->setFont (ParityLookAndFeel::getFont (13.0f));
        addAndMakeVisible (*label);
    }

    for (int row = 0; row < 2; ++row)
        for (auto* label : { &stereoMixValues[(size_t) row], &stereoRefValues[(size_t) row], &stereoDeltaValues[(size_t) row] })
        {
            label->setJustificationType (juce::Justification::centredRight);
            label->setFont (ParityLookAndFeel::getFont (18.0f));
            addAndMakeVisible (*label);
        }

    for (auto* label : { &stereoDeltaValues[0], &stereoDeltaValues[1] })
        label->setFont (ParityLookAndFeel::getFont (17.0f));

    correlationBar.setScaleLabels (juce::String::fromUTF8 ("−1"), "0", "+1");
    correlationBar.setFillAnchor (0.5f);
    correlationBar.setTooltip ("Correlation scale: markers show mix (pale) and reference (orange)");
    widthBar.setScaleLabels (juce::String::fromUTF8 ("−30"), juce::String::fromUTF8 ("−15"), "0 dB");
    widthBar.setFillAnchor (0.0f);
    widthBar.setTooltip ("Width scale in dB (mid minus side level): markers show mix and reference");
    addAndMakeVisible (correlationBar);
    addAndMakeVisible (widthBar);

    //==============================================================================
    // Master processors panel
    masterTitleLabel.setText ("MASTER", juce::dontSendNotification);
    masterTitleLabel.setFont (ParityLookAndFeel::getFont (10.0f, juce::Font::bold));
    masterTitleLabel.setColour (juce::Label::textColourId, ParityLookAndFeel::inkMuted);
    addAndMakeVisible (masterTitleLabel);

    clipNameLabel.setText ("CLIP", juce::dontSendNotification);
    limitNameLabel.setText ("LIMIT", juce::dontSendNotification);

    for (auto* label : { &clipNameLabel, &limitNameLabel })
    {
        label->setFont (ParityLookAndFeel::getFont (11.0f, juce::Font::bold));
        label->setColour (juce::Label::textColourId, ParityLookAndFeel::ink);
        label->setJustificationType (juce::Justification::centred);
        addAndMakeVisible (*label);
    }

    addAndMakeVisible (clipPill);
    addAndMakeVisible (limitPill);
    clipPill.setTooltip ("Hard-clip the mix at the threshold (flat-top distortion). The reference is never processed");
    limitPill.setTooltip ("Brickwall-limit the mix: GAIN pushes into the CEIL, REL sets recovery speed");

    clipFader.setTooltip ("Clip threshold: samples beyond this level are chopped flat");
    gainFader.setTooltip ("Gain into the limiter: push up for loudness, the ceiling catches the peaks");
    ceilingFader.setTooltip ("Output ceiling the limiter never exceeds (keep around -1 dB for streaming)");
    releaseFader.setTooltip ("How fast the limiter recovers: short = louder but can distort, long = cleaner but pumps");

    for (auto* fader : { &clipFader, &gainFader, &ceilingFader, &releaseFader })
        addAndMakeVisible (*fader);

    const char* units[4] = { "dB", "dB", "dB", "ms" };
    const char* captions[4] = { "THRESHOLD", "GAIN", "CEIL", "REL" };

    for (int i = 0; i < 4; ++i)
    {
        auto& value = faderValueLabels[(size_t) i];
        value.setFont (ParityLookAndFeel::getFont (14.0f, juce::Font::bold));
        value.setColour (juce::Label::textColourId, ParityLookAndFeel::ink);
        value.setJustificationType (juce::Justification::centred);
        addAndMakeVisible (value);

        auto& unit = faderUnitLabels[(size_t) i];
        unit.setText (units[i], juce::dontSendNotification);
        unit.setFont (ParityLookAndFeel::getFont (10.0f));
        unit.setJustificationType (juce::Justification::centred);
        addAndMakeVisible (unit);

        auto& caption = faderCaptionLabels[(size_t) i];
        caption.setText (captions[i], juce::dontSendNotification);
        caption.setFont (ParityLookAndFeel::getFont (11.0f));
        caption.setJustificationType (juce::Justification::centred);
        addAndMakeVisible (caption);
    }

    auto& params = processorRef.getParameters();
    matchAttachment = std::make_unique<ButtonAttachment> (params, "matchOn", matchButton);
    clipOnAttachment = std::make_unique<ButtonAttachment> (params, "clipOn", clipPill);
    clipThresholdAttachment = std::make_unique<SliderAttachment> (params, "clipThreshold", clipFader);
    limitOnAttachment = std::make_unique<ButtonAttachment> (params, "limitOn", limitPill);
    limitGainAttachment = std::make_unique<SliderAttachment> (params, "limitGain", gainFader);
    limitCeilingAttachment = std::make_unique<SliderAttachment> (params, "limitCeiling", ceilingFader);
    limitReleaseAttachment = std::make_unique<SliderAttachment> (params, "limitRelease", releaseFader);

    startTimerHz (10);

    setResizable (true, false);
    setResizeLimits (920, 640, 1500, 1100);
    setSize (1100, 750);
}

ParityAudioProcessorEditor::~ParityAudioProcessorEditor()
{
    setLookAndFeel (nullptr);
}

//==============================================================================
void ParityAudioProcessorEditor::paint (juce::Graphics& g)
{
    using LF = ParityLookAndFeel;

    g.fillAll (LF::background);

    // Section backgrounds.
    g.setColour (LF::panel);
    g.fillRect (monitorArea);
    g.fillRect (bottomStripArea);

    // Hairline rules between sections.
    g.setColour (LF::line);
    g.drawHorizontalLine ((int) headerArea.getBottom() - 1, 0.0f, (float) getWidth());
    g.drawHorizontalLine ((int) monitorArea.getBottom() - 1, 0.0f, (float) getWidth());
    g.drawHorizontalLine ((int) bottomStripArea.getY(), 0.0f, (float) getWidth());
    g.drawVerticalLine ((int) loudnessArea.getRight(), bottomStripArea.getY(), bottomStripArea.getBottom());
    g.drawVerticalLine ((int) stereoArea.getRight(), bottomStripArea.getY(), bottomStripArea.getBottom());

    // Header: title divider, file icon, load-button icon, settings gear.
    g.drawVerticalLine (113, headerArea.getY() + 18.0f, headerArea.getY() + 38.0f);

    if (audioLinesIcon != nullptr)
        audioLinesIcon->drawWithin (g, fileIconArea, juce::RectanglePlacement::centred, 1.0f);

    if (folderIcon != nullptr)
        folderIcon->drawWithin (g, loadIconArea, juce::RectanglePlacement::centred, 1.0f);

    if (settingsIcon != nullptr)
        settingsIcon->drawWithin (g, settingsIconArea, juce::RectanglePlacement::centred, 0.9f);

    // Monitoring strip: divider, transport pause icon + progress bar.
    g.drawVerticalLine (212, monitorArea.getY() + 16.0f, monitorArea.getY() + 48.0f);

    if (pauseIcon != nullptr)
        pauseIcon->drawWithin (g, transportIconArea, juce::RectanglePlacement::centred,
                               processorRef.isHostPlaying() ? 1.0f : 0.4f);

    g.setColour (LF::line);
    g.fillRoundedRectangle (progressBarArea, 2.0f);

    if (referenceProgress > 0.0f)
    {
        g.setColour (LF::accent);
        g.fillRoundedRectangle (progressBarArea.withWidth (progressBarArea.getWidth()
                                                             * juce::jlimit (0.0f, 1.0f, referenceProgress)),
                                2.0f);
    }

    // Spectrum toolbar legend: trace chips next to the title.
    {
        auto chip = legendArea.removeFromLeft (41.0f);

        for (auto entry : { std::pair<juce::Colour, const char*> { LF::inkSoft, "MIX" },
                            { LF::accent, "REF" } })
        {
            g.setColour (entry.first);
            g.fillRect (chip.getX(), chip.getCentreY() - 1.0f, 16.0f, 2.0f);
            g.setFont (ParityLookAndFeel::getFont (10.0f, juce::Font::bold));
            g.drawText (entry.second, chip.withTrimmedLeft (22.0f), juce::Justification::centredLeft);
            chip = legendArea.withTrimmedLeft ((float) chip.getRight() - legendArea.getX());
        }
    }

    // Master panel card.
    if (! masterPanelArea.isEmpty())
    {
        g.setColour (LF::panel);
        g.fillRoundedRectangle (masterPanelArea, 8.0f);
        g.setColour (LF::line);
        g.drawRoundedRectangle (masterPanelArea.reduced (0.5f), 8.0f, 1.0f);

        g.fillRect (masterDivider);
        g.fillRect (masterColumnDivider);
    }

    // Peak warning dot under the loudness table when a channel is over 0 dBTP.
    if (peakWarningLabel.getText().isNotEmpty())
    {
        g.setColour (LF::warn);
        g.fillEllipse (loudnessArea.getX() + 24.0f, bottomStripArea.getBottom() - 24.0f, 4.0f, 4.0f);
    }
}

void ParityAudioProcessorEditor::paintOverChildren (juce::Graphics& g)
{
    // Check mark inside the MATCH toggle when loudness matching is on.
    if (matchButton.getToggleState() && checkIcon != nullptr)
    {
        auto iconArea = matchButton.getBounds().toFloat();
        iconArea = juce::Rectangle<float> (iconArea.getX() + 13.5f, iconArea.getY() + 10.0f, 12.0f, 12.0f);
        checkIcon->drawWithin (g, iconArea, juce::RectanglePlacement::centred, 1.0f);
    }
}

void ParityAudioProcessorEditor::resized()
{
    const auto width = getWidth();
    const auto height = getHeight();

    headerArea = { 0.0f, 0.0f, (float) width, 56.0f };
    monitorArea = { 0.0f, 56.0f, (float) width, 64.0f };
    bottomStripArea = { 0.0f, (float) height - 246.0f, (float) width, 246.0f };

    // Floating master panel: fixed width at the right edge, spanning the
    // spectrum body down to the bottom of the window.
    const auto panelTop = headerArea.getBottom() + monitorArea.getHeight() + 48.0f;
    masterPanelArea = { (float) width - 24.0f - 248.0f, panelTop,
                        248.0f, (float) height - panelTop };

    //==============================================================================
    // Header internals
    titleLabel.setBounds (24, 16, 73, 24);

    settingsIconArea = { (float) width - 24.0f - 22.0f, 20.0f, 16.0f, 16.0f };
    loadButton.setBounds (width - 24 - 28 - 16 - 129, 12, 129, 32);
    loadIconArea = { (float) loadButton.getX() + 12.0f, 21.0f, 14.0f, 14.0f };

    fileIconArea = { 130.0f, 20.0f, 16.0f, 16.0f };
    fileLabel.setBounds (154, 20, juce::jmax (50, loadButton.getX() - 16 - 154), 16);

    //==============================================================================
    // Monitoring strip internals
    abSwitch.setBounds (24, (int) monitorArea.getY() + 12, 172, 40);
    matchButton.setBounds (229, (int) monitorArea.getY() + 16, 88, 32);

    progressBarArea = { (float) width - 24.0f - 96.0f, monitorArea.getCentreY() - 1.5f, 96.0f, 3.0f };
    transportIconArea = { progressBarArea.getX() - 12.0f - 14.0f, monitorArea.getCentreY() - 7.0f, 14.0f, 14.0f };

    //==============================================================================
    // Spectrum toolbar
    const auto toolbarY = (int) monitorArea.getBottom();
    spectrumTitleLabel.setBounds (24, toolbarY + 16, 63, 16);
    legendArea = { 103.0f, (float) toolbarY + 18.0f, 98.0f, 12.0f };

    averageButton.setBounds (width - 24 - 44, toolbarY + 8, 44, 32);
    realtimeButton.setBounds (averageButton.getX() - 4 - 47, toolbarY + 8, 47, 32);
    differenceButton.setBounds (realtimeButton.getX() - 16 - 79, toolbarY + 8, 79, 32);
    overlayButton.setBounds (differenceButton.getX() - 4 - 66, toolbarY + 8, 66, 32);

    //==============================================================================
    // Spectrum view (plot + axis gutters live inside the component)
    spectrumView.setBounds (24, toolbarY + 48,
                          juce::jmax (100, (int) masterPanelArea.getX() - 24 - 16 - 24),
                          juce::jmax (80, (int) bottomStripArea.getY() - toolbarY - 48 - 12));

    //==============================================================================
    // Bottom strip: loudness 440 px, stereo fills to the master panel.
    const auto stripTop = bottomStripArea.getY();

    loudnessArea = { 0.0f, stripTop, 440.0f, 246.0f };
    stereoArea = { loudnessArea.getRight(), stripTop,
                   juce::jmax (200.0f, masterPanelArea.getX() - loudnessArea.getRight()), 246.0f };

    {
        auto content = loudnessArea.reduced (0.0f).toNearestInt();
        content.removeFromLeft (24);
        content.removeFromRight (16);
        content.removeFromTop (16);

        loudnessTitleLabel.setBounds (content.removeFromTop (28));

        auto columns = content.removeFromTop (20);
        columns.removeFromLeft (148);
        mixHeader.setBounds (columns.removeFromLeft (84));
        refHeader.setBounds (columns.removeFromLeft (84));
        deltaHeader.setBounds (columns);

        for (int row = 0; row < numLoudnessRows; ++row)
        {
            auto rowArea = content.removeFromTop (36);
            rowLabels[(size_t) row].setBounds (rowArea.removeFromLeft (148).withTrimmedTop (10));
            mixValueLabels[(size_t) row].setBounds (rowArea.removeFromLeft (84).withTrimmedTop (7));
            refValueLabels[(size_t) row].setBounds (rowArea.removeFromLeft (84).withTrimmedTop (7));
            deltaLabels[(size_t) row].setBounds (rowArea.withTrimmedTop (7));
        }
    }

    {
        auto content = stereoArea.toNearestInt().reduced (16);

        stereoTitleLabel.setBounds (content.removeFromTop (28));

        auto columns = content.removeFromTop (20);
        const auto third = columns.getWidth() / 3;
        stereoMixHeader.setBounds (columns.removeFromLeft (third));
        stereoRefHeader.setBounds (columns.removeFromLeft (third));
        stereoDeltaHeader.setBounds (columns);

        correlationNameLabel.setBounds (content.removeFromTop (16));

        auto correlationReadouts = content.removeFromTop (32);
        const auto readoutWidth = (correlationReadouts.getWidth() - 16) / 3;
        stereoMixValues[0].setBounds (correlationReadouts.removeFromLeft (readoutWidth + 8));
        stereoRefValues[0].setBounds (correlationReadouts.removeFromLeft (readoutWidth));
        stereoDeltaValues[0].setBounds (correlationReadouts);

        correlationBar.setBounds (content.removeFromTop (26).withTrimmedTop (0));

        widthNameLabel.setBounds (content.removeFromTop (28).withTrimmedTop (12));

        auto widthReadouts = content.removeFromTop (32);
        stereoMixValues[1].setBounds (widthReadouts.removeFromLeft (readoutWidth + 8));
        stereoRefValues[1].setBounds (widthReadouts.removeFromLeft (readoutWidth));
        stereoDeltaValues[1].setBounds (widthReadouts);

        widthBar.setBounds (content.removeFromTop (26));
    }

    //==============================================================================
    // Master panel internals
    {
        auto content = masterPanelArea.toNearestInt().reduced (16);

        masterTitleLabel.setBounds (content.removeFromTop (12));
        content.removeFromTop (12);
        masterDivider = content.removeFromTop (1).toFloat();
        content.removeFromTop (12);

        auto processorRow = content;

        // Fixed bottom rows: value readouts, then captions.
        auto captionRow = processorRow.removeFromBottom (13);
        processorRow.removeFromBottom (8);
        auto valueRow = processorRow.removeFromBottom (30);
        processorRow.removeFromBottom (8);

        // Shared header row: processor name + ON/BYP pill in each column.
        auto headerRow = processorRow.removeFromTop (14);
        processorRow.removeFromTop (8);

        auto clipHeader = headerRow.removeFromLeft (76);
        clipNameLabel.setBounds (clipHeader.removeFromLeft (clipHeader.getWidth() / 2 - 2));
        clipPill.setBounds (clipHeader.withTrimmedLeft (4).withWidth (29));

        auto limitHeader = headerRow.withTrimmedLeft (1);
        limitNameLabel.setBounds (limitHeader.removeFromLeft (limitHeader.getWidth() / 2 - 2));
        limitPill.setBounds (limitHeader.withTrimmedLeft (4).withWidth (29));

        // CLIP column: 76 px, vertical divider, then LIMIT.
        auto clipColumn = processorRow.removeFromLeft (76);

        masterColumnDivider = { (float) processorRow.getX(), (float) processorRow.getY() + 16.0f,
                                1.0f, (float) processorRow.getHeight() - 16.0f };

        auto limitColumn = processorRow.withTrimmedLeft (1);

        // Faders fill the remaining height.
        clipFader.setBounds (clipColumn.withSizeKeepingCentre (44, clipColumn.getHeight()));

        const auto slotWidth = limitColumn.getWidth() / 3;
        for (auto* fader : { &gainFader, &ceilingFader, &releaseFader })
        {
            auto slot = limitColumn.removeFromLeft (slotWidth);
            fader->setBounds (slot.withSizeKeepingCentre (38, slot.getHeight()));
        }

        // Value + unit + caption labels under each fader.
        auto placeReadout = [&] (int index, juce::Rectangle<int> column)
        {
            faderValueLabels[(size_t) index].setBounds (column.getX(), valueRow.getY(), column.getWidth(), 18);
            faderUnitLabels[(size_t) index].setBounds (column.getX(), valueRow.getY() + 18, column.getWidth(), 12);
            faderCaptionLabels[(size_t) index].setBounds (column.getX(), captionRow.getY(), column.getWidth(), 13);
        };

        placeReadout (0, clipColumn.withWidth (76));

        {
            auto limitSlots = masterPanelArea.toNearestInt().reduced (16);
            limitSlots.removeFromTop (12 + 12 + 1 + 12);
            limitSlots.removeFromLeft (76 + 1);
            limitSlots.removeFromTop (14 + 8); // header row + gap

            for (int index = 1; index < 4; ++index)
                placeReadout (index, limitSlots.removeFromLeft (slotWidth));
        }
    }
}

void ParityAudioProcessorEditor::timerCallback()
{
    const auto status = processorRef.getReferenceLoadStatus();

    if (status != lastLoadStatus)
    {
        lastLoadStatus = status;
        loadButton.setEnabled (status != ParityAudioProcessor::ReferenceLoadStatus::loading);
        updateFileLabel();

        if (status == ParityAudioProcessor::ReferenceLoadStatus::failed)
        {
            processorRef.clearReferenceLoadFailure();
            juce::AlertWindow::showMessageBoxAsync (juce::MessageBoxIconType::WarningIcon,
                                                    "Parity",
                                                    "Couldn't load the selected file.");
        }
    }

    abSwitch.setEnabledForReference (processorRef.getReferencePlayer().hasFileLoaded());
    abSwitch.setReferenceActive (processorRef.isReferenceActive(), juce::dontSendNotification);

    const auto duration = processorRef.getReferencePlayer().getDurationSeconds();
    const auto progress = duration > 0.0 ? (float) (processorRef.getPlayheadSeconds() / duration) : 0.0f;

    if (std::abs (progress - referenceProgress) > 0.002f)
    {
        referenceProgress = progress;
        repaint (progressBarArea.toNearestInt().expanded (2));
    }

    // Fader meters.
    clipFader.setMeterLevel (processorRef.getMasterChain().getPostClipLevel());
    const auto limitLevel = processorRef.getMasterChain().getPostLimitLevel();
    gainFader.setMeterLevel (limitLevel);
    ceilingFader.setMeterLevel (limitLevel);
    releaseFader.setMeterLevel (limitLevel);

    updateLoudnessLabels();
    updateStereoLabels();
    updateMasterReadouts();
}

void ParityAudioProcessorEditor::updateStereoLabels()
{
    const auto& mix = processorRef.getMixStereo();
    const auto& ref = processorRef.getReferenceStereo();

    struct RowValues { float mix, ref; };

    const RowValues values[2] = {
        { mix.getCorrelation(), ref.getCorrelation() },
        { mix.getWidthDb(), ref.getWidthDb() }
    };

    auto hasValue = [] (float v) { return v > StereoAnalyzer::noValue + 1.0f; };

    for (int row = 0; row < 2; ++row)
    {
        auto formatValue = [row, &hasValue] (float value) -> juce::String
        {
            if (! hasValue (value))
                return "-";

            return row == 0 ? juce::String (value, 2)
                            : juce::String (value, 1);
        };

        stereoMixValues[(size_t) row].setText (formatValue (values[row].mix), juce::dontSendNotification);
        stereoRefValues[(size_t) row].setText (formatValue (values[row].ref), juce::dontSendNotification);

        stereoMixValues[(size_t) row].setColour (juce::Label::textColourId, ParityLookAndFeel::ink);
        stereoRefValues[(size_t) row].setColour (juce::Label::textColourId, ParityLookAndFeel::accent);

        auto& delta = stereoDeltaValues[(size_t) row];

        if (! hasValue (values[row].mix) || ! hasValue (values[row].ref))
        {
            delta.setText ("-", juce::dontSendNotification);
            delta.setColour (juce::Label::textColourId, ParityLookAndFeel::inkDim);
        }
        else
        {
            const auto deltaValue = values[row].mix - values[row].ref;
            const auto text = (deltaValue >= 0.0f ? "+" : "") + (row == 0 ? juce::String (deltaValue, 2)
                                                                        : juce::String (deltaValue, 1));
            delta.setText (text, juce::dontSendNotification);
            delta.setColour (juce::Label::textColourId,
                             ParityLookAndFeel::colourForDelta (deltaValue * (row == 0 ? 10.0f : 0.75f)));
        }
    }

    // Normalised scale positions for the comparison bars.
    auto toNorm = [&hasValue] (float v, float lo, float hi)
    {
        return hasValue (v) ? juce::jlimit (0.0f, 1.0f, (v - lo) / (hi - lo)) : -1.0f;
    };

    correlationBar.setValues (toNorm (values[0].mix, -1.0f, 1.0f), toNorm (values[0].ref, -1.0f, 1.0f));
    widthBar.setValues (toNorm (values[1].mix, -30.0f, 0.0f), toNorm (values[1].ref, -30.0f, 0.0f));
}

void ParityAudioProcessorEditor::updateLoudnessLabels()
{
    auto format = [] (float value) -> juce::String
    {
        if (value <= LoudnessAnalyzer::silenceLufs + 1.0f)
            return "-";

        return juce::String (value, 1);
    };

    const auto& mix = processorRef.getMixLoudness();
    const auto& ref = processorRef.getReferenceLoudness();
    const auto fileStats = processorRef.getReferenceFileStats();

    const float mixValues[numLoudnessRows] = { mix.getMomentaryLufs(), mix.getShortTermLufs(),
                                               mix.getIntegratedLufs(), mix.getTruePeakDb() };

    // Integrated and true peak for the reference use the offline full-file values.
    const float refValues[numLoudnessRows] = { ref.getMomentaryLufs(), ref.getShortTermLufs(),
                                               fileStats.integratedLufs, fileStats.truePeakDb };

    const float deltaScales[numLoudnessRows] = { 1.0f, 1.0f, 1.0f, 1.0f };

    for (int row = 0; row < numLoudnessRows; ++row)
    {
        // True peak readings turn warning-red once they exceed 0 dBTP.
        const auto peakHot = row == numLoudnessRows - 1;
        const auto mixColour = peakHot && mixValues[row] > 0.0f ? ParityLookAndFeel::warn
                                                              : ParityLookAndFeel::ink;
        const auto refColour = peakHot && refValues[row] > 0.0f ? ParityLookAndFeel::warn
                                                              : ParityLookAndFeel::accent;

        mixValueLabels[(size_t) row].setText (format (mixValues[row]), juce::dontSendNotification);
        mixValueLabels[(size_t) row].setColour (juce::Label::textColourId, mixColour);
        refValueLabels[(size_t) row].setText (format (refValues[row]), juce::dontSendNotification);
        refValueLabels[(size_t) row].setColour (juce::Label::textColourId, refColour);

        auto& delta = deltaLabels[(size_t) row];

        if (mixValues[row] <= LoudnessAnalyzer::silenceLufs + 1.0f
         || refValues[row] <= LoudnessAnalyzer::silenceLufs + 1.0f)
        {
            delta.setText ("-", juce::dontSendNotification);
            delta.setColour (juce::Label::textColourId, ParityLookAndFeel::inkDim);
        }
        else
        {
            const auto deltaValue = mixValues[row] - refValues[row];
            delta.setText ((deltaValue >= 0.0f ? "+" : "") + juce::String (deltaValue, 1),
                           juce::dontSendNotification);
            delta.setColour (juce::Label::textColourId,
                             ParityLookAndFeel::colourForDelta (deltaValue * deltaScales[row]));
        }
    }

    peakWarningLabel.setText (mix.getTruePeakDb() > 0.0f || fileStats.truePeakDb > 0.0f ? "!" : "",
                              juce::dontSendNotification);

    if (peakWarningLabel.getText().isNotEmpty())
        repaint();
}

void ParityAudioProcessorEditor::updateMasterReadouts()
{
    auto format = [] (const juce::Slider& fader) -> juce::String
    {
        const auto v = fader.getValue();
        return v < 100.0 ? juce::String (v, 1) : juce::String ((int) std::round (v));
    };

    for (auto pair : { std::pair<const juce::Slider*, juce::Label*> { &clipFader, &faderValueLabels[0] },
                       { &gainFader, &faderValueLabels[1] },
                       { &ceilingFader, &faderValueLabels[2] },
                       { &releaseFader, &faderValueLabels[3] } })
        pair.second->setText (format (*pair.first), juce::dontSendNotification);
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
            processorRef.loadReferenceFileAsync (file); // timerCallback tracks progress
    });
}

void ParityAudioProcessorEditor::updateFileLabel()
{
    if (processorRef.getReferenceLoadStatus() == ParityAudioProcessor::ReferenceLoadStatus::loading)
    {
        fileLabel.setText ("Loading reference...", juce::dontSendNotification);
        return;
    }

    const auto file = processorRef.getReferencePlayer().getFile();
    fileLabel.setText (file != juce::File() ? file.getFileName()
                                            : "No reference loaded - click Load Reference",
                       juce::dontSendNotification);
}
