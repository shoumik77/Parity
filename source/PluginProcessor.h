#pragma once

#include <juce_audio_processors/juce_audio_processors.h>

#include "Audio/LoudnessAnalyzer.h"
#include "Audio/ReferencePlayer.h"
#include "Audio/SpectrumAnalyzer.h"
#include "Audio/StereoAnalyzer.h"

//==============================================================================
class ParityAudioProcessor final : public juce::AudioProcessor
{
public:
    //==============================================================================
    ParityAudioProcessor();
    ~ParityAudioProcessor() override;

    //==============================================================================
    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;

    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    using AudioProcessor::processBlock;

    //==============================================================================
    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override;

    //==============================================================================
    const juce::String getName() const override;

    bool acceptsMidi() const override;
    bool producesMidi() const override;
    bool isMidiEffect() const override;
    double getTailLengthSeconds() const override;

    //==============================================================================
    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram (int index) override;
    const juce::String getProgramName (int index) override;
    void changeProgramName (int index, const juce::String& newName) override;

    //==============================================================================
    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    //==============================================================================
    enum class ReferenceLoadStatus { idle, loading, loaded, failed };

    /** Kicks off decoding and analysis of a reference file on a background
        thread. Poll getReferenceLoadStatus() for the outcome. Message thread only. */
    void loadReferenceFileAsync (const juce::File& file);

    ReferenceLoadStatus getReferenceLoadStatus() const noexcept  { return loadStatus.load(); }

    /** Acknowledges a failed load, returning the status to idle. */
    void clearReferenceLoadFailure() noexcept
    {
        auto expected = ReferenceLoadStatus::failed;
        loadStatus.compare_exchange_strong (expected, ReferenceLoadStatus::idle);
    }

    ReferencePlayer& getReferencePlayer() noexcept          { return referencePlayer; }

    void setReferenceActive (bool shouldBeActive) noexcept  { referenceActive.store (shouldBeActive); }
    bool isReferenceActive() const noexcept                 { return referenceActive.load(); }

    const LoudnessAnalyzer& getMixLoudness() const noexcept        { return mixLoudness; }
    const LoudnessAnalyzer& getReferenceLoudness() const noexcept  { return referenceLoudness; }

    SpectrumAnalyzer& getMixSpectrum() noexcept                    { return mixSpectrum; }
    SpectrumAnalyzer& getReferenceSpectrum() noexcept              { return referenceSpectrum; }

    const StereoAnalyzer& getMixStereo() const noexcept            { return mixStereo; }
    const StereoAnalyzer& getReferenceStereo() const noexcept      { return referenceStereo; }

    /** Full-file stats computed offline when the reference was loaded. */
    LoudnessAnalyzer::Stats getReferenceFileStats() const noexcept
    {
        return { referenceFileLufs.load(), referenceFilePeak.load() };
    }

private:
    //==============================================================================
    ReferencePlayer referencePlayer;
    std::atomic<bool> referenceActive { false };

    LoudnessAnalyzer mixLoudness, referenceLoudness;
    SpectrumAnalyzer mixSpectrum, referenceSpectrum;
    StereoAnalyzer mixStereo, referenceStereo;
    std::atomic<float> referenceFileLufs { LoudnessAnalyzer::silenceLufs };
    std::atomic<float> referenceFilePeak { LoudnessAnalyzer::silenceLufs };

    juce::ThreadPool loadPool { 1 };
    std::atomic<ReferenceLoadStatus> loadStatus { ReferenceLoadStatus::idle };
    std::atomic<int> loadGeneration { 0 };

    juce::AudioBuffer<float> referenceBuffer;
    juce::SmoothedValue<float> referenceGain { 0.0f };
    double hostSampleRate = 44100.0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ParityAudioProcessor)
};
