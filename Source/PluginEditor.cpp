//
// Created by lacikapeek on 2026. 11. 22..
//
#include "PluginProcessor.h"
#include "PluginEditor.h"

VoxOneAudioProcessorEditor::VoxOneAudioProcessorEditor (VoxOneAudioProcessor& p)
    : AudioProcessorEditor (&p), audioProcessor (p)
{
    setSize (600, 400);
}

VoxOneAudioProcessorEditor::~VoxOneAudioProcessorEditor()
{
}

void VoxOneAudioProcessorEditor::paint (juce::Graphics& g)
{
    g.fillAll (juce::Colour (0xff111216));

    g.setColour (juce::Colour (0xff00e5ff));
    g.setFont (juce::FontOptions (28.0f, juce::Font::bold));
    g.drawFittedText ("VoxOne v0.1", getLocalBounds(), juce::Justification::centred, 1);
}

void VoxOneAudioProcessorEditor::resized()
{
}
