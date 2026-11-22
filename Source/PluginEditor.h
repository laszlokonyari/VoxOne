#pragma once

#include "PluginProcessor.h"

class VoxOneAudioProcessorEditor : public juce::AudioProcessorEditor
{
public:
    explicit VoxOneAudioProcessorEditor (VoxOneAudioProcessor&);
    ~VoxOneAudioProcessorEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    VoxOneAudioProcessor& audioProcessor;

    juce::Slider inputGainSlider;
    juce::Slider hpfSlider;
    juce::Slider cleanSlider;
    juce::Slider compSlider;
    juce::Slider outputGainSlider;

    juce::Label inputGainLabel;
    juce::Label hpfLabel;
    juce::Label cleanLabel;
    juce::Label compLabel;
    juce::Label outputGainLabel;

    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> inputGainAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> hpfAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> cleanAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> compAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> outputGainAttachment;

    void setupRotarySlider (juce::Slider& slider, juce::Label& label, const juce::String& text, const juce::String& suffix);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (VoxOneAudioProcessorEditor)
};