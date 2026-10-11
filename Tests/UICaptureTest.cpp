// Visual-check harness: renders the plugin editor offscreen to PNGs in the
// working directory (ui.png, ui-loudness.png, ui-master.png).

#include "PluginProcessor.h"

#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_audio_utils/juce_audio_utils.h>

namespace
{
    void pumpMessages (int iterations)
    {
        auto* mm = juce::MessageManager::getInstance();
        for (int i = 0; i < iterations; ++i)
            mm->runDispatchLoopUntil (25);
    }

    bool capture (juce::Component& editor, const juce::String& fileName)
    {
        juce::Image img (juce::Image::ARGB, editor.getWidth(), editor.getHeight(), true);
        juce::Graphics g (img);
        editor.paintEntireComponent (g, false);

        auto outFile = juce::File::getCurrentWorkingDirectory().getChildFile (fileName);
        outFile.deleteFile();
        juce::FileOutputStream out (outFile);

        juce::PNGImageFormat png;
        const auto ok = png.writeImageToStream (img, out);
        juce::Logger::writeToLog (ok ? "wrote " + outFile.getFullPathName() : "PNG write failed: " + fileName);
        return ok;
    }

    juce::TextButton* findButton (juce::Component& root, const juce::String& text)
    {
        for (auto* child : root.getChildren())
            if (auto* button = dynamic_cast<juce::TextButton*> (child))
                if (button->getButtonText() == text)
                    return button;

        return nullptr;
    }
}

int main()
{
    juce::ScopedJuceInitialiser_GUI init;

    // Fake transport so analyzers and the progress bar have real content.
    struct FakePlayHead final : juce::AudioPlayHead
    {
        juce::Optional<PositionInfo> getPosition() const override
        {
            PositionInfo info;
            info.setTimeInSamples (samplePos);
            info.setTimeInSeconds ((double) samplePos / 48000.0);
            info.setIsPlaying (true);
            return info;
        }

        juce::int64 samplePos = 0;
    } playHead;

    ParityAudioProcessor proc;
    proc.setPlayHead (&playHead);
    proc.setPlayConfigDetails (2, 2, 48000.0, 512);
    proc.prepareToPlay (48000.0, 512);

    // Engage both stages so the displays have something to show.
    auto& params = proc.getParameters();
    auto setParam = [&params] (const char* id, float plainValue)
    {
        auto* p = params.getParameter (id);
        p->setValueNotifyingHost (p->convertTo0to1 (plainValue));
    };

    setParam ("clipOn", 1.0f);
    setParam ("clipThreshold", -2.0f);
    setParam ("limitOn", 1.0f);
    setParam ("limitGain", 3.0f);
    setParam ("limitCeiling", -5.0f);

    // Tone + periodic drum-like bursts so some slices clip and some don't.
    juce::AudioBuffer<float> buffer (2, 512);
    juce::MidiBuffer midi;
    juce::Random random (42);

    for (int block = 0; block < 500; ++block)
    {
        for (int i = 0; i < 512; ++i)
        {
            const auto n = (double) (block * 512 + i);
            const auto beatPhase = std::fmod (n, 24000.0) / 24000.0; // 2 Hz
            const auto burst = std::exp (-beatPhase * 14.0);

            const auto tone = 0.25 * std::sin (n * 2.0 * juce::MathConstants<double>::pi * 110.0 / 48000.0)
                            + 0.08 * std::sin (n * 2.0 * juce::MathConstants<double>::pi * 3000.0 / 48000.0);
            const auto sample = (float) (tone + burst * 0.9 * (random.nextFloat() * 2.0f - 1.0f));

            buffer.setSample (0, i, sample);
            buffer.setSample (1, i, 0.9f * sample);
        }

        proc.processBlock (buffer, midi);
        playHead.samplePos += 512;
    }

    std::unique_ptr<juce::AudioProcessorEditor> editor (proc.createEditor());
    editor->setSize (1100, 750);
    pumpMessages (60);

    bool ok = capture (*editor, "ui.png");

    if (auto* loudness = findButton (*editor, "Loudness"))
    {
        loudness->triggerClick();
        pumpMessages (20);
        ok = capture (*editor, "ui-loudness.png") && ok;
    }

    if (auto* master = findButton (*editor, "Master"))
    {
        master->triggerClick();
        pumpMessages (20);
        ok = capture (*editor, "ui-master.png") && ok;
    }

    editor.reset();
    return ok ? 0 : 1;
}
