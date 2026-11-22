#include "PluginProcessor.h"
#include "PluginEditor.h"

VoxOneAudioProcessorEditor::VoxOneAudioProcessorEditor (VoxOneAudioProcessor& p)
    : AudioProcessorEditor (&p), audioProcessor (p)
{
    setupRotarySlider (inputGainSlider, inputGainLabel, "INPUT", " dB");
    setupRotarySlider (hpfSlider, hpfLabel, "HPF", " Hz");
    setupRotarySlider (cleanSlider, cleanLabel, "CLEAN", " %");
    setupRotarySlider (compSlider, compLabel, "COMP", " %");
    setupRotarySlider (deEssSlider, deEssLabel, "DE-ESS", " %");
    setupRotarySlider (airSlider, airLabel, "AIR", " %");
    setupRotarySlider (clipSlider, clipLabel, "SOFT CLIP", " %");
    setupRotarySlider (outputGainSlider, outputGainLabel, "OUTPUT", " dB");
    setupRotarySlider (ceilingSlider, ceilingLabel, "CEILING", " dB");

    inputGainAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (
        audioProcessor.apvts, "inputGain", inputGainSlider);

    hpfAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (
        audioProcessor.apvts, "hpfFreq", hpfSlider);

    cleanAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (
        audioProcessor.apvts, "cleanAmount", cleanSlider);

    compAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (
        audioProcessor.apvts, "compAmount", compSlider);

    deEssAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (
        audioProcessor.apvts, "deEssAmount", deEssSlider);

    airAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (
        audioProcessor.apvts, "airAmount", airSlider);

    clipAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (
        audioProcessor.apvts, "clipAmount", clipSlider);

    outputGainAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (
        audioProcessor.apvts, "outputGain", outputGainSlider);

    ceilingAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (
        audioProcessor.apvts, "limiterCeiling", ceilingSlider);

    setSize (860, 480);
}

VoxOneAudioProcessorEditor::~VoxOneAudioProcessorEditor()
{
}

void VoxOneAudioProcessorEditor::setupRotarySlider (juce::Slider& slider, juce::Label& label, const juce::String& text, const juce::String& suffix)
{
    slider.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    slider.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 72, 22);
    slider.setTextValueSuffix (suffix);
    slider.setColour (juce::Slider::rotarySliderFillColourId, juce::Colour (0xff00e5ff));
    slider.setColour (juce::Slider::rotarySliderOutlineColourId, juce::Colour (0xff262936));
    slider.setColour (juce::Slider::thumbColourId, juce::Colour (0xffffffff));
    slider.setColour (juce::Slider::textBoxTextColourId, juce::Colour (0xffe0e4ec));
    slider.setColour (juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
    addAndMakeVisible (slider);

    label.setText (text, juce::dontSendNotification);
    label.setJustificationType (juce::Justification::centred);
    label.setFont (juce::FontOptions (13.0f, juce::Font::bold));
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
    area.removeFromTop (64);
    area.reduce (24, 16);

    auto topRow = area.removeFromTop (area.getHeight() / 2);
    topRow.removeFromBottom (10);
    auto bottomRow = area;
    bottomRow.removeFromTop (10);

    int topColWidth = topRow.getWidth() / 5;
    auto hpfArea   = topRow.removeFromLeft (topColWidth);
    auto cleanArea = topRow.removeFromLeft (topColWidth);
    auto compArea  = topRow.removeFromLeft (topColWidth);
    auto deEssArea = topRow.removeFromLeft (topColWidth);
    auto airArea   = topRow;

    hpfLabel.setBounds (hpfArea.removeFromTop (22));
    hpfSlider.setBounds (hpfArea.reduced (6));

    cleanLabel.setBounds (cleanArea.removeFromTop (22));
    cleanSlider.setBounds (cleanArea.reduced (6));

    compLabel.setBounds (compArea.removeFromTop (22));
    compSlider.setBounds (compArea.reduced (6));

    deEssLabel.setBounds (deEssArea.removeFromTop (22));
    deEssSlider.setBounds (deEssArea.reduced (6));

    airLabel.setBounds (airArea.removeFromTop (22));
    airSlider.setBounds (airArea.reduced (6));

    int bottomColWidth = bottomRow.getWidth() / 4;
    auto inputArea   = bottomRow.removeFromLeft (bottomColWidth);
    auto clipArea    = bottomRow.removeFromLeft (bottomColWidth);
    auto outputArea  = bottomRow.removeFromLeft (bottomColWidth);
    auto ceilingArea = bottomRow;

    inputGainLabel.setBounds (inputArea.removeFromTop (22));
    inputGainSlider.setBounds (inputArea.reduced (6));

    clipLabel.setBounds (clipArea.removeFromTop (22));
    clipSlider.setBounds (clipArea.reduced (6));

    outputGainLabel.setBounds (outputArea.removeFromTop (22));
    outputGainSlider.setBounds (outputArea.reduced (6));

    ceilingLabel.setBounds (ceilingArea.removeFromTop (22));
    ceilingSlider.setBounds (ceilingArea.reduced (6));
}