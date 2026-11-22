//
// Created by lacikapeek on 2026. 11. 22..
//
#include "PluginProcessor.h"
#include "PluginEditor.h"

VoxOneAudioProcessor::VoxOneAudioProcessor()
    : AudioProcessor (BusesProperties()
                          .withInput ("Input", juce::AudioChannelSet::stereo(), true)
                          .withOutput ("Output", juce::AudioChannelSet::stereo(), true))
{
}

VoxOneAudioProcessor::~VoxOneAudioProcessor()
{
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
    juce::ignoreUnused (sampleRate, samplesPerBlock);
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
    juce::ignoreUnused (destData);
}

void VoxOneAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    juce::ignoreUnused (data, sizeInBytes);
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new VoxOneAudioProcessor();
}