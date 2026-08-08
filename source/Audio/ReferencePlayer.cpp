#include "ReferencePlayer.h"

ReferencePlayer::ReferencePlayer()
{
    formatManager.registerBasicFormats();
}

bool ReferencePlayer::loadFile (const juce::File& file)
{
    std::unique_ptr<juce::AudioFormatReader> reader (formatManager.createReaderFor (file));

    if (reader == nullptr || reader->lengthInSamples <= 0)
        return false;

    auto newFile = std::make_shared<LoadedFile>();
    newFile->sourceFile = file;
    newFile->sampleRate = reader->sampleRate;

    const auto numSamples = (int) reader->lengthInSamples;
    const auto numChannels = juce::jmin ((int) reader->numChannels, 2);

    newFile->audio.setSize (numChannels, numSamples);

    if (! reader->read (&newFile->audio, 0, numSamples, 0, true, numChannels > 1))
        return false;

    std::atomic_store (&loadedFile, std::move (newFile));
    return true;
}

void ReferencePlayer::clear()
{
    std::atomic_store (&loadedFile, std::shared_ptr<LoadedFile>());
}

void ReferencePlayer::process (juce::AudioBuffer<float>& buffer,
                               double playheadTimeSeconds,
                               double hostSampleRate,
                               bool isPlaying) noexcept
{
    buffer.clear();

    auto file = std::atomic_load (&loadedFile);

    if (file == nullptr || ! isPlaying || playheadTimeSeconds < 0.0 || hostSampleRate <= 0.0)
        return;

    const auto& source = file->audio;
    const auto sourceLength = source.getNumSamples();
    const auto numOutChannels = buffer.getNumChannels();

    // Host time -> file sample position; the ratio handles any sample-rate
    // mismatch between the file and the host.
    double readPos = playheadTimeSeconds * file->sampleRate;
    const double increment = file->sampleRate / hostSampleRate;

    for (int i = 0; i < buffer.getNumSamples(); ++i)
    {
        const auto index = (int) readPos;

        if (index + 1 >= sourceLength)
            break;

        const auto frac = (float) (readPos - (double) index);

        for (int ch = 0; ch < numOutChannels; ++ch)
        {
            const auto* src = source.getReadPointer (juce::jmin (ch, source.getNumChannels() - 1));
            const auto sample = src[index] + frac * (src[index + 1] - src[index]);
            buffer.setSample (ch, i, sample);
        }

        readPos += increment;
    }
}
