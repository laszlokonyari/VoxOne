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
    juce::Slider outputGainSlider;

    juce::Label inputGainLabel;
    juce::Label outputGainLabel;

    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> inputGainAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> outputGainAttachment;

    void setupRotarySlider (juce::Slider& slider, juce::Label& label, const juce::String& text);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (VoxOneAudioProcessorEditor)
};