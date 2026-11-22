#include "PluginProcessor.h"
#include "PluginEditor.h"

VoxOneAudioProcessorEditor::VoxOneAudioProcessorEditor (VoxOneAudioProcessor& p)
    : AudioProcessorEditor (&p), audioProcessor (p)
{
    setupRotarySlider (inputGainSlider, inputGainLabel, "INPUT GAIN");
    setupRotarySlider (outputGainSlider, outputGainLabel, "OUTPUT GAIN");

    inputGainAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (
        audioProcessor.apvts, "inputGain", inputGainSlider);

    outputGainAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (
        audioProcessor.apvts, "outputGain", outputGainSlider);

    setSize (600, 400);
}

VoxOneAudioProcessorEditor::~VoxOneAudioProcessorEditor()
{
}

void VoxOneAudioProcessorEditor::setupRotarySlider (juce::Slider& slider, juce::Label& label, const juce::String& text)
{
    slider.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    slider.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 70, 22);
    slider.setTextValueSuffix (" dB");
    slider.setColour (juce::Slider::rotarySliderFillColourId, juce::Colour (0xff00e5ff));
    slider.setColour (juce::Slider::rotarySliderOutlineColourId, juce::Colour (0xff262936));
    slider.setColour (juce::Slider::thumbColourId, juce::Colour (0xffffffff));
    slider.setColour (juce::Slider::textBoxTextColourId, juce::Colour (0xffe0e4ec));
    slider.setColour (juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
    addAndMakeVisible (slider);

    label.setText (text, juce::dontSendNotification);
    label.setJustificationType (juce::Justification::centred);
    label.setFont (juce::FontOptions (14.0f, juce::Font::bold));
    label.setColour (juce::Label::textColourId, juce::Colour (0xff9aa0b4));
    addAndMakeVisible (label);
}

void VoxOneAudioProcessorEditor::paint (juce::Graphics& g)
{
    g.fillAll (juce::Colour (0xff111216));

    g.setColour (juce::Colour (0xff00e5ff));
    g.setFont (juce::FontOptions (26.0f, juce::Font::bold));
    g.drawFittedText ("VOXONE", getLocalBounds().removeFromTop (60), juce::Justification::centred, 1);
}

void VoxOneAudioProcessorEditor::resized()
{
    auto area = getLocalBounds();
    area.removeFromTop (80);
    area.reduce (60, 40);

    auto leftArea = area.removeFromLeft (area.getWidth() / 2);
    auto rightArea = area;

    inputGainLabel.setBounds (leftArea.removeFromTop (24));
    inputGainSlider.setBounds (leftArea.reduced (10));

    outputGainLabel.setBounds (rightArea.removeFromTop (24));
    outputGainSlider.setBounds (rightArea.reduced (10));
}