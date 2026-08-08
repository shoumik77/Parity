#pragma once

#include <juce_audio_formats/juce_audio_formats.h>

//==============================================================================
/**
    Loads a reference audio file into memory and renders it synced to the
    host playhead. File loading happens on the message thread; the loaded
    buffer is swapped in atomically so the audio thread never sees a
    partially-loaded file.
*/
class ReferencePlayer
{
public:
    ReferencePlayer();

    /** Attempts to load the given audio file. Returns true on success.
        Call from the message thread only. */
    bool loadFile (const juce::File& file);

    /** Clears the currently loaded file. Call from the message thread only. */
    void clear();

    bool hasFileLoaded() const noexcept   { return std::atomic_load (&loadedFile) != nullptr; }
    juce::File getFile() const            { auto f = std::atomic_load (&loadedFile); return f != nullptr ? f->sourceFile : juce::File(); }

    /** Returns a wildcard pattern of supported formats for use with a FileChooser. */
    juce::String getWildcardPattern() const  { return formatManager.getWildcardForAllFormats(); }

    /** Renders the reference into the buffer (replacing its contents), reading
        from the file position corresponding to the host playhead time.
        Fills with silence if stopped, nothing is loaded, or past EOF.
        Safe to call from the audio thread. */
    void process (juce::AudioBuffer<float>& buffer,
                  double playheadTimeSeconds,
                  double hostSampleRate,
                  bool isPlaying) noexcept;

private:
    struct LoadedFile
    {
        juce::File sourceFile;
        juce::AudioBuffer<float> audio;
        double sampleRate = 0.0;
    };

    juce::AudioFormatManager formatManager;
    std::shared_ptr<LoadedFile> loadedFile;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ReferencePlayer)
};
