#include "PluginProcessor.h"
#include "PluginEditor.h"

VoxOneAudioProcessorEditor::VoxOneAudioProcessorEditor (VoxOneAudioProcessor& p)
    : AudioProcessorEditor (&p), audioProcessor (p)
{
    setupRotarySlider (inputGainSlider, inputGainLabel, "INPUT", " dB");
    setupRotarySlider (hpfSlider, hpfLabel, "HPF", " Hz");
    setupRotarySlider (cleanSlider, cleanLabel, "CLEAN", " %");
    setupRotarySlider (compSlider, compLabel, "COMP", " %");
    setupRotarySlider (outputGainSlider, outputGainLabel, "OUTPUT", " dB");

    inputGainAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (
        audioProcessor.apvts, "inputGain", inputGainSlider);

    hpfAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (
        audioProcessor.apvts, "hpfFreq", hpfSlider);

    cleanAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (
        audioProcessor.apvts, "cleanAmount", cleanSlider);

    compAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (
        audioProcessor.apvts, "compAmount", compSlider);

    outputGainAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (
        audioProcessor.apvts, "outputGain", outputGainSlider);

    setSize (840, 360);
}

VoxOneAudioProcessorEditor::~VoxOneAudioProcessorEditor()
{
}

void VoxOneAudioProcessorEditor::setupRotarySlider (juce::Slider& slider, juce::Label& label, const juce::String& text, const juce::String& suffix)
{
    slider.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    slider.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 76, 22);
    slider.setTextValueSuffix (suffix);
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
    area.removeFromTop (70);
    area.reduce (24, 30);

    int columnWidth = area.getWidth() / 5;

    auto inputArea  = area.removeFromLeft (columnWidth);
    auto hpfArea    = area.removeFromLeft (columnWidth);
    auto cleanArea  = area.removeFromLeft (columnWidth);
    auto compArea   = area.removeFromLeft (columnWidth);
    auto outputArea = area;

    inputGainLabel.setBounds (inputArea.removeFromTop (24));
    inputGainSlider.setBounds (inputArea.reduced (8));

    hpfLabel.setBounds (hpfArea.removeFromTop (24));
    hpfSlider.setBounds (hpfArea.reduced (8));

    cleanLabel.setBounds (cleanArea.removeFromTop (24));
    cleanSlider.setBounds (cleanArea.reduced (8));

    compLabel.setBounds (compArea.removeFromTop (24));
    compSlider.setBounds (compArea.reduced (8));

    outputGainLabel.setBounds (outputArea.removeFromTop (24));
    outputGainSlider.setBounds (outputArea.reduced (8));
}