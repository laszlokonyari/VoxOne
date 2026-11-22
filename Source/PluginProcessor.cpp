#include "PluginProcessor.h"
#include "PluginEditor.h"

VoxOneAudioProcessor::VoxOneAudioProcessor()
    : AudioProcessor (BusesProperties()
                          .withInput ("Input", juce::AudioChannelSet::stereo(), true)
                          .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "Parameters", createParameterLayout())
{
    initPresets();

    for (auto& v : inputWaveform)
        v.store (0.0f, std::memory_order_relaxed);

    for (auto& v : outputWaveform)
        v.store (0.0f, std::memory_order_relaxed);
}

VoxOneAudioProcessor::~VoxOneAudioProcessor()
{
}

void VoxOneAudioProcessor::initPresets()
{
    presets = {
        { "Default",         0.0f,  80.0f, 50.0f, 40.0f, 35.0f, 30.0f, 20.0f,  0.0f, -0.3f },
        { "Modern Rap",      1.5f,  95.0f, 65.0f, 68.0f, 45.0f, 45.0f, 40.0f,  0.0f, -0.2f },
        { "Trap / Drill",    2.0f, 110.0f, 75.0f, 80.0f, 50.0f, 55.0f, 55.0f,  0.5f, -0.1f },
        { "Melodic Rap",     0.5f,  85.0f, 60.0f, 55.0f, 45.0f, 65.0f, 25.0f,  0.0f, -0.3f },
        { "Pop Lead",        0.0f,  90.0f, 70.0f, 60.0f, 55.0f, 60.0f, 20.0f,  0.0f, -0.3f },
        { "R&B Smooth",      0.0f,  70.0f, 40.0f, 45.0f, 40.0f, 35.0f, 15.0f,  0.0f, -0.5f },
        { "Hyperpop",        2.5f, 125.0f, 85.0f, 90.0f, 60.0f, 85.0f, 65.0f,  0.0f, -0.1f },
        { "Rock / Punk",     2.0f, 100.0f, 70.0f, 75.0f, 35.0f, 30.0f, 75.0f,  0.0f, -0.2f },
        { "Ad-Libs",         0.0f, 160.0f, 80.0f, 75.0f, 55.0f, 50.0f, 35.0f, -2.5f, -0.5f },
        { "Warm Ballad",     0.0f,  65.0f, 35.0f, 30.0f, 30.0f, 20.0f, 10.0f,  0.0f, -0.5f },
        { "Podcast / Voice", 1.0f,  85.0f, 55.0f, 55.0f, 60.0f, 15.0f, 10.0f,  0.5f, -1.0f },
        { "Lo-Fi Radio",     3.0f, 240.0f, 90.0f, 85.0f, 20.0f,  0.0f, 90.0f, -1.5f, -0.5f }
    };
}

const std::vector<VoxOnePreset>& VoxOneAudioProcessor::getPresets() const
{
    return presets;
}

int VoxOneAudioProcessor::getCurrentPresetIndex() const
{
    return currentPresetIndex.load (std::memory_order_relaxed);
}

void VoxOneAudioProcessor::setParameterValue (const juce::String& paramID, float plainValue)
{
    if (auto* param = apvts.getParameter (paramID))
    {
        auto normalized = apvts.getParameterRange (paramID).convertTo0to1 (plainValue);
        param->setValueNotifyingHost (normalized);
    }
}

void VoxOneAudioProcessor::loadPreset (int index)
{
    if (index < 0 || index >= static_cast<int> (presets.size()))
        return;

    currentPresetIndex.store (index, std::memory_order_relaxed);
    const auto& preset = presets[static_cast<size_t> (index)];

    setParameterValue ("inputGain", preset.inputGain);
    setParameterValue ("hpfFreq", preset.hpfFreq);
    setParameterValue ("cleanAmount", preset.cleanAmount);
    setParameterValue ("compAmount", preset.compAmount);
    setParameterValue ("deEssAmount", preset.deEssAmount);
    setParameterValue ("airAmount", preset.airAmount);
    setParameterValue ("clipAmount", preset.clipAmount);
    setParameterValue ("outputGain", preset.outputGain);
    setParameterValue ("limiterCeiling", preset.limiterCeiling);
}

void VoxOneAudioProcessor::copyWaveformData (std::array<float, waveformSize>& destIn,
                                             std::array<float, waveformSize>& destOut) const
{
    int head = waveformWriteIndex.load (std::memory_order_acquire);

    for (int i = 0; i < waveformSize; ++i)
    {
        int idx = (head + i) & (waveformSize - 1);
        destIn[static_cast<size_t> (i)] = inputWaveform[static_cast<size_t> (idx)].load (std::memory_order_relaxed);
        destOut[static_cast<size_t> (i)] = outputWaveform[static_cast<size_t> (idx)].load (std::memory_order_relaxed);
    }
}

float VoxOneAudioProcessor::getInputLevel() const
{
    return inputMeterLevel.load (std::memory_order_relaxed);
}

float VoxOneAudioProcessor::getOutputLevel() const
{
    return outputMeterLevel.load (std::memory_order_relaxed);
}

float VoxOneAudioProcessor::getGainReductionDb() const
{
    return compGainReductionDb.load (std::memory_order_relaxed);
}

juce::AudioProcessorValueTreeState::ParameterLayout VoxOneAudioProcessor::createParameterLayout()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "inputGain", 1 },
        "Input Gain",
        juce::NormalisableRange<float> (-24.0f, 24.0f, 0.1f),
        0.0f,
        "dB"));

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "hpfFreq", 1 },
        "High-Pass",
        juce::NormalisableRange<float> (20.0f, 250.0f, 1.0f, 0.5f),
        80.0f,
        "Hz"));

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "cleanAmount", 1 },
        "Clean EQ",
        juce::NormalisableRange<float> (0.0f, 100.0f, 1.0f),
        50.0f,
        "%"));

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "compAmount", 1 },
        "Compressor",
        juce::NormalisableRange<float> (0.0f, 100.0f, 1.0f),
        40.0f,
        "%"));

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "deEssAmount", 1 },
        "De-Esser",
        juce::NormalisableRange<float> (0.0f, 100.0f, 1.0f),
        35.0f,
        "%"));

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "airAmount", 1 },
        "Fresh Air",
        juce::NormalisableRange<float> (0.0f, 100.0f, 1.0f),
        30.0f,
        "%"));

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "clipAmount", 1 },
        "Soft Clip",
        juce::NormalisableRange<float> (0.0f, 100.0f, 1.0f),
        20.0f,
        "%"));

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "outputGain", 1 },
        "Output Gain",
        juce::NormalisableRange<float> (-24.0f, 24.0f, 0.1f),
        0.0f,
        "dB"));

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "limiterCeiling", 1 },
        "Ceiling",
        juce::NormalisableRange<float> (-6.0f, -0.1f, 0.1f),
        -0.3f,
        "dB"));

    params.push_back (std::make_unique<juce::AudioParameterBool> (
        juce::ParameterID { "bypass", 1 },
        "Bypass",
        false));

    return { params.begin(), params.end() };
}

void VoxOneAudioProcessor::updateCleanEQ (float cleanAmount)
{
    if (std::abs (cleanAmount - lastCleanAmount) <= 0.05f)
        return;

    float norm = cleanAmount * 0.01f;
    mudCutFilter.setPeak (currentSampleRate, 260.0f, 1.15f, -4.2f * norm);
    boxCutFilter.setPeak (currentSampleRate, 560.0f, 1.8f, -2.0f * norm);
    presenceFilter.setPeak (currentSampleRate, 3200.0f, 0.95f, 3.2f * norm);
    lastCleanAmount = cleanAmount;
}

void VoxOneAudioProcessor::pushWaveformSamples (const juce::AudioBuffer<float>& inBuf,
                                                const juce::AudioBuffer<float>& outBuf)
{
    int numSamples = outBuf.getNumSamples();
    int numChannels = std::min (inBuf.getNumChannels(), outBuf.getNumChannels());

    if (numChannels <= 0 || numSamples <= 0)
        return;

    const auto* inL = inBuf.getReadPointer (0);
    const auto* inR = (numChannels > 1) ? inBuf.getReadPointer (1) : nullptr;
    const auto* outL = outBuf.getReadPointer (0);
    const auto* outR = (numChannels > 1) ? outBuf.getReadPointer (1) : nullptr;

    float blockInPeak = 0.0f;
    float blockOutPeak = 0.0f;

    for (int i = 0; i < numSamples; ++i)
    {
        float inSample = std::abs (inL[i]);
        if (inR != nullptr)
            inSample = std::max (inSample, std::abs (inR[i]));

        float outSample = std::abs (outL[i]);
        if (outR != nullptr)
            outSample = std::max (outSample, std::abs (outR[i]));

        blockInPeak = std::max (blockInPeak, inSample);
        blockOutPeak = std::max (blockOutPeak, outSample);

        pendingInPeak = std::max (pendingInPeak, inSample);
        pendingOutPeak = std::max (pendingOutPeak, outSample);

        if (++visualSampleCounter >= samplesPerVisualPoint)
        {
            int idx = waveformWriteIndex.load (std::memory_order_relaxed);
            inputWaveform[static_cast<size_t> (idx)].store (pendingInPeak, std::memory_order_relaxed);
            outputWaveform[static_cast<size_t> (idx)].store (pendingOutPeak, std::memory_order_relaxed);
            waveformWriteIndex.store ((idx + 1) & (waveformSize - 1), std::memory_order_release);

            pendingInPeak = 0.0f;
            pendingOutPeak = 0.0f;
            visualSampleCounter = 0;
        }
    }

    inputMeterLevel.store (blockInPeak, std::memory_order_relaxed);
    outputMeterLevel.store (blockOutPeak, std::memory_order_relaxed);
}

const juce::String VoxOneAudioProcessor::getName() const
{
    return JucePlugin_Name;
}

bool VoxOneAudioProcessor::acceptsMidi() const
{
    return false;
}

bool VoxOneAudioProcessor::producesMidi() const
{
    return false;
}

bool VoxOneAudioProcessor::isMidiEffect() const
{
    return false;
}

double VoxOneAudioProcessor::getTailLengthSeconds() const
{
    return 0.0;
}

int VoxOneAudioProcessor::getNumPrograms()
{
    return static_cast<int> (presets.size());
}

int VoxOneAudioProcessor::getCurrentProgram()
{
    return getCurrentPresetIndex();
}

void VoxOneAudioProcessor::setCurrentProgram (int index)
{
    loadPreset (index);
}

const juce::String VoxOneAudioProcessor::getProgramName (int index)
{
    if (index >= 0 && index < static_cast<int> (presets.size()))
        return presets[static_cast<size_t> (index)].name;

    return {};
}

void VoxOneAudioProcessor::changeProgramName (int index, const juce::String& newName)
{
    juce::ignoreUnused (index, newName);
}

void VoxOneAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    currentSampleRate = std::max (1.0, sampleRate);
    lastCleanAmount = -1.0f;
    samplesPerVisualPoint = std::max (64, static_cast<int> (currentSampleRate * 0.006));
    visualSampleCounter = 0;
    pendingInPeak = 0.0f;
    pendingOutPeak = 0.0f;

    juce::dsp::ProcessSpec spec;
    spec.sampleRate = currentSampleRate;
    spec.maximumBlockSize = static_cast<juce::uint32> (std::max (samplesPerBlock, 4096));
    spec.numChannels = static_cast<juce::uint32> (std::max (1, getTotalNumOutputChannels()));

    inputMonitorBuffer.setSize (static_cast<int> (spec.numChannels), static_cast<int> (spec.maximumBlockSize));

    inputGainProcessor.prepare (spec);
    inputGainProcessor.setRampDurationSeconds (0.02);

    hpfStage1.prepare (spec);
    hpfStage1.reset();
    hpfStage1.setType (juce::dsp::StateVariableTPTFilterType::highpass);
    hpfStage1.setResonance (0.5412f);

    hpfStage2.prepare (spec);
    hpfStage2.reset();
    hpfStage2.setType (juce::dsp::StateVariableTPTFilterType::highpass);
    hpfStage2.setResonance (1.3066f);

    mudCutFilter.reset();
    boxCutFilter.reset();
    presenceFilter.reset();

    vocalCompressor.prepare (currentSampleRate);
    dynamicDeEsser.prepare (currentSampleRate);
    vocalExciter.prepare (currentSampleRate);

    outputGainProcessor.prepare (spec);
    outputGainProcessor.setRampDurationSeconds (0.02);

    outputLimiter.prepare (spec);
    outputLimiter.reset();
    outputLimiter.setRelease (45.0f);

    double smoothTime = 0.025;
    smoothHpfFreq.reset (currentSampleRate, smoothTime);
    smoothCleanAmount.reset (currentSampleRate, smoothTime);
    smoothCompAmount.reset (currentSampleRate, smoothTime);
    smoothDeEssAmount.reset (currentSampleRate, smoothTime);
    smoothAirAmount.reset (currentSampleRate, smoothTime);
    smoothClipAmount.reset (currentSampleRate, smoothTime);

    smoothHpfFreq.setCurrentAndTargetValue (apvts.getRawParameterValue ("hpfFreq")->load());
    smoothCleanAmount.setCurrentAndTargetValue (apvts.getRawParameterValue ("cleanAmount")->load());
    smoothCompAmount.setCurrentAndTargetValue (apvts.getRawParameterValue ("compAmount")->load());
    smoothDeEssAmount.setCurrentAndTargetValue (apvts.getRawParameterValue ("deEssAmount")->load());
    smoothAirAmount.setCurrentAndTargetValue (apvts.getRawParameterValue ("airAmount")->load());
    smoothClipAmount.setCurrentAndTargetValue (apvts.getRawParameterValue ("clipAmount")->load());

    updateCleanEQ (smoothCleanAmount.getCurrentValue());
    outputLimiter.setThreshold (apvts.getRawParameterValue ("limiterCeiling")->load());
}

void VoxOneAudioProcessor::releaseResources()
{
}

bool VoxOneAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono()
        && layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;

    if (layouts.getMainOutputChannelSet() != layouts.getMainInputChannelSet())
        return false;

    return true;
}

void VoxOneAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ignoreUnused (midiMessages);
    juce::ScopedNoDenormals noDenormals;

    auto totalNumInputChannels = getTotalNumInputChannels();
    auto totalNumOutputChannels = getTotalNumOutputChannels();
    int numSamples = buffer.getNumSamples();

    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear (i, 0, numSamples);

    int monitorChannels = std::min (buffer.getNumChannels(), inputMonitorBuffer.getNumChannels());
    int copySamples = std::min (numSamples, inputMonitorBuffer.getNumSamples());

    for (int ch = 0; ch < monitorChannels; ++ch)
        inputMonitorBuffer.copyFrom (ch, 0, buffer, ch, 0, copySamples);

    bool isBypassed = apvts.getRawParameterValue ("bypass")->load() > 0.5f;

    if (isBypassed)
    {
        compGainReductionDb.store (0.0f, std::memory_order_relaxed);
        pushWaveformSamples (inputMonitorBuffer, buffer);
        return;
    }

    auto inputGainDb = apvts.getRawParameterValue ("inputGain")->load();
    auto hpfTarget = apvts.getRawParameterValue ("hpfFreq")->load();
    auto cleanTarget = apvts.getRawParameterValue ("cleanAmount")->load();
    auto compTarget = apvts.getRawParameterValue ("compAmount")->load();
    auto deEssTarget = apvts.getRawParameterValue ("deEssAmount")->load();
    auto airTarget = apvts.getRawParameterValue ("airAmount")->load();
    auto clipTarget = apvts.getRawParameterValue ("clipAmount")->load();
    auto outputGainDb = apvts.getRawParameterValue ("outputGain")->load();
    auto limiterCeiling = apvts.getRawParameterValue ("limiterCeiling")->load();

    smoothHpfFreq.setTargetValue (hpfTarget);
    smoothCleanAmount.setTargetValue (cleanTarget);
    smoothCompAmount.setTargetValue (compTarget);
    smoothDeEssAmount.setTargetValue (deEssTarget);
    smoothAirAmount.setTargetValue (airTarget);
    smoothClipAmount.setTargetValue (clipTarget);

    float hpfFreq = smoothHpfFreq.skip (numSamples);
    float cleanAmount = smoothCleanAmount.skip (numSamples);
    float compAmount = smoothCompAmount.skip (numSamples);
    float deEssAmount = smoothDeEssAmount.skip (numSamples);
    float airAmount = smoothAirAmount.skip (numSamples);
    float clipAmount = smoothClipAmount.skip (numSamples);

    inputGainProcessor.setGainDecibels (inputGainDb);
    outputGainProcessor.setGainDecibels (outputGainDb);
    outputLimiter.setThreshold (limiterCeiling);

    hpfStage1.setCutoffFrequency (hpfFreq);
    hpfStage2.setCutoffFrequency (hpfFreq);
    updateCleanEQ (cleanAmount);

    juce::dsp::AudioBlock<float> block (buffer);
    juce::dsp::ProcessContextReplacing<float> context (block);

    inputGainProcessor.process (context);

    if (hpfFreq > 20.5f)
    {
        hpfStage1.process (context);
        hpfStage2.process (context);
    }

    if (cleanAmount > 0.05f)
    {
        for (int ch = 0; ch < std::min (buffer.getNumChannels(), 2); ++ch)
        {
            auto* data = buffer.getWritePointer (ch);
            for (int i = 0; i < numSamples; ++i)
            {
                float s = mudCutFilter.processSample (ch, data[i]);
                s = boxCutFilter.processSample (ch, s);
                data[i] = presenceFilter.processSample (ch, s);
            }
        }
    }

    float grDb = vocalCompressor.processBlock (buffer, compAmount);
    compGainReductionDb.store (grDb, std::memory_order_relaxed);

    dynamicDeEsser.processBlock (buffer, deEssAmount);
    vocalExciter.processBlock (buffer, airAmount);
    VocalSoftClipper::processBlock (buffer, clipAmount);

    outputGainProcessor.process (context);
    outputLimiter.process (context);

    float ceilingLinear = juce::Decibels::decibelsToGain (limiterCeiling);
    for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
    {
        auto* data = buffer.getWritePointer (ch);
        for (int i = 0; i < numSamples; ++i)
            data[i] = juce::jlimit (-ceilingLinear, ceilingLinear, data[i]);
    }

    pushWaveformSamples (inputMonitorBuffer, buffer);
}

bool VoxOneAudioProcessor::hasEditor() const
{
    return true;
}

juce::AudioProcessorEditor* VoxOneAudioProcessor::createEditor()
{
    return new VoxOneAudioProcessorEditor (*this);
}

void VoxOneAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    auto state = apvts.copyState();
    std::unique_ptr<juce::XmlElement> xml (state.createXml());
    copyXmlToBinary (*xml, destData);
}

void VoxOneAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xmlState (getXmlFromBinary (data, sizeInBytes));

    if (xmlState != nullptr && xmlState->hasTagName (apvts.state.getType()))
        apvts.replaceState (juce::ValueTree::fromXml (*xmlState));
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new VoxOneAudioProcessor();
}