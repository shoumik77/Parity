// Temporary visual-check harness: renders the plugin editor offscreen to
// ui.png in the working directory. Not meant to be committed long-term.

#include "PluginProcessor.h"

#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_audio_utils/juce_audio_utils.h>

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

    // Feed a sine through the processor so the analyzers have real content.
    juce::AudioBuffer<float> buffer (2, 512);
    juce::MidiBuffer midi;

    for (int block = 0; block < 400; ++block)
    {
        for (int i = 0; i < 512; ++i)
        {
            const auto n = (double) (block * 512 + i);
            const auto sample = (float) (0.2 * std::sin (n * 2.0 * juce::MathConstants<double>::pi * 220.0 / 48000.0)
                                       + 0.08 * std::sin (n * 2.0 * juce::MathConstants<double>::pi * 3000.0 / 48000.0));
            buffer.setSample (0, i, sample);
            buffer.setSample (1, i, (float) (0.9 * sample));
        }

        proc.processBlock (buffer, midi);
        playHead.samplePos += 512;
    }

    std::unique_ptr<juce::AudioProcessorEditor> editor (proc.createEditor());
    editor->setSize (1100, 750);

    // Pump the message loop so timer callbacks populate labels/meters.
    auto* mm = juce::MessageManager::getInstance();
    for (int i = 0; i < 60; ++i)
        mm->runDispatchLoopUntil (25);

    juce::Image img (juce::Image::ARGB, 1100, 750, true);
    juce::Graphics g (img);
    editor->paintEntireComponent (g, false);
    editor.reset();

    juce::File outFile = juce::File::getCurrentWorkingDirectory().getChildFile ("ui.png");
    juce::FileOutputStream out (outFile);
    out.setPosition (0);
    out.truncate();

    juce::PNGImageFormat png;
    const auto ok = png.writeImageToStream (img, out);

    juce::Logger::writeToLog (ok ? "wrote " + outFile.getFullPathName() : "PNG write failed");
    return ok ? 0 : 1;
}
