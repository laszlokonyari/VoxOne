#include "PluginProcessor.h"
#include "PluginEditor.h"

VoxOneAudioProcessor::VoxOneAudioProcessor()
    : AudioProcessor (BusesProperties()
                          .withInput ("Input", juce::AudioChannelSet::stereo(), true)
                          .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "Parameters", createParameterLayout())
{
}

VoxOneAudioProcessor::~VoxOneAudioProcessor()
{
}

juce::AudioProcessorValueTreeState::ParameterLayout VoxOneAudioProcessor::createParameterLayout()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "inputGain", 1 },
        "Input Gain",
        juce::NormalisableRange<float> (-24.0f, 24.0f, 0.1f),
        0.0f,
        "dB"));

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "outputGain", 1 },
        "Output Gain",
        juce::NormalisableRange<float> (-24.0f, 24.0f, 0.1f),
        0.0f,
        "dB"));

    return { params.begin(), params.end() };
}

const juce::String VoxOneAudioProcessor::getName() const
{
    return JucePlugin_Name;
}

bool VoxOneAudioProcessor::acceptsMidi() const
{
    return false;
}

bool VoxOneAudioProcessor::producesMidi() const
{
    return false;
}

bool VoxOneAudioProcessor::isMidiEffect() const
{
    return false;
}

double VoxOneAudioProcessor::getTailLengthSeconds() const
{
    return 0.0;
}

int VoxOneAudioProcessor::getNumPrograms()
{
    return 1;
}

int VoxOneAudioProcessor::getCurrentProgram()
{
    return 0;
}

void VoxOneAudioProcessor::setCurrentProgram (int index)
{
    juce::ignoreUnused (index);
}

const juce::String VoxOneAudioProcessor::getProgramName (int index)
{
    juce::ignoreUnused (index);
    return {};
}

void VoxOneAudioProcessor::changeProgramName (int index, const juce::String& newName)
{
    juce::ignoreUnused (index, newName);
}

void VoxOneAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    juce::dsp::ProcessSpec spec;
    spec.sampleRate = sampleRate;
    spec.maximumBlockSize = static_cast<juce::uint32> (samplesPerBlock);
    spec.numChannels = static_cast<juce::uint32> (getTotalNumOutputChannels());

    inputGainProcessor.prepare (spec);
    inputGainProcessor.setRampDurationSeconds (0.02);

    outputGainProcessor.prepare (spec);
    outputGainProcessor.setRampDurationSeconds (0.02);
}

void VoxOneAudioProcessor::releaseResources()
{
}

bool VoxOneAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono()
        && layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;

    if (layouts.getMainOutputChannelSet() != layouts.getMainInputChannelSet())
        return false;

    return true;
}

void VoxOneAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ignoreUnused (midiMessages);
    juce::ScopedNoDenormals noDenormals;

    auto totalNumInputChannels  = getTotalNumInputChannels();
    auto totalNumOutputChannels = getTotalNumOutputChannels();

    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear (i, 0, buffer.getNumSamples());

    auto inputGainDb  = apvts.getRawParameterValue ("inputGain")->load();
    auto outputGainDb = apvts.getRawParameterValue ("outputGain")->load();

    inputGainProcessor.setGainDecibels (inputGainDb);
    outputGainProcessor.setGainDecibels (outputGainDb);

    juce::dsp::AudioBlock<float> block (buffer);
    juce::dsp::ProcessContextReplacing<float> context (block);

    inputGainProcessor.process (context);
    outputGainProcessor.process (context);
}

bool VoxOneAudioProcessor::hasEditor() const
{
    return true;
}

juce::AudioProcessorEditor* VoxOneAudioProcessor::createEditor()
{
    return new VoxOneAudioProcessorEditor (*this);
}

void VoxOneAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    auto state = apvts.copyState();
    std::unique_ptr<juce::XmlElement> xml (state.createXml());
    copyXmlToBinary (*xml, destData);
}

void VoxOneAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xmlState (getXmlFromBinary (data, sizeInBytes));

    if (xmlState != nullptr && xmlState->hasTagName (apvts.state.getType()))
        apvts.replaceState (juce::ValueTree::fromXml (*xmlState));
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new VoxOneAudioProcessor();
}