#pragma once

#include <juce_dsp/juce_dsp.h>
#include <algorithm>
#include <array>
#include <cmath>

class StereoBiquad
{
public:
    void reset()
    {
        z1.fill (0.0f);
        z2.fill (0.0f);
    }

    void setIdentity()
    {
        b0 = 1.0f;
        b1 = 0.0f;
        b2 = 0.0f;
        a1 = 0.0f;
        a2 = 0.0f;
    }

    void setHighPass (double sampleRate, float freq, float q)
    {
        if (sampleRate <= 0.0)
            return;

        double clampedFreq = std::clamp (static_cast<double> (freq), 20.0, sampleRate * 0.45);
        double w0 = 2.0 * juce::MathConstants<double>::pi * clampedFreq / sampleRate;
        double cosw0 = std::cos (w0);
        double alpha = std::sin (w0) / (2.0 * static_cast<double> (q));

        double nb0 = (1.0 + cosw0) * 0.5;
        double nb1 = -(1.0 + cosw0);
        double nb2 = (1.0 + cosw0) * 0.5;
        double na0 = 1.0 + alpha;
        double na1 = -2.0 * cosw0;
        double na2 = 1.0 - alpha;

        assignNormalized (nb0, nb1, nb2, na0, na1, na2);
    }

    void setBandPass (double sampleRate, float freq, float q)
    {
        if (sampleRate <= 0.0)
            return;

        double clampedFreq = std::clamp (static_cast<double> (freq), 20.0, sampleRate * 0.45);
        double w0 = 2.0 * juce::MathConstants<double>::pi * clampedFreq / sampleRate;
        double cosw0 = std::cos (w0);
        double alpha = std::sin (w0) / (2.0 * static_cast<double> (q));

        double nb0 = alpha;
        double nb1 = 0.0;
        double nb2 = -alpha;
        double na0 = 1.0 + alpha;
        double na1 = -2.0 * cosw0;
        double na2 = 1.0 - alpha;

        assignNormalized (nb0, nb1, nb2, na0, na1, na2);
    }

    void setPeak (double sampleRate, float freq, float q, float gainDb)
    {
        if (sampleRate <= 0.0)
            return;

        if (std::abs (gainDb) < 0.01f)
        {
            setIdentity();
            return;
        }

        double clampedFreq = std::clamp (static_cast<double> (freq), 20.0, sampleRate * 0.45);
        double a = std::pow (10.0, static_cast<double> (gainDb) / 40.0);
        double w0 = 2.0 * juce::MathConstants<double>::pi * clampedFreq / sampleRate;
        double cosw0 = std::cos (w0);
        double alpha = std::sin (w0) / (2.0 * static_cast<double> (q));

        double nb0 = 1.0 + alpha * a;
        double nb1 = -2.0 * cosw0;
        double nb2 = 1.0 - alpha * a;
        double na0 = 1.0 + alpha / a;
        double na1 = -2.0 * cosw0;
        double na2 = 1.0 - alpha / a;

        assignNormalized (nb0, nb1, nb2, na0, na1, na2);
    }

    void setHighShelf (double sampleRate, float freq, float q, float gainDb)
    {
        if (sampleRate <= 0.0)
            return;

        if (std::abs (gainDb) < 0.01f)
        {
            setIdentity();
            return;
        }

        double clampedFreq = std::clamp (static_cast<double> (freq), 20.0, sampleRate * 0.45);
        double a = std::pow (10.0, static_cast<double> (gainDb) / 40.0);
        double w0 = 2.0 * juce::MathConstants<double>::pi * clampedFreq / sampleRate;
        double cosw0 = std::cos (w0);
        double alpha = std::sin (w0) / (2.0 * static_cast<double> (q));
        double twoSqrtAAlpha = 2.0 * std::sqrt (a) * alpha;

        double nb0 = a * ((a + 1.0) + (a - 1.0) * cosw0 + twoSqrtAAlpha);
        double nb1 = -2.0 * a * ((a - 1.0) + (a + 1.0) * cosw0);
        double nb2 = a * ((a + 1.0) + (a - 1.0) * cosw0 - twoSqrtAAlpha);
        double na0 = (a + 1.0) - (a - 1.0) * cosw0 + twoSqrtAAlpha;
        double na1 = 2.0 * ((a - 1.0) - (a + 1.0) * cosw0);
        double na2 = (a + 1.0) - (a - 1.0) * cosw0 - twoSqrtAAlpha;

        assignNormalized (nb0, nb1, nb2, na0, na1, na2);
    }

    inline float processSample (int channel, float x)
    {
        if (channel < 0 || channel >= 2)
            return x;

        float y = b0 * x + z1[static_cast<size_t> (channel)];
        z1[static_cast<size_t> (channel)] = b1 * x - a1 * y + z2[static_cast<size_t> (channel)];
        z2[static_cast<size_t> (channel)] = b2 * x - a2 * y;
        return y;
    }

private:
    void assignNormalized (double nb0, double nb1, double nb2, double na0, double na1, double na2)
    {
        double invA0 = 1.0 / na0;
        b0 = static_cast<float> (nb0 * invA0);
        b1 = static_cast<float> (nb1 * invA0);
        b2 = static_cast<float> (nb2 * invA0);
        a1 = static_cast<float> (na1 * invA0);
        a2 = static_cast<float> (na2 * invA0);
    }

    float b0 { 1.0f };
    float b1 { 0.0f };
    float b2 { 0.0f };
    float a1 { 0.0f };
    float a2 { 0.0f };
    std::array<float, 2> z1 { 0.0f, 0.0f };
    std::array<float, 2> z2 { 0.0f, 0.0f };
};

class VocalCompressor
{
public:
    void prepare (double newSampleRate)
    {
        sampleRate = std::max (1.0, newSampleRate);
        attackCoeff = static_cast<float> (std::exp (-1.0 / (0.008 * sampleRate)));
        releaseCoeff = static_cast<float> (std::exp (-1.0 / (0.095 * sampleRate)));
        reset();
    }

    void reset()
    {
        envelope = 0.0f;
        lastGainReductionDb = 0.0f;
    }

    float processBlock (juce::AudioBuffer<float>& buffer, float compAmount)
    {
        int numChannels = buffer.getNumChannels();
        int numSamples = buffer.getNumSamples();

        if (compAmount <= 0.01f || numChannels == 0)
        {
            lastGainReductionDb = 0.0f;
            return 0.0f;
        }

        float norm = compAmount * 0.01f;
        float thresholdDb = -2.0f - 26.0f * norm;
        float ratio = 1.0f + 4.5f * norm;
        float makeupDb = 5.5f * norm;
        float kneeDb = 6.0f;
        float halfKnee = kneeDb * 0.5f;
        float slope = 1.0f - (1.0f / ratio);

        auto* leftData = buffer.getWritePointer (0);
        auto* rightData = (numChannels > 1) ? buffer.getWritePointer (1) : nullptr;

        float maxGrDb = 0.0f;

        for (int i = 0; i < numSamples; ++i)
        {
            float absL = std::abs (leftData[i]);
            float absR = (rightData != nullptr) ? std::abs (rightData[i]) : absL;
            float detector = std::max (absL, absR);

            float coeff = (detector > envelope) ? attackCoeff : releaseCoeff;
            envelope = coeff * envelope + (1.0f - coeff) * detector;

            float envDb = juce::Decibels::gainToDecibels (envelope, -96.0f);
            float diff = envDb - thresholdDb;
            float grDb = 0.0f;

            if (diff >= halfKnee)
            {
                grDb = diff * slope;
            }
            else if (diff > -halfKnee)
            {
                float x = diff + halfKnee;
                grDb = slope * (x * x) / (2.0f * kneeDb);
            }

            if (grDb > maxGrDb)
                maxGrDb = grDb;

            float totalGain = juce::Decibels::decibelsToGain (-grDb + makeupDb);
            leftData[i] *= totalGain;

            if (rightData != nullptr)
                rightData[i] *= totalGain;
        }

        lastGainReductionDb = maxGrDb;
        return maxGrDb;
    }

    float getLastGainReductionDb() const
    {
        return lastGainReductionDb;
    }

private:
    double sampleRate { 44100.0 };
    float attackCoeff { 0.99f };
    float releaseCoeff { 0.999f };
    float envelope { 0.0f };
    float lastGainReductionDb { 0.0f };
};

class DynamicDeEsser
{
public:
    void prepare (double newSampleRate)
    {
        sampleRate = std::max (1.0, newSampleRate);
        attackCoeff = static_cast<float> (std::exp (-1.0 / (0.0015 * sampleRate)));
        releaseCoeff = static_cast<float> (std::exp (-1.0 / (0.042 * sampleRate)));
        sidechainFilter.setBandPass (sampleRate, 7200.0f, 1.35f);
        dynamicCutFilter.setIdentity();
        reset();
    }

    void reset()
    {
        sidechainFilter.reset();
        dynamicCutFilter.reset();
        envelope = 0.0f;
        currentCutDb = 0.0f;
        updateCounter = 0;
    }

    void processBlock (juce::AudioBuffer<float>& buffer, float deEssAmount)
    {
        int numChannels = buffer.getNumChannels();
        int numSamples = buffer.getNumSamples();

        if (deEssAmount <= 0.01f || numChannels == 0)
        {
            if (std::abs (currentCutDb) > 0.01f)
            {
                currentCutDb = 0.0f;
                dynamicCutFilter.setIdentity();
            }
            return;
        }

        float norm = deEssAmount * 0.01f;
        float thresholdDb = -6.0f - 28.0f * norm;
        float ratioScale = 0.85f * norm;

        auto* leftData = buffer.getWritePointer (0);
        auto* rightData = (numChannels > 1) ? buffer.getWritePointer (1) : nullptr;

        for (int i = 0; i < numSamples; ++i)
        {
            float scL = sidechainFilter.processSample (0, leftData[i]);
            float scR = (rightData != nullptr) ? sidechainFilter.processSample (1, rightData[i]) : scL;
            float detector = std::max (std::abs (scL), std::abs (scR));

            float coeff = (detector > envelope) ? attackCoeff : releaseCoeff;
            envelope = coeff * envelope + (1.0f - coeff) * detector;

            if ((updateCounter & 15) == 0)
            {
                float envDb = juce::Decibels::gainToDecibels (envelope, -96.0f);
                float overDb = std::max (0.0f, envDb - thresholdDb);
                float targetCutDb = -std::min (12.0f, overDb * ratioScale);

                if (std::abs (targetCutDb - currentCutDb) > 0.05f)
                {
                    currentCutDb = targetCutDb;
                    dynamicCutFilter.setPeak (sampleRate, 7200.0f, 1.25f, currentCutDb);
                }
            }
            ++updateCounter;

            leftData[i] = dynamicCutFilter.processSample (0, leftData[i]);
            if (rightData != nullptr)
                rightData[i] = dynamicCutFilter.processSample (1, rightData[i]);
        }
    }

private:
    double sampleRate { 44100.0 };
    float attackCoeff { 0.99f };
    float releaseCoeff { 0.999f };
    float envelope { 0.0f };
    float currentCutDb { 0.0f };
    int updateCounter { 0 };
    StereoBiquad sidechainFilter;
    StereoBiquad dynamicCutFilter;
};

class VocalExciter
{
public:
    void prepare (double newSampleRate)
    {
        sampleRate = std::max (1.0, newSampleRate);
        preHighPass.setHighPass (sampleRate, 5000.0f, 0.7071f);
        postHighPass.setHighPass (sampleRate, 5500.0f, 0.7071f);
        lastAirAmount = -1.0f;
        reset();
    }

    void reset()
    {
        preHighPass.reset();
        postHighPass.reset();
        airShelf.reset();
    }

    void processBlock (juce::AudioBuffer<float>& buffer, float airAmount)
    {
        int numChannels = buffer.getNumChannels();
        int numSamples = buffer.getNumSamples();

        if (airAmount <= 0.01f || numChannels == 0)
            return;

        if (std::abs (airAmount - lastAirAmount) > 0.05f)
        {
            float shelfBoostDb = 3.8f * (airAmount * 0.01f);
            airShelf.setHighShelf (sampleRate, 10500.0f, 0.7071f, shelfBoostDb);
            lastAirAmount = airAmount;
        }

        float norm = airAmount * 0.01f;
        float drive = 1.0f + 2.2f * norm;
        float invDrive = 1.0f / drive;
        float blend = 0.38f * norm;

        for (int ch = 0; ch < std::min (numChannels, 2); ++ch)
        {
            auto* data = buffer.getWritePointer (ch);

            for (int i = 0; i < numSamples; ++i)
            {
                float dry = data[i];
                float hi = preHighPass.processSample (ch, dry);
                float sat = std::tanh (hi * drive + 0.12f * hi * hi * drive) * invDrive;
                float cleanHi = postHighPass.processSample (ch, sat);
                data[i] = airShelf.processSample (ch, dry + cleanHi * blend);
            }
        }
    }

private:
    double sampleRate { 44100.0 };
    float lastAirAmount { -1.0f };
    StereoBiquad preHighPass;
    StereoBiquad postHighPass;
    StereoBiquad airShelf;
};

class VocalSoftClipper
{
public:
    static void processBlock (juce::AudioBuffer<float>& buffer, float clipAmount)
    {
        if (clipAmount <= 0.01f)
            return;

        float norm = clipAmount * 0.01f;
        float preDrive = 1.0f + 0.55f * norm;
        float thresh = 1.0f - 0.60f * norm;
        float headroom = std::max (0.05f, 1.0f - thresh);
        float invHeadroom = 1.0f / headroom;

        int numChannels = buffer.getNumChannels();
        int numSamples = buffer.getNumSamples();

        for (int ch = 0; ch < numChannels; ++ch)
        {
            auto* data = buffer.getWritePointer (ch);

            for (int i = 0; i < numSamples; ++i)
            {
                float x = data[i] * preDrive;
                float absX = std::abs (x);

                if (absX > thresh)
                {
                    float shaped = thresh + headroom * std::tanh ((absX - thresh) * invHeadroom);
                    x = (x >= 0.0f) ? shaped : -shaped;
                }

                data[i] = x;
            }
        }
    }
};