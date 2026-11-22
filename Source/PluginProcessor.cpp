#include "PluginProcessor.h"
#include "PluginEditor.h"

VoxOneAudioProcessor::VoxOneAudioProcessor()
    : AudioProcessor (BusesProperties()
                          .withInput ("Input", juce::AudioChannelSet::stereo(), true)
                          .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "Parameters", createParameterLayout())
{
}

VoxOneAudioProcessor::~VoxOneAudioProcessor()
{
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

    return { params.begin(), params.end() };
}

void VoxOneAudioProcessor::updateFilters (float hpfFreq, float cleanAmount, float airAmount)
{
    if (currentSampleRate <= 0.0)
        return;

    if (std::abs (hpfFreq - lastHpfFreq) > 0.01f)
    {
        *hpfFilter.state = *juce::dsp::IIR::Coefficients<float>::makeHighPass (
            currentSampleRate, hpfFreq, 0.7071f);
        lastHpfFreq = hpfFreq;
    }

    if (std::abs (cleanAmount - lastCleanAmount) > 0.01f)
    {
        float norm = cleanAmount * 0.01f;
        float mudCutDb = -4.5f * norm;
        float presenceBoostDb = 3.0f * norm;

        *mudCutFilter.state = *juce::dsp::IIR::Coefficients<float>::makePeakFilter (
            currentSampleRate, 250.0f, 1.2f, juce::Decibels::decibelsToGain (mudCutDb));

        *presenceFilter.state = *juce::dsp::IIR::Coefficients<float>::makePeakFilter (
            currentSampleRate, 3500.0f, 1.0f, juce::Decibels::decibelsToGain (presenceBoostDb));

        lastCleanAmount = cleanAmount;
    }

    if (std::abs (airAmount - lastAirAmount) > 0.01f)
    {
        float norm = airAmount * 0.01f;
        float shelfBoostDb = 4.5f * norm;

        *airShelfFilter.state = *juce::dsp::IIR::Coefficients<float>::makeHighShelf (
            currentSampleRate, 10000.0f, 0.7071f, juce::Decibels::decibelsToGain (shelfBoostDb));

        lastAirAmount = airAmount;
    }
}

void VoxOneAudioProcessor::updateCompressor (float compAmount)
{
    float norm = compAmount * 0.01f;
    float thresholdDb = -28.0f * norm;
    float ratio = 1.0f + 4.0f * norm;
    float makeupDb = 9.0f * norm;

    compressor.setThreshold (thresholdDb);
    compressor.setRatio (ratio);
    compressor.setAttack (10.0f);
    compressor.setRelease (80.0f);
    compMakeupProcessor.setGainDecibels (makeupDb);
}

void VoxOneAudioProcessor::updateDeEsser (float deEssAmount)
{
    float norm = deEssAmount * 0.01f;
    float thresholdDb = -36.0f * norm;
    float ratio = 1.0f + 7.0f * norm;

    deEssCompressor.setThreshold (thresholdDb);
    deEssCompressor.setRatio (ratio);
    deEssCompressor.setAttack (1.0f);
    deEssCompressor.setRelease (40.0f);
}

void VoxOneAudioProcessor::processDeEsser (juce::AudioBuffer<float>& buffer, float deEssAmount)
{
    if (deEssAmount <= 0.01f)
        return;

    int numChannels = buffer.getNumChannels();
    int numSamples = buffer.getNumSamples();

    sibilanceBuffer.setSize (numChannels, numSamples, false, false, true);
    sibilanceCompressedBuffer.setSize (numChannels, numSamples, false, false, true);

    for (int ch = 0; ch < numChannels; ++ch)
        sibilanceBuffer.copyFrom (ch, 0, buffer, ch, 0, numSamples);

    juce::dsp::AudioBlock<float> sibBlock (sibilanceBuffer);
    juce::dsp::ProcessContextReplacing<float> sibContext (sibBlock);
    deEssBandPass.process (sibContext);

    for (int ch = 0; ch < numChannels; ++ch)
        sibilanceCompressedBuffer.copyFrom (ch, 0, sibilanceBuffer, ch, 0, numSamples);

    juce::dsp::AudioBlock<float> compSibBlock (sibilanceCompressedBuffer);
    juce::dsp::ProcessContextReplacing<float> compSibContext (compSibBlock);
    deEssCompressor.process (compSibContext);

    for (int ch = 0; ch < numChannels; ++ch)
    {
        auto* mainData = buffer.getWritePointer (ch);
        const auto* origSib = sibilanceBuffer.getReadPointer (ch);
        const auto* compSib = sibilanceCompressedBuffer.getReadPointer (ch);

        for (int i = 0; i < numSamples; ++i)
        {
            float reduction = origSib[i] - compSib[i];
            mainData[i] -= reduction;
        }
    }
}

void VoxOneAudioProcessor::processAirExciter (juce::AudioBuffer<float>& buffer, float airAmount)
{
    if (airAmount <= 0.01f)
        return;

    int numChannels = buffer.getNumChannels();
    int numSamples = buffer.getNumSamples();
    float norm = airAmount * 0.01f;
    float drive = 1.0f + 2.5f * norm;
    float blend = 0.35f * norm;

    airBuffer.setSize (numChannels, numSamples, false, false, true);

    for (int ch = 0; ch < numChannels; ++ch)
        airBuffer.copyFrom (ch, 0, buffer, ch, 0, numSamples);

    juce::dsp::AudioBlock<float> airBlock (airBuffer);
    juce::dsp::ProcessContextReplacing<float> airContext (airBlock);
    airHighPass.process (airContext);

    for (int ch = 0; ch < numChannels; ++ch)
    {
        auto* mainData = buffer.getWritePointer (ch);
        auto* airData = airBuffer.getWritePointer (ch);

        for (int i = 0; i < numSamples; ++i)
        {
            float excited = std::tanh (airData[i] * drive) / drive;
            mainData[i] += excited * blend;
        }
    }

    juce::dsp::AudioBlock<float> mainBlock (buffer);
    juce::dsp::ProcessContextReplacing<float> mainContext (mainBlock);
    airShelfFilter.process (mainContext);
}

void VoxOneAudioProcessor::processSoftClipper (juce::AudioBuffer<float>& buffer, float clipAmount)
{
    if (clipAmount <= 0.01f)
        return;

    int numChannels = buffer.getNumChannels();
    int numSamples = buffer.getNumSamples();
    float norm = clipAmount * 0.01f;
    float drive = 1.0f + 3.0f * norm;
    float invTanhDrive = 1.0f / std::tanh (drive);

    for (int ch = 0; ch < numChannels; ++ch)
    {
        auto* data = buffer.getWritePointer (ch);

        for (int i = 0; i < numSamples; ++i)
            data[i] = std::tanh (data[i] * drive) * invTanhDrive;
    }
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
    return 1;
}

int VoxOneAudioProcessor::getCurrentProgram()
{
    return 0;
}

void VoxOneAudioProcessor::setCurrentProgram (int index)
{
    juce::ignoreUnused (index);
}

const juce::String VoxOneAudioProcessor::getProgramName (int index)
{
    juce::ignoreUnused (index);
    return {};
}

void VoxOneAudioProcessor::changeProgramName (int index, const juce::String& newName)
{
    juce::ignoreUnused (index, newName);
}

void VoxOneAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    currentSampleRate = sampleRate;
    lastHpfFreq = -1.0f;
    lastCleanAmount = -1.0f;
    lastAirAmount = -1.0f;

    juce::dsp::ProcessSpec spec;
    spec.sampleRate = sampleRate;
    spec.maximumBlockSize = static_cast<juce::uint32> (samplesPerBlock);
    spec.numChannels = static_cast<juce::uint32> (getTotalNumOutputChannels());

    inputGainProcessor.prepare (spec);
    inputGainProcessor.setRampDurationSeconds (0.02);

    hpfFilter.prepare (spec);
    hpfFilter.reset();

    mudCutFilter.prepare (spec);
    mudCutFilter.reset();

    presenceFilter.prepare (spec);
    presenceFilter.reset();

    compressor.prepare (spec);
    compressor.reset();

    compMakeupProcessor.prepare (spec);
    compMakeupProcessor.setRampDurationSeconds (0.02);

    deEssBandPass.prepare (spec);
    deEssBandPass.reset();
    *deEssBandPass.state = *juce::dsp::IIR::Coefficients<float>::makeBandPass (
        currentSampleRate, 6500.0f, 1.4f);

    deEssCompressor.prepare (spec);
    deEssCompressor.reset();

    sibilanceBuffer.setSize (static_cast<int> (spec.numChannels), samplesPerBlock);
    sibilanceCompressedBuffer.setSize (static_cast<int> (spec.numChannels), samplesPerBlock);

    airHighPass.prepare (spec);
    airHighPass.reset();
    *airHighPass.state = *juce::dsp::IIR::Coefficients<float>::makeHighPass (
        currentSampleRate, 5500.0f, 0.7071f);

    airShelfFilter.prepare (spec);
    airShelfFilter.reset();

    airBuffer.setSize (static_cast<int> (spec.numChannels), samplesPerBlock);

    outputGainProcessor.prepare (spec);
    outputGainProcessor.setRampDurationSeconds (0.02);

    outputLimiter.prepare (spec);
    outputLimiter.reset();
    outputLimiter.setRelease (50.0f);

    updateFilters (
        apvts.getRawParameterValue ("hpfFreq")->load(),
        apvts.getRawParameterValue ("cleanAmount")->load(),
        apvts.getRawParameterValue ("airAmount")->load());

    updateCompressor (apvts.getRawParameterValue ("compAmount")->load());
    updateDeEsser (apvts.getRawParameterValue ("deEssAmount")->load());
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

    auto totalNumInputChannels  = getTotalNumInputChannels();
    auto totalNumOutputChannels = getTotalNumOutputChannels();

    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear (i, 0, buffer.getNumSamples());

    auto inputGainDb     = apvts.getRawParameterValue ("inputGain")->load();
    auto hpfFreq         = apvts.getRawParameterValue ("hpfFreq")->load();
    auto cleanAmount     = apvts.getRawParameterValue ("cleanAmount")->load();
    auto compAmount      = apvts.getRawParameterValue ("compAmount")->load();
    auto deEssAmount     = apvts.getRawParameterValue ("deEssAmount")->load();
    auto airAmount       = apvts.getRawParameterValue ("airAmount")->load();
    auto clipAmount      = apvts.getRawParameterValue ("clipAmount")->load();
    auto outputGainDb    = apvts.getRawParameterValue ("outputGain")->load();
    auto limiterCeiling  = apvts.getRawParameterValue ("limiterCeiling")->load();

    inputGainProcessor.setGainDecibels (inputGainDb);
    outputGainProcessor.setGainDecibels (outputGainDb);
    outputLimiter.setThreshold (limiterCeiling);
    updateFilters (hpfFreq, cleanAmount, airAmount);
    updateCompressor (compAmount);
    updateDeEsser (deEssAmount);

    juce::dsp::AudioBlock<float> block (buffer);
    juce::dsp::ProcessContextReplacing<float> context (block);

    inputGainProcessor.process (context);
    hpfFilter.process (context);
    mudCutFilter.process (context);
    presenceFilter.process (context);
    compressor.process (context);
    compMakeupProcessor.process (context);

    processDeEsser (buffer, deEssAmount);
    processAirExciter (buffer, airAmount);
    processSoftClipper (buffer, clipAmount);

    outputGainProcessor.process (context);
    outputLimiter.process (context);
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