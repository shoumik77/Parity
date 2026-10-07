#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
ParityAudioProcessor::ParityAudioProcessor()
     : AudioProcessor (BusesProperties()
                     #if ! JucePlugin_IsMidiEffect
                      #if ! JucePlugin_IsSynth
                       .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                      #endif
                       .withOutput ("Output", juce::AudioChannelSet::stereo(), true)
                     #endif
                       ),
       apvts (*this, nullptr, "Parameters", createParameterLayout())
{
    clipOnParam        = apvts.getRawParameterValue ("clipOn");
    clipThresholdParam = apvts.getRawParameterValue ("clipThreshold");
    limitOnParam       = apvts.getRawParameterValue ("limitOn");
    limitGainParam     = apvts.getRawParameterValue ("limitGain");
    limitCeilingParam  = apvts.getRawParameterValue ("limitCeiling");
    limitReleaseParam  = apvts.getRawParameterValue ("limitRelease");
    matchOnParam       = apvts.getRawParameterValue ("matchOn");
}

juce::AudioProcessorValueTreeState::ParameterLayout ParityAudioProcessor::createParameterLayout()
{
    using FloatParam = juce::AudioParameterFloat;
    using BoolParam = juce::AudioParameterBool;

    juce::AudioProcessorValueTreeState::ParameterLayout layout;

    layout.add (std::make_unique<BoolParam> ("clipOn", "Clip On", false));
    layout.add (std::make_unique<FloatParam> ("clipThreshold", "Clip Threshold",
                                              juce::NormalisableRange<float> (-20.0f, 0.0f, 0.1f), 0.0f));

    layout.add (std::make_unique<BoolParam> ("limitOn", "Limit On", false));
    layout.add (std::make_unique<FloatParam> ("limitGain", "Limit Gain",
                                              juce::NormalisableRange<float> (-12.0f, 12.0f, 0.1f), 0.0f));
    layout.add (std::make_unique<FloatParam> ("limitCeiling", "Limit Ceiling",
                                              juce::NormalisableRange<float> (-12.0f, 0.0f, 0.1f), -1.0f));
    layout.add (std::make_unique<FloatParam> ("limitRelease", "Limit Release",
                                              juce::NormalisableRange<float> (1.0f, 1000.0f, 1.0f, 0.3f), 100.0f));

    layout.add (std::make_unique<BoolParam> ("matchOn", "Loudness Match", false));

    return layout;
}

ParityAudioProcessor::~ParityAudioProcessor()
{
    loadPool.removeAllJobs (true, 5000);
}

//==============================================================================
const juce::String ParityAudioProcessor::getName() const
{
    return JucePlugin_Name;
}

bool ParityAudioProcessor::acceptsMidi() const
{
   #if JucePlugin_WantsMidiInput
    return true;
   #else
    return false;
   #endif
}

bool ParityAudioProcessor::producesMidi() const
{
   #if JucePlugin_ProducesMidiOutput
    return true;
   #else
    return false;
   #endif
}

bool ParityAudioProcessor::isMidiEffect() const
{
   #if JucePlugin_IsMidiEffect
    return true;
   #else
    return false;
   #endif
}

double ParityAudioProcessor::getTailLengthSeconds() const
{
    return 0.0;
}

int ParityAudioProcessor::getNumPrograms()
{
    return 1;   // NB: some hosts don't cope very well if you tell them there are 0 programs,
                // so this should be at least 1, even if you're not really implementing programs.
}

int ParityAudioProcessor::getCurrentProgram()
{
    return 0;
}

void ParityAudioProcessor::setCurrentProgram (int index)
{
    juce::ignoreUnused (index);
}

const juce::String ParityAudioProcessor::getProgramName (int index)
{
    juce::ignoreUnused (index);
    return {};
}

void ParityAudioProcessor::changeProgramName (int index, const juce::String& newName)
{
    juce::ignoreUnused (index, newName);
}

//==============================================================================
void ParityAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    hostSampleRate = sampleRate;
    referenceBuffer.setSize (2, samplesPerBlock);
    referenceGain.reset (sampleRate, 0.02); // 20 ms ramp to avoid clicks on toggle
    referenceGain.setCurrentAndTargetValue (referenceActive.load() ? 1.0f : 0.0f);
    matchGain.reset (sampleRate, 0.05);
    matchGain.setCurrentAndTargetValue (1.0f);

    mixLoudness.prepare (sampleRate, samplesPerBlock);
    referenceLoudness.prepare (sampleRate, samplesPerBlock);
    mixSpectrum.prepare (sampleRate);
    referenceSpectrum.prepare (sampleRate);
    mixStereo.prepare (sampleRate);
    referenceStereo.prepare (sampleRate);
    masterChain.prepare (sampleRate, samplesPerBlock, juce::jmax (1, getTotalNumOutputChannels()));
}

void ParityAudioProcessor::releaseResources()
{
    // When playback stops, you can use this as an opportunity to free up any
    // spare memory, etc.
}

bool ParityAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
  #if JucePlugin_IsMidiEffect
    juce::ignoreUnused (layouts);
    return true;
  #else
    // This is the place where you check if the layout is supported.
    // In this template code we only support mono or stereo.
    // Some plugin hosts, such as certain GarageBand versions, will only
    // load plugins that support stereo bus layouts.
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono()
     && layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;

    // This checks if the input layout matches the output layout
   #if ! JucePlugin_IsSynth
    if (layouts.getMainOutputChannelSet() != layouts.getMainInputChannelSet())
        return false;
   #endif

    return true;
  #endif
}

void ParityAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer,
                                              juce::MidiBuffer& midiMessages)
{
    juce::ignoreUnused (midiMessages);

    juce::ScopedNoDenormals noDenormals;
    auto totalNumInputChannels  = getTotalNumInputChannels();
    auto totalNumOutputChannels = getTotalNumOutputChannels();

    // In case we have more outputs than inputs, this code clears any output
    // channels that didn't contain input data, (because these aren't
    // guaranteed to be empty - they may contain garbage).
    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear (i, 0, buffer.getNumSamples());

    const auto numSamples = buffer.getNumSamples();

    // Query the host playhead for the current position.
    double playheadSeconds = -1.0;
    bool hostIsPlaying = false;

    if (auto* playHead = getPlayHead())
    {
        if (auto position = playHead->getPosition())
        {
            hostIsPlaying = position->getIsPlaying();

            if (auto seconds = position->getTimeInSeconds())
                playheadSeconds = *seconds;
        }
    }

    lastPlayheadSeconds.store (playheadSeconds);
    hostPlaying.store (hostIsPlaying);

    // Master processing applies to the mix only, ahead of the analyzer taps,
    // so the meters read what would actually be rendered.
    masterChain.setParameters (clipOnParam->load() > 0.5f, clipThresholdParam->load(),
                               limitOnParam->load() > 0.5f, limitGainParam->load(),
                               limitCeilingParam->load(), limitReleaseParam->load());
    masterChain.process (buffer);

    // Measure the mix input while the host is playing (pre-crossfade so the
    // reading always reflects the actual mix, not what's being monitored).
    if (hostIsPlaying)
    {
        mixLoudness.process (buffer);
        mixSpectrum.process (buffer);
        mixStereo.process (buffer);
    }

    // Render the reference whenever a file is loaded so its meters stay live
    // even while listening to the mix.
    const auto referenceRendered = referencePlayer.hasFileLoaded() && hostIsPlaying;

    if (referenceRendered)
    {
        referenceBuffer.setSize (buffer.getNumChannels(), numSamples, false, false, true);
        referencePlayer.process (referenceBuffer, playheadSeconds, hostSampleRate, true);
        referenceLoudness.process (referenceBuffer);
        referenceSpectrum.process (referenceBuffer);
        referenceStereo.process (referenceBuffer);
    }

    referenceGain.setTargetValue (referenceActive.load() && referencePlayer.hasFileLoaded() ? 1.0f : 0.0f);

    // Loudness match: play the reference at the mix's integrated loudness so
    // A/B comparisons aren't biased by level. Applied to monitoring only -
    // the reference meters still show the file's true levels.
    const auto mixLufs = mixLoudness.getIntegratedLufs();
    const auto refLufs = referenceFileLufs.load();
    const auto matchActive = matchOnParam->load() > 0.5f
                          && mixLufs > LoudnessAnalyzer::silenceLufs + 1.0f
                          && refLufs > LoudnessAnalyzer::silenceLufs + 1.0f;

    const auto matchDb = matchActive ? juce::jlimit (-24.0f, 24.0f, mixLufs - refLufs) : 0.0f;
    matchGainDb.store (matchDb);
    matchGain.setTargetValue (juce::Decibels::decibelsToGain (matchDb));

    // Skip the crossfade entirely while fully faded out.
    if (referenceGain.getCurrentValue() <= 0.0f && ! referenceGain.isSmoothing())
        return;

    if (! referenceRendered)
    {
        referenceBuffer.setSize (buffer.getNumChannels(), numSamples, false, false, true);
        referenceBuffer.clear();
    }

    // Crossfade: gain -> reference, (1 - gain) -> mix.
    for (int i = 0; i < numSamples; ++i)
    {
        const auto gain = referenceGain.getNextValue();
        const auto match = matchGain.getNextValue();

        for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
        {
            const auto mix = buffer.getSample (ch, i);
            const auto ref = referenceBuffer.getSample (ch, i) * match;
            buffer.setSample (ch, i, mix * (1.0f - gain) + ref * gain);
        }
    }
}

//==============================================================================
bool ParityAudioProcessor::hasEditor() const
{
    return true; // (change this to false if you choose to not supply an editor)
}

juce::AudioProcessorEditor* ParityAudioProcessor::createEditor()
{
    return new ParityAudioProcessorEditor (*this);
}

//==============================================================================
void ParityAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    juce::XmlElement state ("ParityState");
    state.setAttribute ("referenceFile", referencePlayer.getFile().getFullPathName());
    state.setAttribute ("referenceActive", referenceActive.load());

    if (auto paramsXml = apvts.copyState().createXml())
        state.addChildElement (paramsXml.release());

    copyXmlToBinary (state, destData);
}

void ParityAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    if (auto state = getXmlFromBinary (data, sizeInBytes))
    {
        if (state->hasTagName ("ParityState"))
        {
            const juce::File file (state->getStringAttribute ("referenceFile"));

            if (file.existsAsFile())
                loadReferenceFileAsync (file);

            referenceActive.store (state->getBoolAttribute ("referenceActive"));

            if (auto* paramsXml = state->getChildByName (apvts.state.getType()))
                apvts.replaceState (juce::ValueTree::fromXml (*paramsXml));
        }
    }
}

void ParityAudioProcessor::loadReferenceFileAsync (const juce::File& file)
{
    const auto generation = loadGeneration.fetch_add (1) + 1;
    loadStatus.store (ReferenceLoadStatus::loading);

    loadPool.addJob ([this, file, generation]
    {
        // Decode into job-local storage; only the final atomic swap is shared.
        const auto ok = referencePlayer.loadFile (file);

        if (generation != loadGeneration.load())
            return; // superseded by a newer load request

        if (! ok)
        {
            loadStatus.store (ReferenceLoadStatus::failed);
            return;
        }

        referencePlayer.withLoadedAudio ([this] (const juce::AudioBuffer<float>& audio, double sampleRate)
        {
            const auto stats = LoudnessAnalyzer::analyzeBuffer (audio, sampleRate);
            referenceFileLufs.store (stats.integratedLufs);
            referenceFilePeak.store (stats.truePeakDb);
        });

        loadStatus.store (ReferenceLoadStatus::loaded);
    });
}

//==============================================================================
// This creates new instances of the plugin..
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new ParityAudioProcessor();
}
