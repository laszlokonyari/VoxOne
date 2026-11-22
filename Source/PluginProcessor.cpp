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
        juce::ParameterID { "hpfFreq", 1 },
        "High-Pass",
        juce::NormalisableRange<float> (20.0f, 250.0f, 1.0f, 0.5f),
        80.0f,
        "Hz"));

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "cleanAmount", 1 },
        "Clean EQ",
        juce::NormalisableRange<float> (0.0f, 100.0f, 1.0f),
        50.0f,
        "%"));

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "outputGain", 1 },
        "Output Gain",
        juce::NormalisableRange<float> (-24.0f, 24.0f, 0.1f),
        0.0f,
        "dB"));

    return { params.begin(), params.end() };
}

void VoxOneAudioProcessor::updateFilters (float hpfFreq, float cleanAmount)
{
    if (currentSampleRate <= 0.0)
        return;

    if (std::abs (hpfFreq - lastHpfFreq) > 0.01f)
    {
        *hpfFilter.state = *juce::dsp::IIR::Coefficients<float>::makeHighPass (
            currentSampleRate, hpfFreq, 0.7071f);
        lastHpfFreq = hpfFreq;
    }

    if (std::abs (cleanAmount - lastCleanAmount) > 0.01f)
    {
        float norm = cleanAmount * 0.01f;
        float mudCutDb = -4.5f * norm;
        float presenceBoostDb = 3.0f * norm;

        *mudCutFilter.state = *juce::dsp::IIR::Coefficients<float>::makePeakFilter (
            currentSampleRate, 250.0f, 1.2f, juce::Decibels::decibelsToGain (mudCutDb));

        *presenceFilter.state = *juce::dsp::IIR::Coefficients<float>::makePeakFilter (
            currentSampleRate, 3500.0f, 1.0f, juce::Decibels::decibelsToGain (presenceBoostDb));

        lastCleanAmount = cleanAmount;
    }
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
    currentSampleRate = sampleRate;
    lastHpfFreq = -1.0f;
    lastCleanAmount = -1.0f;

    juce::dsp::ProcessSpec spec;
    spec.sampleRate = sampleRate;
    spec.maximumBlockSize = static_cast<juce::uint32> (samplesPerBlock);
    spec.numChannels = static_cast<juce::uint32> (getTotalNumOutputChannels());

    inputGainProcessor.prepare (spec);
    inputGainProcessor.setRampDurationSeconds (0.02);

    hpfFilter.prepare (spec);
    hpfFilter.reset();

    mudCutFilter.prepare (spec);
    mudCutFilter.reset();

    presenceFilter.prepare (spec);
    presenceFilter.reset();

    outputGainProcessor.prepare (spec);
    outputGainProcessor.setRampDurationSeconds (0.02);

    updateFilters (
        apvts.getRawParameterValue ("hpfFreq")->load(),
        apvts.getRawParameterValue ("cleanAmount")->load());
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
    auto hpfFreq      = apvts.getRawParameterValue ("hpfFreq")->load();
    auto cleanAmount  = apvts.getRawParameterValue ("cleanAmount")->load();
    auto outputGainDb = apvts.getRawParameterValue ("outputGain")->load();

    inputGainProcessor.setGainDecibels (inputGainDb);
    outputGainProcessor.setGainDecibels (outputGainDb);
    updateFilters (hpfFreq, cleanAmount);

    juce::dsp::AudioBlock<float> block (buffer);
    juce::dsp::ProcessContextReplacing<float> context (block);

    inputGainProcessor.process (context);
    hpfFilter.process (context);
    mudCutFilter.process (context);
    presenceFilter.process (context);
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