#pragma once

#include "PluginProcessor.h"
#include "VoxOneLookAndFeel.h"
#include "WaveformVisualizer.h"
#include "SpaceVisualizer.h"
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
    using SliderAttach = std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>;
    using ButtonAttach = std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment>;

    void timerCallback() override;
    void setupRotarySlider (AnimatedVoxKnob& slider, juce::Label& label,
                            const juce::String& text, const juce::String& suffix,
                            juce::Colour accentColour, double defaultVal,
                            bool bipolar = false, int textBoxWidth = 68);
    void setupHeaderControls();
    void setupSpaceControls();
    void drawModuleCard (juce::Graphics& g, juce::Rectangle<int> bounds,
                         const juce::String& title, juce::Colour accent, float activityGlow);
    void drawSignalChain (juce::Graphics& g, juce::Rectangle<int> area);

    VoxOneAudioProcessor& audioProcessor;
    VoxOneLookAndFeel customLookAndFeel;
    WaveformVisualizer waveformVisualizer;
    SpaceVisualizer spaceVisualizer;

    juce::ComboBox presetSelector;
    juce::TextButton prevPresetButton { "<" };
    juce::TextButton nextPresetButton { ">" };
    juce::TextButton bypassButton { "ACTIVE" };

    juce::TextButton spaceBypassButton { "SPACE ON" };
    std::array<juce::TextButton, 4> reverbModeButtons;
    juce::TextButton pingPongButton { "STEREO" };

    AnimatedVoxKnob inputGainSlider, hpfSlider, cleanSlider, compSlider, deEssSlider;
    AnimatedVoxKnob airSlider, clipSlider, outputGainSlider, ceilingSlider;

    AnimatedVoxKnob revMixSlider, revSizeSlider, revPreDelaySlider, revToneSlider, revWidthSlider, revDuckSlider;
    AnimatedVoxKnob delMixSlider, delTimeSlider, delFeedbackSlider, delToneSlider, delDuckSlider;

    juce::Label inputGainLabel, hpfLabel, cleanLabel, compLabel, deEssLabel;
    juce::Label airLabel, clipLabel, outputGainLabel, ceilingLabel;

    juce::Label revMixLabel, revSizeLabel, revPreDelayLabel, revToneLabel, revWidthLabel, revDuckLabel;
    juce::Label delMixLabel, delTimeLabel, delFeedbackLabel, delToneLabel, delDuckLabel, pingPongLabel;

    SliderAttach inputGainAttachment, hpfAttachment, cleanAttachment, compAttachment, deEssAttachment;
    SliderAttach airAttachment, clipAttachment, outputGainAttachment, ceilingAttachment;

    SliderAttach revMixAttachment, revSizeAttachment, revPreDelayAttachment, revToneAttachment, revWidthAttachment, revDuckAttachment;
    SliderAttach delMixAttachment, delTimeAttachment, delFeedbackAttachment, delToneAttachment, delDuckAttachment;

    ButtonAttach bypassAttachment, spaceBypassAttachment, pingPongAttachment;

    juce::Rectangle<int> toneCardBounds;
    juce::Rectangle<int> dynamicsCardBounds;
    juce::Rectangle<int> characterCardBounds;
    juce::Rectangle<int> spaceCardBounds;
    juce::Rectangle<int> reverbSubBounds;
    juce::Rectangle<int> delaySubBounds;
    juce::Rectangle<int> masterCardBounds;

    std::array<float, VoxOneAudioProcessor::waveformSize> inWaveCache {};
    std::array<float, VoxOneAudioProcessor::waveformSize> outWaveCache {};

    float toneCardGlow { 0.0f };
    float dynamicsCardGlow { 0.0f };
    float characterCardGlow { 0.0f };
    float spaceCardGlow { 0.0f };
    float masterCardGlow { 0.0f };
    int lastDisplayedPreset { -1 };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (VoxOneAudioProcessorEditor)
};