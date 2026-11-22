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

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (VoxOneAudioProcessorEditor)
};