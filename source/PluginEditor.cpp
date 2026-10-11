#include "PluginProcessor.h"
#include "PluginEditor.h"

#include "BinaryData.h"

//==============================================================================
ParityAudioProcessorEditor::ParityAudioProcessorEditor (ParityAudioProcessor& p)
    : AudioProcessorEditor (&p), processorRef (p),
      spectrumView (p.getMixSpectrum(), p.getReferenceSpectrum()),
      loudnessHistoryView (p.getMixLoudness(), p.getReferenceLoudness()),
      masterHistoryView (p.getMasterChain(), MasterHistoryView::Mode::detailed),
      panelHistoryView (p.getMasterChain(), MasterHistoryView::Mode::compact)
{
    setLookAndFeel (&lookAndFeel);

    audioLinesIcon = ParityLookAndFeel::loadIcon (BinaryData::audiolines_svg, BinaryData::audiolines_svgSize);
    folderIcon     = ParityLookAndFeel::loadIcon (BinaryData::folderopen_svg, BinaryData::folderopen_svgSize);
    settingsIcon   = ParityLookAndFeel::loadIcon (BinaryData::settings_svg, BinaryData::settings_svgSize);
    pauseIcon      = ParityLookAndFeel::loadIcon (BinaryData::pause_svg, BinaryData::pause_svgSize);

    //==============================================================================
    // Header
    titleLabel.setText ("PARITY", juce::dontSendNotification);
    titleLabel.setFont (ParityLookAndFeel::getLabelFont (13.0f));
    titleLabel.setJustificationType (juce::Justification::centredLeft);
    addAndMakeVisible (titleLabel);

    fileLabel.setJustificationType (juce::Justification::centredLeft);
    fileLabel.setFont (ParityLookAndFeel::getMediumFont (11.5f));
    fileLabel.setColour (juce::Label::textColourId, ParityLookAndFeel::inkFaint);
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
    // Spectrum toolbar — view buttons are text tabs with a sliding accent
    // underline (drawn in paintOverChildren).
    spectrumTitleLabel.setText ("Spectrum", juce::dontSendNotification);
    spectrumTitleLabel.setFont (ParityLookAndFeel::getLabelFont (10.0f));
    spectrumTitleLabel.setColour (juce::Label::textColourId, ParityLookAndFeel::inkDim);
    addAndMakeVisible (spectrumTitleLabel);

    auto setUpRadioGroup = [this] (std::initializer_list<juce::TextButton*> buttons, int radioGroup)
    {
        for (auto* button : buttons)
        {
            button->setRadioGroupId (radioGroup);
            button->setClickingTogglesState (true);
            addAndMakeVisible (*button);
        }
    };

    setUpRadioGroup ({ &overlayButton, &differenceButton, &loudnessButton, &masterButton }, 1001);
    setUpRadioGroup ({ &realtimeButton, &averageButton }, 1002);

    overlayButton.setToggleState (true, juce::dontSendNotification);
    realtimeButton.setToggleState (true, juce::dontSendNotification);

    overlayButton.onClick = [this] { showView (MainView::spectrumOverlay); };
    differenceButton.onClick = [this] { showView (MainView::spectrumDifference); };
    loudnessButton.onClick = [this] { showView (MainView::loudnessHistory); };
    masterButton.onClick = [this] { showView (MainView::masterHistory); };
    realtimeButton.onClick = [this] { spectrumView.setAveraging (false); };
    averageButton.onClick = [this] { spectrumView.setAveraging (true); };

    overlayButton.setTooltip ("Spectrum: mix and reference curves together");
    differenceButton.setTooltip ("Spectrum: dB difference per frequency (mix minus reference)");
    loudnessButton.setTooltip ("Short-term loudness of mix and reference over the last 60 s");
    masterButton.setTooltip ("What the clipper and limiter are doing: input vs threshold/ceiling over the last 6 s");
    realtimeButton.setTooltip ("Fast-moving spectrum that follows the audio");
    averageButton.setTooltip ("Slow long-term average, best for judging overall tonal balance");

    addAndMakeVisible (spectrumView);
    addChildComponent (loudnessHistoryView);
    addChildComponent (masterHistoryView);
    loudnessHistoryView.setTooltip ("Solid: short-term (3 s). Faint: momentary (400 ms). Dashed: integrated");
    masterHistoryView.setTooltip ("Grey: input level. Red: clipped away. Amber: limited away. Pale line: output");

    //==============================================================================
    // Loudness table — section titles demoted to small caps, numbers mono.
    loudnessTitleLabel.setText ("LOUDNESS", juce::dontSendNotification);
    loudnessTitleLabel.setFont (ParityLookAndFeel::getLabelFont (10.0f));
    loudnessTitleLabel.setColour (juce::Label::textColourId, ParityLookAndFeel::inkDim);
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
            label->setFont (ParityLookAndFeel::getMonoFont (16.0f));
            addAndMakeVisible (*label);
        }

        deltaLabels[(size_t) row].setFont (ParityLookAndFeel::getMonoFont (15.0f));
    }

    auto setUpHeader = [this] (juce::Label& label, const juce::String& text, juce::Colour colour)
    {
        label.setText (text, juce::dontSendNotification);
        label.setFont (ParityLookAndFeel::getLabelFont (9.0f));
        label.setJustificationType (juce::Justification::centredRight);
        label.setColour (juce::Label::textColourId, colour);
        addAndMakeVisible (label);
    };

    setUpHeader (mixHeader, "MIX", ParityLookAndFeel::ink);
    setUpHeader (refHeader, "REF", ParityLookAndFeel::accent);
    setUpHeader (deltaHeader, juce::String::fromUTF8 ("Δ LU / dB"), ParityLookAndFeel::inkGhost);
    deltaHeader.setTooltip ("Difference: mix minus reference. Positive means the mix is higher");

    // Latching clip LEDs on the true-peak row — click to reset.
    mixPeakLed.setTooltip ("Latches when the mix exceeds -0.1 dBTP (intersample overs). Click to reset");
    refPeakLed.setTooltip ("Latches when the reference exceeds -0.1 dBTP (intersample overs). Click to reset");
    addAndMakeVisible (mixPeakLed);
    addAndMakeVisible (refPeakLed);

    //==============================================================================
    // Stereo section
    stereoTitleLabel.setText ("STEREO", juce::dontSendNotification);
    stereoTitleLabel.setFont (ParityLookAndFeel::getLabelFont (10.0f));
    stereoTitleLabel.setColour (juce::Label::textColourId, ParityLookAndFeel::inkDim);
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
        label->setFont (ParityLookAndFeel::getMediumFont (11.5f));
        label->setColour (juce::Label::textColourId, ParityLookAndFeel::inkFaint);
        addAndMakeVisible (*label);
    }

    for (int row = 0; row < 2; ++row)
        for (auto* label : { &stereoMixValues[(size_t) row], &stereoRefValues[(size_t) row], &stereoDeltaValues[(size_t) row] })
        {
            label->setJustificationType (juce::Justification::centredRight);
            label->setFont (ParityLookAndFeel::getMonoFont (16.0f));
            addAndMakeVisible (*label);
        }

    for (auto* label : { &stereoDeltaValues[0], &stereoDeltaValues[1] })
        label->setFont (ParityLookAndFeel::getMonoFont (15.0f));

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
    masterTitleLabel.setFont (ParityLookAndFeel::getLabelFont (10.0f));
    masterTitleLabel.setColour (juce::Label::textColourId, ParityLookAndFeel::inkDim);
    addAndMakeVisible (masterTitleLabel);

    clipLed.setTooltip ("Latches while the clipper is working. Click to reset");
    addAndMakeVisible (clipLed);

    clipNameLabel.setText ("CLIP", juce::dontSendNotification);
    limitNameLabel.setText ("LIMIT", juce::dontSendNotification);

    for (auto* label : { &clipNameLabel, &limitNameLabel })
    {
        label->setFont (ParityLookAndFeel::getLabelFont (10.5f));
        label->setColour (juce::Label::textColourId, ParityLookAndFeel::ink);
        label->setJustificationType (juce::Justification::centred);
        addAndMakeVisible (*label);
    }

    addAndMakeVisible (clipPower);
    addAndMakeVisible (limitPower);
    clipPower.setTooltip ("Hard-clip the mix at the threshold (flat-top distortion). The reference is never processed");
    limitPower.setTooltip ("Brickwall-limit the mix: GAIN pushes into the CEIL, REL sets recovery speed");

    clipFader.setTooltip ("Clip threshold: samples beyond this level are chopped flat");
    gainFader.setTooltip ("Gain into the limiter: push up for loudness, the ceiling catches the peaks");
    ceilingFader.setTooltip ("Output ceiling the limiter never exceeds (keep around -1 dB for streaming)");
    releaseFader.setTooltip ("How fast the limiter recovers: short = louder but can distort, long = cleaner but pumps");

    for (auto* fader : { &clipFader, &gainFader, &ceilingFader, &releaseFader })
        addAndMakeVisible (*fader);

    addAndMakeVisible (panelHistoryView);
    panelHistoryView.setTooltip ("Last 3 s: grey = input, red = clipped away, amber = limited away. Open the Master tab for detail");

    addAndMakeVisible (clipReductionMeter);
    addAndMakeVisible (limitReductionMeter);
    clipReductionMeter.setTooltip ("Clipper gain reduction: how far above the threshold the loudest peaks are");
    limitReductionMeter.setTooltip ("Limiter gain reduction: how hard the limiter is pulling peaks down to the ceiling");

    const char* units[4] = { "dB", "dB", "dB", "ms" };
    const char* captions[4] = { "THRESHOLD", "GAIN", "CEIL", "REL" };
    VerticalFader* faders[4] = { &clipFader, &gainFader, &ceilingFader, &releaseFader };

    for (int i = 0; i < 4; ++i)
    {
        faderReadouts[(size_t) i] = std::make_unique<ValueReadout> (*faders[i], units[i], captions[i]);
        addAndMakeVisible (*faderReadouts[(size_t) i]);
    }

    // Dim a section's controls while it is bypassed.
    auto applyBypassDimming = [this]
    {
        const float clipAlpha = clipPower.getToggleState() ? 1.0f : 0.4f;
        const float limitAlpha = limitPower.getToggleState() ? 1.0f : 0.4f;

        clipFader.setAlpha (clipAlpha);
        clipReductionMeter.setAlpha (clipAlpha);
        faderReadouts[0]->setAlpha (clipAlpha);

        gainFader.setAlpha (limitAlpha);
        ceilingFader.setAlpha (limitAlpha);
        releaseFader.setAlpha (limitAlpha);
        limitReductionMeter.setAlpha (limitAlpha);

        for (int i = 1; i < 4; ++i)
            faderReadouts[(size_t) i]->setAlpha (limitAlpha);
    };

    clipPower.onStateChange = applyBypassDimming;
    limitPower.onStateChange = applyBypassDimming;

    auto& params = processorRef.getParameters();
    matchAttachment = std::make_unique<ButtonAttachment> (params, "matchOn", matchButton);
    clipOnAttachment = std::make_unique<ButtonAttachment> (params, "clipOn", clipPower);
    clipThresholdAttachment = std::make_unique<SliderAttachment> (params, "clipThreshold", clipFader);
    limitOnAttachment = std::make_unique<ButtonAttachment> (params, "limitOn", limitPower);
    limitGainAttachment = std::make_unique<SliderAttachment> (params, "limitGain", gainFader);
    limitCeilingAttachment = std::make_unique<SliderAttachment> (params, "limitCeiling", ceilingFader);
    limitReleaseAttachment = std::make_unique<SliderAttachment> (params, "limitRelease", releaseFader);

    applyBypassDimming();

    startTimerHz (10);
    meterTimer.startTimerHz (30);

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

    LF::paintBackground (g, getLocalBounds());

    // Section backgrounds.
    g.setColour (LF::panel.withAlpha (0.55f));
    g.fillRect (monitorArea);
    g.fillRect (bottomStripArea);

    // Engraved rules between sections.
    LF::paintRule (g, { 0.0f, headerArea.getBottom() - 1.0f, (float) getWidth(), 2.0f }, false);
    LF::paintRule (g, { 0.0f, monitorArea.getBottom() - 1.0f, (float) getWidth(), 2.0f }, false);
    LF::paintRule (g, { 0.0f, bottomStripArea.getY() - 1.0f, (float) getWidth(), 2.0f }, false);
    LF::paintRule (g, { loudnessArea.getRight() - 1.0f, bottomStripArea.getY(), 2.0f, bottomStripArea.getHeight() }, true);
    LF::paintRule (g, { stereoArea.getRight() - 1.0f, bottomStripArea.getY(), 2.0f, bottomStripArea.getHeight() }, true);

    // Header: title divider, file icon, load-button icon, settings gear.
    LF::paintRule (g, { 112.0f, headerArea.getY() + 17.0f, 2.0f, 22.0f }, true);

    if (audioLinesIcon != nullptr)
        audioLinesIcon->drawWithin (g, fileIconArea, juce::RectanglePlacement::centred, 1.0f);

    if (folderIcon != nullptr)
        folderIcon->drawWithin (g, loadIconArea, juce::RectanglePlacement::centred, 1.0f);

    if (settingsIcon != nullptr)
        settingsIcon->drawWithin (g, settingsIconArea, juce::RectanglePlacement::centred, 0.9f);

    // Monitoring strip: divider, transport pause icon + progress bar.
    LF::paintRule (g, { 211.0f, monitorArea.getY() + 15.0f, 2.0f, 34.0f }, true);

    if (pauseIcon != nullptr)
        pauseIcon->drawWithin (g, transportIconArea, juce::RectanglePlacement::centred,
                               processorRef.isHostPlaying() ? 1.0f : 0.4f);

    // Recessed progress well with accent fill.
    LF::paintRecessed (g, progressBarArea.expanded (0.0f, 1.0f), 2.5f);

    if (referenceProgress > 0.0f)
    {
        g.setColour (LF::accent);
        g.fillRoundedRectangle (progressBarArea.expanded (0.0f, 1.0f)
                                    .withWidth ((progressBarArea.getWidth())
                                                * juce::jlimit (0.0f, 1.0f, referenceProgress)),
                                2.0f);
    }

    // Toolbar legend: trace chips next to the title (master view carries its own).
    if (currentView != MainView::masterHistory)
    {
        auto remaining = legendArea;
        g.setFont (ParityLookAndFeel::getLabelFont (9.0f));

        for (auto entry : { std::pair<juce::Colour, const char*> { LF::inkSoft, "MIX" },
                            { LF::accent, "REF" } })
        {
            auto chip = remaining.removeFromLeft (49.0f);
            g.setColour (entry.first);
            g.fillRoundedRectangle (chip.getX(), chip.getCentreY() - 1.0f, 16.0f, 2.0f, 1.0f);
            g.setColour (LF::inkFaint);
            g.drawText (entry.second, chip.withTrimmedLeft (22.0f), juce::Justification::centredLeft);
        }
    }

    // Master panel card with soft drop shadow.
    if (! masterPanelArea.isEmpty())
    {
        LF::paintCard (g, masterPanelArea, 8.0f);

        LF::paintRule (g, masterDivider.expanded (0.0f, 0.5f), false);
        LF::paintRule (g, masterColumnDivider.expanded (0.5f, 0.0f), true);
    }
}

void ParityAudioProcessorEditor::paintOverChildren (juce::Graphics& g)
{
    using LF = ParityLookAndFeel;

    // Glowing LED inside the MATCH toggle when loudness matching is on.
    if (matchButton.getToggleState())
    {
        const auto b = matchButton.getBounds().toFloat();
        LF::paintLed (g, { b.getX() + 14.0f, b.getCentreY() }, 3.0f, LF::accent, true);
    }

    // Accent underline under the active view tab.
    juce::TextButton* activeTab = nullptr;

    switch (currentView)
    {
        case MainView::spectrumOverlay:    activeTab = &overlayButton;    break;
        case MainView::spectrumDifference: activeTab = &differenceButton; break;
        case MainView::loudnessHistory:    activeTab = &loudnessButton;   break;
        case MainView::masterHistory:      activeTab = &masterButton;     break;
    }

    if (activeTab != nullptr)
    {
        const auto b = activeTab->getBounds().toFloat();
        juce::Rectangle<float> underline (b.getX() + 6.0f, b.getBottom() - 4.0f, b.getWidth() - 12.0f, 2.0f);
        g.setColour (LF::accent);
        g.fillRoundedRectangle (underline, 1.0f);
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
    constexpr float panelWidth = 300.0f;
    masterPanelArea = { (float) width - 24.0f - panelWidth, panelTop,
                        panelWidth, (float) height - panelTop };

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
    masterButton.setBounds (realtimeButton.getX() - 16 - 64, toolbarY + 8, 64, 32);
    loudnessButton.setBounds (masterButton.getX() - 4 - 76, toolbarY + 8, 76, 32);
    differenceButton.setBounds (loudnessButton.getX() - 4 - 79, toolbarY + 8, 79, 32);
    overlayButton.setBounds (differenceButton.getX() - 4 - 66, toolbarY + 8, 66, 32);

    //==============================================================================
    // Main view (plot + axis gutters live inside each component)
    const juce::Rectangle<int> viewBounds (24, toolbarY + 48,
                                           juce::jmax (100, (int) masterPanelArea.getX() - 24 - 16 - 24),
                                           juce::jmax (80, (int) bottomStripArea.getY() - toolbarY - 48 - 12));
    spectrumView.setBounds (viewBounds);
    loudnessHistoryView.setBounds (viewBounds);
    masterHistoryView.setBounds (viewBounds);

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

            auto mixColumn = rowArea.removeFromLeft (84);
            auto refColumn = rowArea.removeFromLeft (84);

            if (row == numLoudnessRows - 1)
            {
                // Latching clip LEDs sit at the right edge of each peak column.
                mixPeakLed.setBounds (mixColumn.removeFromRight (16).withSizeKeepingCentre (10, 16));
                refPeakLed.setBounds (refColumn.removeFromRight (16).withSizeKeepingCentre (10, 16));
            }

            mixValueLabels[(size_t) row].setBounds (mixColumn.withTrimmedTop (7));
            refValueLabels[(size_t) row].setBounds (refColumn.withTrimmedTop (7));
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

        auto titleRow = content.removeFromTop (14);
        masterTitleLabel.setBounds (titleRow.removeFromLeft (60));
        clipLed.setBounds (titleRow.removeFromLeft (18).withSizeKeepingCentre (12, 14));
        content.removeFromTop (10);
        masterDivider = content.removeFromTop (1).toFloat();
        content.removeFromTop (12);

        // Compact activity strip: what's being clipped / limited right now.
        panelHistoryView.setBounds (content.removeFromTop (juce::jmax (40, content.getHeight() / 6)));
        content.removeFromTop (12);

        auto processorRow = content;

        // Fixed bottom row: ValueReadout chips (chip + caption inside).
        auto readoutRow = processorRow.removeFromBottom (40);
        processorRow.removeFromBottom (8);

        // Shared header row: processor name + power toggle in each column.
        auto headerRow = processorRow.removeFromTop (16);
        processorRow.removeFromTop (8);

        constexpr int clipColumnWidth = 96, reductionWidth = 26, faderGap = 4;

        auto clipHeader = headerRow.removeFromLeft (clipColumnWidth);
        clipNameLabel.setBounds (clipHeader.removeFromLeft (36));
        clipPower.setBounds (clipHeader.withTrimmedLeft (4).withSizeKeepingCentre (30, 14));

        auto limitHeader = headerRow.withTrimmedLeft (9);
        limitNameLabel.setBounds (limitHeader.removeFromLeft (40));
        limitPower.setBounds (limitHeader.withTrimmedLeft (4).withSizeKeepingCentre (30, 14));

        // CLIP column: fader + GR meter, vertical divider, then LIMIT.
        auto clipColumn = processorRow.removeFromLeft (clipColumnWidth);
        auto clipReductionArea = clipColumn.removeFromRight (reductionWidth);
        clipColumn.removeFromRight (faderGap);

        processorRow.removeFromLeft (4);
        masterColumnDivider = { (float) processorRow.getX(), (float) processorRow.getY() + 16.0f,
                                1.0f, (float) processorRow.getHeight() - 16.0f };
        processorRow.removeFromLeft (5);

        auto limitReductionArea = processorRow.removeFromRight (reductionWidth);
        auto limitColumn = processorRow;

        // Faders fill the remaining height; GR meters sit alongside, same height.
        clipFader.setBounds (clipColumn.withSizeKeepingCentre (48, clipColumn.getHeight()));
        clipReductionMeter.setBounds (clipReductionArea);
        limitReductionMeter.setBounds (limitReductionArea);

        const auto slotWidth = limitColumn.getWidth() / 3;
        std::array<juce::Rectangle<int>, 3> limitSlots;

        for (int i = 0; i < 3; ++i)
            limitSlots[(size_t) i] = limitColumn.removeFromLeft (slotWidth);

        gainFader.setBounds (limitSlots[0].withSizeKeepingCentre (40, limitSlots[0].getHeight()));
        ceilingFader.setBounds (limitSlots[1].withSizeKeepingCentre (40, limitSlots[1].getHeight()));
        releaseFader.setBounds (limitSlots[2].withSizeKeepingCentre (40, limitSlots[2].getHeight()));

        // ValueReadout chips under each fader column.
        faderReadouts[0]->setBounds (clipColumn.getX() - 10, readoutRow.getY(),
                                     clipColumn.getWidth() + 20, 40);

        for (int i = 0; i < 3; ++i)
            faderReadouts[(size_t) i + 1]->setBounds (limitSlots[(size_t) i].withTrimmedTop (0)
                                                    .withY (readoutRow.getY()).withHeight (40));
    }
}

void ParityAudioProcessorEditor::showView (MainView view)
{
    currentView = view;

    const auto spectrum = view == MainView::spectrumOverlay || view == MainView::spectrumDifference;

    if (spectrum)
        spectrumView.setDisplay (view == MainView::spectrumOverlay ? SpectrumView::Display::overlay
                                                                   : SpectrumView::Display::difference);

    spectrumView.setVisible (spectrum);
    loudnessHistoryView.setVisible (view == MainView::loudnessHistory);
    masterHistoryView.setVisible (view == MainView::masterHistory);

    // Live/Avg smoothing only applies to the spectrum.
    realtimeButton.setEnabled (spectrum);
    averageButton.setEnabled (spectrum);

    spectrumTitleLabel.setText (spectrum ? "Spectrum"
                              : view == MainView::loudnessHistory ? "Loudness"
                                                                  : "Master",
                                juce::dontSendNotification);
    repaint();
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

    // Fader meters show each stage's *input* so overshoot past the handle is
    // exactly what the stage removes; GR meters show how much.
    loudnessHistoryView.setReferenceIntegratedLufs (processorRef.getReferenceFileStats().integratedLufs);

    updateLoudnessLabels();
    updateStereoLabels();
}

//==============================================================================
// 30 Hz meter path — fader levels, GR meters, latching LEDs. Text labels stay
// on the 10 Hz timer so the animation path never does string work.
void ParityAudioProcessorEditor::meterTick()
{
    const auto& chain = processorRef.getMasterChain();
    const auto clipActive = clipPower.getToggleState();
    const auto limitActive = limitPower.getToggleState();

    clipFader.setMeterLevel (chain.getClipInputLevel());
    clipFader.setProcessingActive (clipActive);
    ceilingFader.setMeterLevel (chain.getPostClipLevel());
    ceilingFader.setProcessingActive (limitActive);

    clipReductionMeter.setReductionDb (chain.getClipReductionDb());
    clipReductionMeter.setActive (clipActive);
    limitReductionMeter.setReductionDb (chain.getLimitReductionDb());
    limitReductionMeter.setActive (limitActive);

    // Latch the master-header LED while the clipper is actually working.
    if (clipActive && chain.getClipReductionDb() > 0.1f)
        clipLed.trigger();

    // Latch true-peak LEDs on intersample overs.
    if (processorRef.getMixLoudness().getTruePeakDb() > -0.1f)
        mixPeakLed.trigger();

    if (processorRef.getReferenceFileStats().truePeakDb > -0.1f)
        refPeakLed.trigger();
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
                return juce::String::fromUTF8 ("\xe2\x80\x94");

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
            delta.setText (juce::String::fromUTF8 ("\xe2\x80\x94"), juce::dontSendNotification);
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
            return juce::String::fromUTF8 ("\xe2\x80\x94");

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
            delta.setText (juce::String::fromUTF8 ("\xe2\x80\x94"), juce::dontSendNotification);
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

    // Red stays for over-ceiling values; the LEDs themselves latch in meterTick.
    if (mix.getTruePeakDb() > 0.0f || fileStats.truePeakDb > 0.0f)
        repaint();
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
