#pragma once

#include "PluginProcessor.h"
#include "UI/ABSwitch.h"
#include "UI/ComparisonBar.h"
#include "UI/GainReductionMeter.h"
#include "UI/LoudnessHistoryView.h"
#include "UI/MasterHistoryView.h"
#include "UI/ParityLookAndFeel.h"
#include "UI/PowerButton.h"
#include "UI/SpectrumView.h"
#include "UI/ValueReadout.h"
#include "UI/VerticalFader.h"

//==============================================================================
/**
    Figma "Parity · Comparison" layout: header + monitoring strip on top,
    a tabbed main view (spectrum / loudness history / master history) in the
    middle, loudness/stereo analysis strip at the bottom, and a floating
    master-processors panel on the right with vertical faders, GR meters and
    an activity strip.
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
    /** Small LED that latches on and clears when clicked. */
    class LatchedLed final : public juce::Component,
                             public juce::SettableTooltipClient
    {
    public:
        void trigger()
        {
            if (! lit)
            {
                lit = true;
                repaint();
            }
        }

        void paint (juce::Graphics& g) override
        {
            ParityLookAndFeel::paintLed (g, getLocalBounds().toFloat().getCentre(), 3.5f,
                                         ParityLookAndFeel::warn, lit);
        }

        void mouseDown (const juce::MouseEvent&) override
        {
            if (lit)
            {
                lit = false;
                repaint();
            }
        }

    private:
        bool lit = false;
    };

    //==============================================================================
    /** 30 Hz timer for meters/faders/LEDs; the editor's own Timer runs the
        10 Hz text updates. Keeps label churn off the animation path. */
    struct MeterTimer final : juce::Timer
    {
        explicit MeterTimer (ParityAudioProcessorEditor& o) : owner (o) {}
        void timerCallback() override  { owner.meterTick(); }
        ParityAudioProcessorEditor& owner;
    };

    void meterTick();

    //==============================================================================
    void timerCallback() override;
    void loadButtonClicked();
    void updateFileLabel();
    void updateLoudnessLabels();
    void updateStereoLabels();

    enum class MainView { spectrumOverlay, spectrumDifference, loudnessHistory, masterHistory };
    void showView (MainView view);

    // This reference is provided as a quick way for your editor to
    // access the processor object that created it.
    ParityAudioProcessor& processorRef;

    ParityLookAndFeel lookAndFeel;
    MeterTimer meterTimer { *this };

    //==============================================================================
    // Header strip
    juce::Label titleLabel;
    juce::Label fileLabel;
    juce::TextButton loadButton { "Load Reference" };
    std::unique_ptr<juce::Drawable> audioLinesIcon, folderIcon, settingsIcon, pauseIcon;

    // Monitoring strip
    ABSwitch abSwitch;
    juce::TextButton matchButton { "MATCH" };
    float referenceProgress = 0.0f;

    // Main view toolbar + the three swappable views
    juce::Label spectrumTitleLabel;
    SpectrumView spectrumView;
    LoudnessHistoryView loudnessHistoryView;
    MasterHistoryView masterHistoryView;
    juce::TextButton overlayButton { "Overlay" }, differenceButton { "Difference" },
                     loudnessButton { "Loudness" }, masterButton { "Master" };
    juce::TextButton realtimeButton { "Live" }, averageButton { "Avg" };
    MainView currentView = MainView::spectrumOverlay;

    // Loudness table
    juce::Label loudnessTitleLabel;
    static constexpr int numLoudnessRows = 4; // momentary, short-term, integrated, true peak
    std::array<juce::Label, numLoudnessRows> rowLabels;
    std::array<juce::Label, numLoudnessRows> mixValueLabels, refValueLabels, deltaLabels;
    juce::Label mixHeader, refHeader, deltaHeader;
    LatchedLed mixPeakLed, refPeakLed;

    // Stereo section
    juce::Label stereoTitleLabel;
    juce::Label stereoMixHeader, stereoRefHeader, stereoDeltaHeader;
    juce::Label correlationNameLabel, widthNameLabel;
    std::array<juce::Label, 2> stereoMixValues, stereoRefValues, stereoDeltaValues;
    ComparisonBar correlationBar, widthBar;

    // Master processors panel
    juce::Label masterTitleLabel;
    juce::Label clipNameLabel, limitNameLabel;
    LatchedLed clipLed;
    PowerButton clipPower, limitPower;
    MasterHistoryView panelHistoryView;
    VerticalFader clipFader { true, 12 }, gainFader { false, 10 },
                  ceilingFader { true, 10 }, releaseFader { false, 10 };
    GainReductionMeter clipReductionMeter, limitReductionMeter;
    std::array<std::unique_ptr<ValueReadout>, 4> faderReadouts;

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
