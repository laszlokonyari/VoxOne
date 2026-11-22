#pragma once

#include "PluginProcessor.h"
#include "VoxOneLookAndFeel.h"
#include "WaveformVisualizer.h"
#include <array>

class VoxOneAudioProcessorEditor : public juce::AudioProcessorEditor,
                                   private juce::Timer
{
public:
    explicit VoxOneAudioProcessorEditor (VoxOneAudioProcessor&);
    ~VoxOneAudioProcessorEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    void timerCallback() override;
    void setupRotarySlider (juce::Slider& slider, juce::Label& label,
                            const juce::String& text, const juce::String& suffix,
                            juce::Colour accentColour);
    void setupHeaderControls();
    void drawModuleCard (juce::Graphics& g, juce::Rectangle<int> bounds,
                         const juce::String& title, juce::Colour accent);

    VoxOneAudioProcessor& audioProcessor;
    VoxOneLookAndFeel customLookAndFeel;
    WaveformVisualizer waveformVisualizer;

    juce::ComboBox presetSelector;
    juce::TextButton prevPresetButton { "<" };
    juce::TextButton nextPresetButton { ">" };
    juce::TextButton bypassButton { "ACTIVE" };

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
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> bypassAttachment;

    juce::Rectangle<int> toneCardBounds;
    juce::Rectangle<int> dynamicsCardBounds;
    juce::Rectangle<int> characterCardBounds;
    juce::Rectangle<int> masterCardBounds;

    std::array<float, VoxOneAudioProcessor::waveformSize> inWaveCache {};
    std::array<float, VoxOneAudioProcessor::waveformSize> outWaveCache {};
    int lastDisplayedPreset { -1 };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (VoxOneAudioProcessorEditor)
};