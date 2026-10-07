#pragma once

#include "PluginProcessor.h"
#include "UI/ABSwitch.h"
#include "UI/ComparisonBar.h"
#include "UI/ParityLookAndFeel.h"
#include "UI/SpectrumView.h"
#include "UI/VerticalFader.h"

//==============================================================================
/**
    Figma "Parity · Comparison" layout: header + monitoring strip on top,
    spectrum plot in the middle, loudness/stereo analysis strip at the
    bottom, and a floating master-processors panel on the right with
    vertical faders.
*/
class ParityAudioProcessorEditor final : public juce::AudioProcessorEditor,
                                         private juce::Timer
{
public:
    explicit ParityAudioProcessorEditor (ParityAudioProcessor&);
    ~ParityAudioProcessorEditor() override;

    //==============================================================================
    void paint (juce::Graphics&) override;
    void paintOverChildren (juce::Graphics&) override;
    void resized() override;

private:
    //==============================================================================
    /** Small pill toggle used for the CLIP/LIMIT bypass indicators. */
    class SectionPill final : public juce::Button
    {
    public:
        SectionPill() : juce::Button ({}) { setClickingTogglesState (true); }

        void paintButton (juce::Graphics& g, bool highlighted, bool) override
        {
            auto bounds = getLocalBounds().toFloat().reduced (0.5f);

            g.setColour (ParityLookAndFeel::control.brighter (highlighted ? 0.08f : 0.0f));
            g.fillRoundedRectangle (bounds, 3.0f);
            g.setColour (ParityLookAndFeel::line);
            g.drawRoundedRectangle (bounds, 3.0f, 1.0f);

            const auto on = getToggleState();
            g.setColour (on ? ParityLookAndFeel::accent : ParityLookAndFeel::inkFaint);
            g.setFont (ParityLookAndFeel::getFont (8.0f, juce::Font::bold));
            g.drawText (on ? "ON" : "BYP", bounds, juce::Justification::centred);
        }
    };

    //==============================================================================
    void timerCallback() override;
    void loadButtonClicked();
    void updateFileLabel();
    void updateLoudnessLabels();
    void updateStereoLabels();
    void updateMasterReadouts();

    // This reference is provided as a quick way for your editor to
    // access the processor object that created it.
    ParityAudioProcessor& processorRef;

    ParityLookAndFeel lookAndFeel;

    //==============================================================================
    // Header strip
    juce::Label titleLabel;
    juce::Label fileLabel;
    juce::TextButton loadButton { "Load Reference" };
    std::unique_ptr<juce::Drawable> audioLinesIcon, folderIcon, settingsIcon,
                                    pauseIcon, checkIcon;

    // Monitoring strip
    ABSwitch abSwitch;
    juce::TextButton matchButton { "MATCH" };
    float referenceProgress = 0.0f;

    // Spectrum toolbar + view
    juce::Label spectrumTitleLabel;
    SpectrumView spectrumView;
    juce::TextButton overlayButton { "Overlay" }, differenceButton { "Difference" };
    juce::TextButton realtimeButton { "Live" }, averageButton { "Avg" };

    // Loudness table
    juce::Label loudnessTitleLabel;
    static constexpr int numLoudnessRows = 4; // momentary, short-term, integrated, true peak
    std::array<juce::Label, numLoudnessRows> rowLabels;
    std::array<juce::Label, numLoudnessRows> mixValueLabels, refValueLabels, deltaLabels;
    juce::Label mixHeader, refHeader, deltaHeader;
    juce::Label peakWarningLabel;

    // Stereo section
    juce::Label stereoTitleLabel;
    juce::Label stereoMixHeader, stereoRefHeader, stereoDeltaHeader;
    juce::Label correlationNameLabel, widthNameLabel;
    std::array<juce::Label, 2> stereoMixValues, stereoRefValues, stereoDeltaValues;
    ComparisonBar correlationBar, widthBar;

    // Master processors panel
    juce::Label masterTitleLabel;
    juce::Label clipNameLabel, limitNameLabel;
    SectionPill clipPill, limitPill;
    VerticalFader clipFader { true, 12 }, gainFader { false, 10 },
                  ceilingFader { false, 10 }, releaseFader { true, 10 };
    std::array<juce::Label, 4> faderValueLabels, faderUnitLabels, faderCaptionLabels;

    using SliderAttachment = juce::AudioProcessorValueTreeState::SliderAttachment;
    using ButtonAttachment = juce::AudioProcessorValueTreeState::ButtonAttachment;
    std::unique_ptr<ButtonAttachment> matchAttachment, clipOnAttachment, limitOnAttachment;
    std::unique_ptr<SliderAttachment> clipThresholdAttachment, limitGainAttachment,
                                      limitCeilingAttachment, limitReleaseAttachment;

    //==============================================================================
    // Painted regions (dividers, icons, fills) computed in resized()
    juce::Rectangle<float> headerArea, monitorArea, bottomStripArea,
                           loudnessArea, stereoArea, masterPanelArea,
                           fileIconArea, loadIconArea, settingsIconArea,
                           transportIconArea, progressBarArea,
                           legendArea, masterDivider, masterColumnDivider;

    juce::TooltipWindow tooltipWindow { this };
    std::unique_ptr<juce::FileChooser> fileChooser;
    ParityAudioProcessor::ReferenceLoadStatus lastLoadStatus = ParityAudioProcessor::ReferenceLoadStatus::idle;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ParityAudioProcessorEditor)
};
