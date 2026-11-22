#pragma once

#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_gui_basics/juce_gui_basics.h>
#include <algorithm>
#include <array>
#include <cmath>

class WaveformVisualizer : public juce::Component
{
public:
    static constexpr int numPoints = 256;

    void setDisplayData (const std::array<float, numPoints>& inData,
                         const std::array<float, numPoints>& outData,
                         float inLevelLin,
                         float outLevelLin,
                         float gainReductionDb,
                         bool isBypassed)
    {
        inputWave = inData;
        outputWave = outData;
        bypassed = isBypassed;

        float targetBypass = isBypassed ? 1.0f : 0.0f;
        bypassBlend += (targetBypass - bypassBlend) * 0.16f;

        float inDb = juce::Decibels::gainToDecibels (inLevelLin, -60.0f);
        float outDb = juce::Decibels::gainToDecibels (outLevelLin, -60.0f);

        float normIn = juce::jlimit (0.0f, 1.0f, (inDb + 60.0f) / 60.0f);
        float normOut = juce::jlimit (0.0f, 1.0f, (outDb + 60.0f) / 60.0f);
        float normGr = juce::jlimit (0.0f, 1.0f, gainReductionDb / 18.0f);

        smoothIn = (normIn > smoothIn) ? normIn : (smoothIn * 0.85f + normIn * 0.15f);
        smoothOut = (normOut > smoothOut) ? normOut : (smoothOut * 0.85f + normOut * 0.15f);
        smoothGr = (normGr > smoothGr) ? normGr : (smoothGr * 0.84f + normGr * 0.16f);
        smoothGrDb = (gainReductionDb > smoothGrDb) ? gainReductionDb : (smoothGrDb * 0.84f + gainReductionDb * 0.16f);

        updatePeakHold (normIn, peakIn, holdIn);
        updatePeakHold (normOut, peakOut, holdOut);
        updatePeakHold (normGr, peakGr, holdGr);

        repaint();
    }

    void paint (juce::Graphics& g) override
    {
        auto bounds = getLocalBounds().toFloat();

        juce::ColourGradient bgGrad (juce::Colour (0xff0c0f18), bounds.getX(), bounds.getY(),
                                     juce::Colour (0xff131826), bounds.getX(), bounds.getBottom(), false);
        g.setGradientFill (bgGrad);
        g.fillRoundedRectangle (bounds, 14.0f);

        g.setColour (juce::Colours::white.withAlpha (0.05f));
        g.drawHorizontalLine (static_cast<int> (bounds.getY() + 1.0f), bounds.getX() + 14.0f, bounds.getRight() - 14.0f);

        g.setColour (juce::Colour (0xff232b42));
        g.drawRoundedRectangle (bounds.reduced (0.5f), 14.0f, 1.2f);

        auto content = bounds.reduced (14.0f, 10.0f);
        auto leftMeterArea = content.removeFromLeft (26.0f);
        content.removeFromLeft (10.0f);
        auto rightMetersArea = content.removeFromRight (58.0f);
        content.removeFromRight (10.0f);

        drawMeter (g, leftMeterArea, smoothIn, peakIn, "IN",
                   juce::Colour (0xff3b82f6), juce::Colour (0xff00e5ff), false);

        auto grMeterArea = rightMetersArea.removeFromLeft (25.0f);
        rightMetersArea.removeFromLeft (8.0f);
        auto outMeterArea = rightMetersArea;

        drawMeter (g, grMeterArea, smoothGr, peakGr, "GR",
                   juce::Colour (0xffff9f43), juce::Colour (0xffff3d71), true);
        drawMeter (g, outMeterArea, smoothOut, peakOut, "OUT",
                   juce::Colour (0xff7c4dff), juce::Colour (0xff00e5ff), false);

        drawWaveformArea (g, content);
    }

private:
    static void updatePeakHold (float current, float& peak, int& holdFrames)
    {
        if (current >= peak)
        {
            peak = current;
            holdFrames = 28;
        }
        else if (holdFrames > 0)
        {
            --holdFrames;
        }
        else
        {
            peak = std::max (current, peak - 0.015f);
        }
    }

    void drawMeter (juce::Graphics& g, juce::Rectangle<float> area, float level, float peakLevel,
                    const juce::String& label, juce::Colour bottomCol, juce::Colour topCol, bool topDown)
    {
        auto labelArea = area.removeFromBottom (16.0f);
        auto barArea = area.reduced (5.0f, 2.0f);

        g.setColour (juce::Colour (0xff090b11));
        g.fillRoundedRectangle (barArea, 4.0f);

        g.setColour (juce::Colour (0xff1d2438));
        g.drawRoundedRectangle (barArea, 4.0f, 1.0f);

        float fillH = barArea.getHeight() * juce::jlimit (0.0f, 1.0f, level);
        if (fillH > 1.0f)
        {
            juce::Rectangle<float> fillRect = topDown
                ? barArea.withHeight (fillH)
                : barArea.withTop (barArea.getBottom() - fillH);

            juce::ColourGradient grad (topCol, barArea.getX(), barArea.getY(),
                                       bottomCol, barArea.getX(), barArea.getBottom(), false);
            g.setGradientFill (grad);
            g.fillRoundedRectangle (fillRect, 3.5f);
        }

        if (peakLevel > 0.02f)
        {
            float peakY = topDown
                ? barArea.getY() + barArea.getHeight() * peakLevel
                : barArea.getBottom() - barArea.getHeight() * peakLevel;

            g.setColour (topCol.brighter (0.4f));
            g.fillRoundedRectangle (barArea.getX() + 1.0f, peakY - 1.0f, barArea.getWidth() - 2.0f, 2.0f, 1.0f);
        }

        g.setColour (juce::Colour (0xff8a94ad));
        g.setFont (juce::FontOptions (10.0f, juce::Font::bold));
        g.drawFittedText (label, labelArea.toNearestInt(), juce::Justification::centred, 1);
    }

    void drawWaveformArea (juce::Graphics& g, juce::Rectangle<float> area)
    {
        g.saveState();
        g.reduceClipRegion (area.toNearestInt());

        float midY = area.getCentreY();
        float maxAmp = area.getHeight() * 0.42f;

        auto activeAccent = juce::Colour (0xff00e5ff).interpolatedWith (juce::Colour (0xff64748b), bypassBlend);
        auto activeSecondary = juce::Colour (0xff8b5cf6).interpolatedWith (juce::Colour (0xff475569), bypassBlend);

        juce::ColourGradient centerGlow (activeAccent.withAlpha (0.07f * (1.0f - bypassBlend * 0.7f)),
                                         area.getCentreX(), midY,
                                         juce::Colours::transparentBlack,
                                         area.getX(), midY, true);
        g.setGradientFill (centerGlow);
        g.fillRect (area);

        g.setColour (juce::Colour (0xff1f273c));
        g.drawHorizontalLine (static_cast<int> (midY), area.getX(), area.getRight());
        g.setColour (juce::Colour (0xff161c2c));
        g.drawHorizontalLine (static_cast<int> (midY - maxAmp * 0.5f), area.getX(), area.getRight());
        g.drawHorizontalLine (static_cast<int> (midY + maxAmp * 0.5f), area.getX(), area.getRight());

        for (int i = 1; i < 8; ++i)
        {
            float gx = area.getX() + area.getWidth() * (static_cast<float> (i) / 8.0f);
            g.setColour (juce::Colour (0xff151b2a));
            g.drawVerticalLine (static_cast<int> (gx), area.getY(), area.getBottom());
        }

        auto buildSmoothWave = [&] (const std::array<float, numPoints>& data, juce::Path& fillPath,
                                    juce::Path& topLine, juce::Path& bottomLine)
        {
            float stepX = area.getWidth() / static_cast<float> (numPoints - 1);

            auto sampleAmp = [&] (int idx) -> float
            {
                int i0 = std::max (0, idx - 1);
                int i2 = std::min (numPoints - 1, idx + 1);
                float avg = 0.25f * data[static_cast<size_t> (i0)]
                          + 0.50f * data[static_cast<size_t> (idx)]
                          + 0.25f * data[static_cast<size_t> (i2)];
                return std::pow (juce::jlimit (0.0f, 1.0f, avg), 0.70f) * maxAmp;
            };

            float prevX = area.getX();
            float prevY = midY - sampleAmp (0);
            fillPath.startNewSubPath (prevX, prevY);
            topLine.startNewSubPath (prevX, prevY);

            for (int i = 1; i < numPoints; ++i)
            {
                float x = area.getX() + static_cast<float> (i) * stepX;
                float y = midY - sampleAmp (i);
                float mx = (prevX + x) * 0.5f;
                float my = (prevY + y) * 0.5f;
                fillPath.quadraticTo (prevX, prevY, mx, my);
                topLine.quadraticTo (prevX, prevY, mx, my);
                prevX = x;
                prevY = y;
            }
            fillPath.lineTo (prevX, prevY);
            topLine.lineTo (prevX, prevY);

            prevX = area.getRight();
            prevY = midY + sampleAmp (numPoints - 1);
            fillPath.lineTo (prevX, prevY);
            bottomLine.startNewSubPath (prevX, prevY);

            for (int i = numPoints - 2; i >= 0; --i)
            {
                float x = area.getX() + static_cast<float> (i) * stepX;
                float y = midY + sampleAmp (i);
                float mx = (prevX + x) * 0.5f;
                float my = (prevY + y) * 0.5f;
                fillPath.quadraticTo (prevX, prevY, mx, my);
                bottomLine.quadraticTo (prevX, prevY, mx, my);
                prevX = x;
                prevY = y;
            }
            fillPath.lineTo (prevX, prevY);
            bottomLine.lineTo (prevX, prevY);
            fillPath.closeSubPath();
        };

        juce::Path inFill, inTop, inBot;
        buildSmoothWave (inputWave, inFill, inTop, inBot);

        g.setColour (juce::Colour (0xff64748b).withAlpha (0.16f));
        g.fillPath (inFill);
        g.setColour (juce::Colour (0xff94a3b8).withAlpha (0.30f));
        g.strokePath (inTop, juce::PathStrokeType (1.0f));
        g.strokePath (inBot, juce::PathStrokeType (1.0f));

        juce::Path outFill, outTop, outBot;
        buildSmoothWave (outputWave, outFill, outTop, outBot);

        juce::ColourGradient fillGrad (activeAccent.withAlpha (0.42f), area.getX(), area.getY(),
                                       activeSecondary.withAlpha (0.28f), area.getRight(), area.getBottom(), false);
        g.setGradientFill (fillGrad);
        g.fillPath (outFill);

        g.setColour (activeAccent.withAlpha (0.16f));
        g.strokePath (outTop, juce::PathStrokeType (5.5f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
        g.strokePath (outBot, juce::PathStrokeType (5.5f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

        g.setColour (activeAccent);
        g.strokePath (outTop, juce::PathStrokeType (1.6f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
        g.strokePath (outBot, juce::PathStrokeType (1.6f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

        g.setFont (juce::FontOptions (10.5f, juce::Font::bold));
        g.setColour (juce::Colour (0xff94a3b8));
        g.fillEllipse (area.getX() + 6.0f, area.getY() + 5.0f, 6.5f, 6.5f);
        g.drawText ("RAW", static_cast<int> (area.getX() + 17.0f), static_cast<int> (area.getY() + 1.0f), 40, 15, juce::Justification::centredLeft);

        g.setColour (activeAccent);
        g.fillEllipse (area.getX() + 56.0f, area.getY() + 5.0f, 6.5f, 6.5f);
        g.drawText (bypassed ? "BYPASSED" : "PROCESSED", static_cast<int> (area.getX() + 67.0f), static_cast<int> (area.getY() + 1.0f), 90, 15, juce::Justification::centredLeft);

        juce::String grText = "GR -" + juce::String (smoothGrDb, 1) + " dB";
        g.setColour (smoothGrDb > 0.2f ? juce::Colour (0xffff9f43) : juce::Colour (0xff64748b));
        g.drawText (grText, static_cast<int> (area.getRight() - 96.0f), static_cast<int> (area.getY() + 1.0f), 90, 15, juce::Justification::centredRight);

        g.restoreState();
    }

    std::array<float, numPoints> inputWave {};
    std::array<float, numPoints> outputWave {};
    float smoothIn { 0.0f };
    float smoothOut { 0.0f };
    float smoothGr { 0.0f };
    float smoothGrDb { 0.0f };
    float peakIn { 0.0f };
    float peakOut { 0.0f };
    float peakGr { 0.0f };
    int holdIn { 0 };
    int holdOut { 0 };
    int holdGr { 0 };
    float bypassBlend { 0.0f };
    bool bypassed { false };
};