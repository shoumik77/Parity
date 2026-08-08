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
                       )
{
}

ParityAudioProcessor::~ParityAudioProcessor()
{
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

    referenceGain.setTargetValue (referenceActive.load() && referencePlayer.hasFileLoaded() ? 1.0f : 0.0f);

    // Skip reference rendering entirely while fully faded out.
    if (referenceGain.getCurrentValue() <= 0.0f && ! referenceGain.isSmoothing())
        return;

    referenceBuffer.setSize (buffer.getNumChannels(), numSamples, false, false, true);
    referencePlayer.process (referenceBuffer, playheadSeconds, hostSampleRate, hostIsPlaying);

    // Crossfade: gain -> reference, (1 - gain) -> mix.
    for (int i = 0; i < numSamples; ++i)
    {
        const auto gain = referenceGain.getNextValue();

        for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
        {
            const auto mix = buffer.getSample (ch, i);
            const auto ref = referenceBuffer.getSample (ch, i);
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
                loadReferenceFile (file);

            referenceActive.store (state->getBoolAttribute ("referenceActive"));
        }
    }
}

bool ParityAudioProcessor::loadReferenceFile (const juce::File& file)
{
    return referencePlayer.loadFile (file);
}

//==============================================================================
// This creates new instances of the plugin..
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new ParityAudioProcessor();
}
