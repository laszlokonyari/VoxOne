#pragma once

#include <juce_dsp/juce_dsp.h>
#include "VocalDSP.h"
#include <algorithm>
#include <cmath>
#include <vector>

struct SpaceProcessMetrics
{
    float wetPeak { 0.0f };
    float duckAmount { 0.0f };
};

class VocalSlapDelay
{
public:
    void prepare (double newSampleRate, int maxBlockSize)
    {
        juce::ignoreUnused (maxBlockSize);
        sampleRate = std::max (1.0, newSampleRate);

        int minSize = static_cast<int> (sampleRate * 1.0) + 64;
        int pow2Size = 1;
        while (pow2Size < minSize)
            pow2Size <<= 1;

        bufferMask = pow2Size - 1;
        maxDelaySamples = static_cast<float> (pow2Size - 8);
        bufL.assign (static_cast<size_t> (pow2Size), 0.0f);
        bufR.assign (static_cast<size_t> (pow2Size), 0.0f);
        writePos = 0;

        attackCoeff = static_cast<float> (std::exp (-1.0 / (0.005 * sampleRate)));
        releaseCoeff = static_cast<float> (std::exp (-1.0 / (0.140 * sampleRate)));
        env = 0.0f;

        smoothTimeSamples.reset (sampleRate, 0.045);
        smoothMix.reset (sampleRate, 0.025);
        smoothTimeSamples.setCurrentAndTargetValue (static_cast<float> (0.115 * sampleRate));
        smoothMix.setCurrentAndTargetValue (0.0f);

        lastTone = -1.0f;
        toneHpf.reset();
        toneLpf.reset();
        updateTone (45.0f);
    }

    void reset()
    {
        std::fill (bufL.begin(), bufL.end(), 0.0f);
        std::fill (bufR.begin(), bufR.end(), 0.0f);
        writePos = 0;
        env = 0.0f;
        toneHpf.reset();
        toneLpf.reset();
    }

    SpaceProcessMetrics processBlock (const juce::AudioBuffer<float>& dryBuffer,
                                      juce::AudioBuffer<float>& wetOutBuffer,
                                      int numSamples,
                                      float mixPercent,
                                      float timeMs,
                                      float feedbackPercent,
                                      float tonePercent,
                                      float duckPercent,
                                      bool pingPong)
    {
        SpaceProcessMetrics metrics;
        int numCh = std::min (dryBuffer.getNumChannels(), wetOutBuffer.getNumChannels());

        if (numCh <= 0 || numSamples <= 0 || bufL.empty())
            return metrics;

        updateTone (tonePercent);

        float targetSamples = juce::jlimit (1.0f, maxDelaySamples * 0.90f,
                                            static_cast<float> (timeMs * 0.001 * sampleRate));
        smoothTimeSamples.setTargetValue (targetSamples);
        smoothMix.setTargetValue (mixPercent * 0.01f);

        float fbGain = juce::jlimit (0.0f, 0.78f, feedbackPercent * 0.01f);
        float duckNorm = duckPercent * 0.01f;

        const auto* dryL = dryBuffer.getReadPointer (0);
        const auto* dryR = (numCh > 1) ? dryBuffer.getReadPointer (1) : nullptr;
        auto* wetL = wetOutBuffer.getWritePointer (0);
        auto* wetR = (numCh > 1) ? wetOutBuffer.getWritePointer (1) : nullptr;

        for (int i = 0; i < numSamples; ++i)
        {
            float inL = dryL[i];
            float inR = (dryR != nullptr) ? dryR[i] : inL;

            float detector = std::max (std::abs (inL), std::abs (inR));
            float envCoeff = (detector > env) ? attackCoeff : releaseCoeff;
            env = envCoeff * env + (1.0f - envCoeff) * detector;

            float envDb = juce::Decibels::gainToDecibels (env, -96.0f);
            float overDb = std::max (0.0f, envDb - (-32.0f));
            float duckDb = std::min (18.0f, overDb * 0.65f) * duckNorm;
            float duckGain = juce::Decibels::decibelsToGain (-duckDb);
            metrics.duckAmount = std::max (metrics.duckAmount, duckDb / 18.0f);

            float dSamplesL = smoothTimeSamples.getNextValue();
            float dSamplesR = pingPong ? dSamplesL : std::min (maxDelaySamples, dSamplesL * 1.075f);
            float mixGain = smoothMix.getNextValue();

            float readL = readInterpolated (bufL, dSamplesL);
            float readR = readInterpolated (bufR, dSamplesR);

            float filtL = toneLpf.processSample (0, toneHpf.processSample (0, readL));
            float filtR = toneLpf.processSample (1, toneHpf.processSample (1, readR));

            if (pingPong)
            {
                float monoIn = 0.5f * (inL + inR);
                bufL[static_cast<size_t> (writePos)] = softSat (monoIn + filtR * fbGain);
                bufR[static_cast<size_t> (writePos)] = softSat (filtL * fbGain);
            }
            else
            {
                bufL[static_cast<size_t> (writePos)] = softSat (inL + filtL * fbGain);
                bufR[static_cast<size_t> (writePos)] = softSat (inR + filtR * fbGain);
            }

            writePos = (writePos + 1) & bufferMask;

            float outL = filtL * mixGain * duckGain;
            float outR = filtR * mixGain * duckGain;

            wetL[i] = outL;
            if (wetR != nullptr)
                wetR[i] = outR;

            metrics.wetPeak = std::max (metrics.wetPeak, std::max (std::abs (outL), std::abs (outR)));
        }

        return metrics;
    }

private:
    static inline float softSat (float x)
    {
        return std::tanh (x * 0.85f) * 1.17647f;
    }

    inline float readInterpolated (const std::vector<float>& buf, float delaySamples) const
    {
        float readPos = static_cast<float> (writePos) - delaySamples;
        int idx0 = static_cast<int> (std::floor (readPos));
        float frac = readPos - static_cast<float> (idx0);
        int i0 = idx0 & bufferMask;
        int i1 = (idx0 + 1) & bufferMask;
        return buf[static_cast<size_t> (i0)] + frac * (buf[static_cast<size_t> (i1)] - buf[static_cast<size_t> (i0)]);
    }

    void updateTone (float tonePercent)
    {
        if (std::abs (tonePercent - lastTone) <= 0.1f)
            return;

        float norm = tonePercent * 0.01f;
        float hpfFreq = 380.0f - 210.0f * norm;
        float lpfFreq = 1900.0f * std::pow (6.2f, norm);

        toneHpf.setHighPass (sampleRate, hpfFreq, 0.7071f);
        toneLpf.setHighShelf (sampleRate, lpfFreq * 0.45f, 0.7071f, -8.5f * (1.0f - norm));
        lastTone = tonePercent;
    }

    double sampleRate { 44100.0 };
    std::vector<float> bufL;
    std::vector<float> bufR;
    int bufferMask { 0 };
    int writePos { 0 };
    float maxDelaySamples { 44100.0f };

    juce::SmoothedValue<float> smoothTimeSamples;
    juce::SmoothedValue<float> smoothMix;

    float attackCoeff { 0.99f };
    float releaseCoeff { 0.999f };
    float env { 0.0f };
    float lastTone { -1.0f };

    StereoBiquad toneHpf;
    StereoBiquad toneLpf;
};

class VocalReverb
{
public:
    void prepare (double newSampleRate, int maxBlockSize)
    {
        sampleRate = std::max (1.0, newSampleRate);

        juce::dsp::ProcessSpec spec;
        spec.sampleRate = sampleRate;
        spec.maximumBlockSize = static_cast<juce::uint32> (std::max (maxBlockSize, 4096));
        spec.numChannels = 2;

        reverb.prepare (spec);
        reverb.reset();

        int minPreSize = static_cast<int> (sampleRate * 0.18) + 64;
        int pow2Size = 1;
        while (pow2Size < minPreSize)
            pow2Size <<= 1;

        preMask = pow2Size - 1;
        maxPreSamples = static_cast<float> (pow2Size - 16);
        preBufL.assign (static_cast<size_t> (pow2Size), 0.0f);
        preBufR.assign (static_cast<size_t> (pow2Size), 0.0f);
        preWritePos = 0;

        lfoPhase = 0.0f;
        lfoInc = static_cast<float> (juce::MathConstants<double>::twoPi * 0.48 / sampleRate);
        modDepthSamples = static_cast<float> (0.00035 * sampleRate);

        attackCoeff = static_cast<float> (std::exp (-1.0 / (0.006 * sampleRate)));
        releaseCoeff = static_cast<float> (std::exp (-1.0 / (0.180 * sampleRate)));
        env = 0.0f;

        smoothPreDelay.reset (sampleRate, 0.035);
        smoothMix.reset (sampleRate, 0.025);
        smoothPreDelay.setCurrentAndTargetValue (static_cast<float> (0.024 * sampleRate));
        smoothMix.setCurrentAndTargetValue (0.0f);

        lastTone = -1.0f;
        lastMode = -1;
        preHpf.reset();
        postToneShelf.reset();
        updateFilters (60.0f, 1);
    }

    void reset()
    {
        reverb.reset();
        std::fill (preBufL.begin(), preBufL.end(), 0.0f);
        std::fill (preBufR.begin(), preBufR.end(), 0.0f);
        preWritePos = 0;
        env = 0.0f;
        preHpf.reset();
        postToneShelf.reset();
    }

    SpaceProcessMetrics processBlock (const juce::AudioBuffer<float>& dryBuffer,
                                      const juce::AudioBuffer<float>& delaySendBuffer,
                                      juce::AudioBuffer<float>& wetOutBuffer,
                                      int numSamples,
                                      int modeIndex,
                                      float mixPercent,
                                      float sizePercent,
                                      float preDelayMs,
                                      float tonePercent,
                                      float widthPercent,
                                      float duckPercent)
    {
        SpaceProcessMetrics metrics;
        int numCh = std::min (dryBuffer.getNumChannels(), wetOutBuffer.getNumChannels());

        if (numCh <= 0 || numSamples <= 0 || preBufL.empty())
            return metrics;

        updateFilters (tonePercent, modeIndex);

        float sizeNorm = sizePercent * 0.01f;
        float toneNorm = tonePercent * 0.01f;
        float widthNorm = widthPercent * 0.01f;
        float duckNorm = duckPercent * 0.01f;

        juce::dsp::Reverb::Parameters rp;
        rp.dryLevel = 0.0f;
        rp.wetLevel = 1.0f;
        rp.freezeMode = 0.0f;
        rp.width = widthNorm;

        switch (modeIndex)
        {
            case 0:
                rp.roomSize = 0.24f + 0.48f * sizeNorm;
                rp.damping = 0.72f - 0.44f * toneNorm;
                break;
            case 1:
                rp.roomSize = 0.36f + 0.50f * sizeNorm;
                rp.damping = 0.50f - 0.38f * toneNorm;
                break;
            case 2:
                rp.roomSize = 0.52f + 0.43f * sizeNorm;
                rp.damping = 0.64f - 0.46f * toneNorm;
                break;
            default:
                rp.roomSize = 0.14f + 0.32f * sizeNorm;
                rp.damping = 0.44f - 0.32f * toneNorm;
                break;
        }

        reverb.setParameters (rp);

        float targetPre = juce::jlimit (2.0f, maxPreSamples - modDepthSamples - 2.0f,
                                        static_cast<float> (preDelayMs * 0.001 * sampleRate) + 2.0f);
        smoothPreDelay.setTargetValue (targetPre);
        smoothMix.setTargetValue (mixPercent * 0.01f);

        const auto* dryL = dryBuffer.getReadPointer (0);
        const auto* dryR = (numCh > 1) ? dryBuffer.getReadPointer (1) : nullptr;
        const auto* delL = delaySendBuffer.getReadPointer (0);
        const auto* delR = (numCh > 1) ? delaySendBuffer.getReadPointer (1) : nullptr;
        auto* wetL = wetOutBuffer.getWritePointer (0);
        auto* wetR = (numCh > 1) ? wetOutBuffer.getWritePointer (1) : nullptr;

        for (int i = 0; i < numSamples; ++i)
        {
            float inL = dryL[i] + delL[i] * 0.18f;
            float inR = ((dryR != nullptr) ? dryR[i] : dryL[i]) + ((delR != nullptr) ? delR[i] : delL[i]) * 0.18f;

            preBufL[static_cast<size_t> (preWritePos)] = preHpf.processSample (0, inL);
            preBufR[static_cast<size_t> (preWritePos)] = preHpf.processSample (1, inR);

            float preSamp = smoothPreDelay.getNextValue();
            float mod = std::sin (lfoPhase) * modDepthSamples;
            lfoPhase += lfoInc;
            if (lfoPhase >= juce::MathConstants<float>::twoPi)
                lfoPhase -= juce::MathConstants<float>::twoPi;

            wetL[i] = readPre (preBufL, std::max (1.0f, preSamp + mod));
            if (wetR != nullptr)
                wetR[i] = readPre (preBufR, std::max (1.0f, preSamp - mod));

            preWritePos = (preWritePos + 1) & preMask;
        }

        juce::dsp::AudioBlock<float> wetBlock (wetOutBuffer.getArrayOfWritePointers(),
                                               static_cast<size_t> (numCh),
                                               static_cast<size_t> (numSamples));
        juce::dsp::ProcessContextReplacing<float> wetContext (wetBlock);
        reverb.process (wetContext);

        float modeGain = (modeIndex == 3) ? 1.65f : 1.45f;

        for (int i = 0; i < numSamples; ++i)
        {
            float dL = dryL[i];
            float dR = (dryR != nullptr) ? dryR[i] : dL;
            float detector = std::max (std::abs (dL), std::abs (dR));
            float envCoeff = (detector > env) ? attackCoeff : releaseCoeff;
            env = envCoeff * env + (1.0f - envCoeff) * detector;

            float envDb = juce::Decibels::gainToDecibels (env, -96.0f);
            float overDb = std::max (0.0f, envDb - (-32.0f));
            float duckDb = std::min (18.0f, overDb * 0.70f) * duckNorm;
            float duckGain = juce::Decibels::decibelsToGain (-duckDb);
            metrics.duckAmount = std::max (metrics.duckAmount, duckDb / 18.0f);

            float mixGain = smoothMix.getNextValue() * modeGain * duckGain;
            float outL = postToneShelf.processSample (0, wetL[i]) * mixGain;
            wetL[i] = outL;

            float outR = outL;
            if (wetR != nullptr)
            {
                outR = postToneShelf.processSample (1, wetR[i]) * mixGain;
                wetR[i] = outR;
            }

            metrics.wetPeak = std::max (metrics.wetPeak, std::max (std::abs (outL), std::abs (outR)));
        }

        return metrics;
    }

private:
    inline float readPre (const std::vector<float>& buf, float delaySamples) const
    {
        float readPos = static_cast<float> (preWritePos) - delaySamples;
        int idx0 = static_cast<int> (std::floor (readPos));
        float frac = readPos - static_cast<float> (idx0);
        int i0 = idx0 & preMask;
        int i1 = (idx0 + 1) & preMask;
        return buf[static_cast<size_t> (i0)] + frac * (buf[static_cast<size_t> (i1)] - buf[static_cast<size_t> (i0)]);
    }

    void updateFilters (float tonePercent, int modeIndex)
    {
        if (std::abs (tonePercent - lastTone) <= 0.1f && modeIndex == lastMode)
            return;

        float norm = tonePercent * 0.01f;
        float hpfFreq = 260.0f;
        if (modeIndex == 1)
            hpfFreq = 320.0f;
        else if (modeIndex == 2)
            hpfFreq = 210.0f;
        else if (modeIndex == 3)
            hpfFreq = 360.0f;

        preHpf.setHighPass (sampleRate, hpfFreq, 0.7071f);
        float shelfDb = -7.5f + 11.0f * norm;
        postToneShelf.setHighShelf (sampleRate, 3200.0f, 0.7071f, shelfDb);

        lastTone = tonePercent;
        lastMode = modeIndex;
    }

    double sampleRate { 44100.0 };
    juce::dsp::Reverb reverb;

    std::vector<float> preBufL;
    std::vector<float> preBufR;
    int preMask { 0 };
    int preWritePos { 0 };
    float maxPreSamples { 4096.0f };

    float lfoPhase { 0.0f };
    float lfoInc { 0.0001f };
    float modDepthSamples { 12.0f };

    juce::SmoothedValue<float> smoothPreDelay;
    juce::SmoothedValue<float> smoothMix;

    float attackCoeff { 0.99f };
    float releaseCoeff { 0.999f };
    float env { 0.0f };

    float lastTone { -1.0f };
    int lastMode { -1 };
    StereoBiquad preHpf;
    StereoBiquad postToneShelf;
};