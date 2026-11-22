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
    juce::Slider deEssSlider;
    juce::Slider airSlider;
    juce::Slider clipSlider;
    juce::Slider outputGainSlider;
    juce::Slider ceilingSlider;

    juce::Label inputGainLabel;
    juce::Label hpfLabel;
    juce::Label cleanLabel;
    juce::Label compLabel;
    juce::Label deEssLabel;
    juce::Label airLabel;
    juce::Label clipLabel;
    juce::Label outputGainLabel;
    juce::Label ceilingLabel;

    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> inputGainAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> hpfAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> cleanAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> compAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> deEssAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> airAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> clipAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> outputGainAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> ceilingAttachment;

    void setupRotarySlider (juce::Slider& slider, juce::Label& label, const juce::String& text, const juce::String& suffix);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (VoxOneAudioProcessorEditor)
};