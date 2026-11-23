#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_dsp/juce_dsp.h>
#include "VocalDSP.h"
#include "SpaceDSP.h"
#include <array>
#include <atomic>
#include <vector>

struct VoxOnePreset
{
    juce::String name;
    float inputGain;
    float hpfFreq;
    float cleanAmount;
    float compAmount;
    float deEssAmount;
    float airAmount;
    float clipAmount;
    float outputGain;
    float limiterCeiling;
    int reverbMode;
    float reverbMix;
    float reverbSize;
    float reverbPreDelay;
    float reverbTone;
    float reverbWidth;
    float reverbDuck;
    float delayMix;
    float delayTime;
    float delayFeedback;
    float delayTone;
    float delayDuck;
    bool delayPingPong;
};

class VoxOneAudioProcessor : public juce::AudioProcessor
{
public:
    static constexpr int waveformSize = 256;

    VoxOneAudioProcessor();
    ~VoxOneAudioProcessor() override;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;

    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override;

    const juce::String getName() const override;

    bool acceptsMidi() const override;
    bool producesMidi() const override;
    bool isMidiEffect() const override;
    double getTailLengthSeconds() const override;

    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram (int index) override;
    const juce::String getProgramName (int index) override;
    void changeProgramName (int index, const juce::String& newName) override;

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    const std::vector<VoxOnePreset>& getPresets() const;
    void loadPreset (int index);
    int getCurrentPresetIndex() const;

    void copyWaveformData (std::array<float, waveformSize>& destIn,
                           std::array<float, waveformSize>& destOut) const;
    float getInputLevel() const;
    float getOutputLevel() const;
    float getGainReductionDb() const;
    float getReverbEnergy() const;
    float getDelayEnergy() const;
    float getSpaceDuckActivity() const;

    juce::AudioProcessorValueTreeState apvts;

private:
    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();
    void initPresets();
    void setParameterValue (const juce::String& paramID, float plainValue);
    void updateCleanEQ (float cleanAmount);
    void pushWaveformSamples (const juce::AudioBuffer<float>& inBuf, const juce::AudioBuffer<float>& outBuf);

    std::vector<VoxOnePreset> presets;
    std::atomic<int> currentPresetIndex { 0 };

    juce::dsp::Gain<float> inputGainProcessor;
    juce::dsp::StateVariableTPTFilter<float> hpfStage1;
    juce::dsp::StateVariableTPTFilter<float> hpfStage2;

    StereoBiquad mudCutFilter;
    StereoBiquad boxCutFilter;
    StereoBiquad presenceFilter;

    VocalCompressor vocalCompressor;
    DynamicDeEsser dynamicDeEsser;
    VocalExciter vocalExciter;

    VocalSlapDelay vocalSlapDelay;
    VocalReverb vocalReverb;

    juce::dsp::Gain<float> outputGainProcessor;
    juce::dsp::Limiter<float> outputLimiter;

    juce::SmoothedValue<float> smoothHpfFreq;
    juce::SmoothedValue<float> smoothCleanAmount;
    juce::SmoothedValue<float> smoothCompAmount;
    juce::SmoothedValue<float> smoothDeEssAmount;
    juce::SmoothedValue<float> smoothAirAmount;
    juce::SmoothedValue<float> smoothClipAmount;

    juce::AudioBuffer<float> inputMonitorBuffer;
    juce::AudioBuffer<float> delayWetBuffer;
    juce::AudioBuffer<float> reverbWetBuffer;

    double currentSampleRate { 44100.0 };
    float lastCleanAmount { -1.0f };

    std::array<std::atomic<float>, waveformSize> inputWaveform {};
    std::array<std::atomic<float>, waveformSize> outputWaveform {};
    std::atomic<int> waveformWriteIndex { 0 };
    int samplesPerVisualPoint { 256 };
    int visualSampleCounter { 0 };
    float pendingInPeak { 0.0f };
    float pendingOutPeak { 0.0f };

    std::atomic<float> inputMeterLevel { 0.0f };
    std::atomic<float> outputMeterLevel { 0.0f };
    std::atomic<float> compGainReductionDb { 0.0f };
    std::atomic<float> reverbEnergyLevel { 0.0f };
    std::atomic<float> delayEnergyLevel { 0.0f };
    std::atomic<float> spaceDuckLevel { 0.0f };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (VoxOneAudioProcessor)
};