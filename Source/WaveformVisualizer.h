#pragma once

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

        float inDb = juce::Decibels::gainToDecibels (inLevelLin, -60.0f);
        float outDb = juce::Decibels::gainToDecibels (outLevelLin, -60.0f);

        float normIn = juce::jlimit (0.0f, 1.0f, (inDb + 60.0f) / 60.0f);
        float normOut = juce::jlimit (0.0f, 1.0f, (outDb + 60.0f) / 60.0f);
        float normGr = juce::jlimit (0.0f, 1.0f, gainReductionDb / 18.0f);

        smoothIn = (normIn > smoothIn) ? normIn : (smoothIn * 0.86f + normIn * 0.14f);
        smoothOut = (normOut > smoothOut) ? normOut : (smoothOut * 0.86f + normOut * 0.14f);
        smoothGr = (normGr > smoothGr) ? normGr : (smoothGr * 0.84f + normGr * 0.16f);
        smoothGrDb = (gainReductionDb > smoothGrDb) ? gainReductionDb : (smoothGrDb * 0.84f + gainReductionDb * 0.16f);

        repaint();
    }

    void paint (juce::Graphics& g) override
    {
        auto bounds = getLocalBounds().toFloat();

        juce::ColourGradient bgGrad (juce::Colour (0xff0e111a), bounds.getX(), bounds.getY(),
                                     juce::Colour (0xff141926), bounds.getX(), bounds.getBottom(), false);
        g.setGradientFill (bgGrad);
        g.fillRoundedRectangle (bounds, 12.0f);

        g.setColour (juce::Colour (0xff232a3e));
        g.drawRoundedRectangle (bounds.reduced (0.5f), 12.0f, 1.2f);

        auto content = bounds.reduced (12.0f, 10.0f);
        auto leftMeterArea = content.removeFromLeft (24.0f);
        content.removeFromLeft (10.0f);
        auto rightMetersArea = content.removeFromRight (52.0f);
        content.removeFromRight (10.0f);

        drawMeter (g, leftMeterArea, smoothIn, "IN", juce::Colour (0xff64748b), juce::Colour (0xff00e5ff), false);

        auto grMeterArea = rightMetersArea.removeFromLeft (22.0f);
        rightMetersArea.removeFromLeft (8.0f);
        auto outMeterArea = rightMetersArea;

        drawMeter (g, grMeterArea, smoothGr, "GR", juce::Colour (0xffff9f43), juce::Colour (0xffff3d71), true);
        drawMeter (g, outMeterArea, smoothOut, "OUT", juce::Colour (0xff7c4dff), juce::Colour (0xff00e5ff), false);

        drawWaveformArea (g, content);
    }

private:
    void drawMeter (juce::Graphics& g, juce::Rectangle<float> area, float level,
                    const juce::String& label, juce::Colour bottomCol, juce::Colour topCol, bool topDown)
    {
        auto labelArea = area.removeFromBottom (16.0f);
        auto barArea = area.reduced (4.0f, 2.0f);

        g.setColour (juce::Colour (0xff0a0c12));
        g.fillRoundedRectangle (barArea, 4.0f);

        g.setColour (juce::Colour (0xff1f2638));
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

        g.setColour (juce::Colour (0xff8a94ad));
        g.setFont (juce::FontOptions (10.5f, juce::Font::bold));
        g.drawFittedText (label, labelArea.toNearestInt(), juce::Justification::centred, 1);
    }

    void drawWaveformArea (juce::Graphics& g, juce::Rectangle<float> area)
    {
        g.saveState();
        g.reduceClipRegion (area.toNearestInt());

        float midY = area.getCentreY();
        float maxAmp = area.getHeight() * 0.44f;

        g.setColour (juce::Colour (0xff1d2334));
        g.drawHorizontalLine (static_cast<int> (midY), area.getX(), area.getRight());
        g.drawHorizontalLine (static_cast<int> (midY - maxAmp * 0.5f), area.getX(), area.getRight());
        g.drawHorizontalLine (static_cast<int> (midY + maxAmp * 0.5f), area.getX(), area.getRight());

        for (int i = 1; i < 8; ++i)
        {
            float gx = area.getX() + area.getWidth() * (static_cast<float> (i) / 8.0f);
            g.setColour (juce::Colour (0xff171c2b));
            g.drawVerticalLine (static_cast<int> (gx), area.getY(), area.getBottom());
        }

        auto buildWavePath = [&] (const std::array<float, numPoints>& data, juce::Path& fillPath,
                                  juce::Path& topLine, juce::Path& bottomLine)
        {
            float stepX = area.getWidth() / static_cast<float> (numPoints - 1);

            for (int i = 0; i < numPoints; ++i)
            {
                float x = area.getX() + static_cast<float> (i) * stepX;
                float amp = std::pow (juce::jlimit (0.0f, 1.0f, data[static_cast<size_t> (i)]), 0.72f) * maxAmp;
                float yTop = midY - amp;

                if (i == 0)
                {
                    fillPath.startNewSubPath (x, yTop);
                    topLine.startNewSubPath (x, yTop);
                }
                else
                {
                    fillPath.lineTo (x, yTop);
                    topLine.lineTo (x, yTop);
                }
            }

            for (int i = numPoints - 1; i >= 0; --i)
            {
                float x = area.getX() + static_cast<float> (i) * stepX;
                float amp = std::pow (juce::jlimit (0.0f, 1.0f, data[static_cast<size_t> (i)]), 0.72f) * maxAmp;
                float yBot = midY + amp;
                fillPath.lineTo (x, yBot);

                if (i == numPoints - 1)
                    bottomLine.startNewSubPath (x, yBot);
                else
                    bottomLine.lineTo (x, yBot);
            }

            fillPath.closeSubPath();
        };

        juce::Path inFill, inTop, inBot;
        buildWavePath (inputWave, inFill, inTop, inBot);

        g.setColour (juce::Colour (0xff64748b).withAlpha (0.20f));
        g.fillPath (inFill);
        g.setColour (juce::Colour (0xff94a3b8).withAlpha (0.35f));
        g.strokePath (inTop, juce::PathStrokeType (1.0f));
        g.strokePath (inBot, juce::PathStrokeType (1.0f));

        juce::Path outFill, outTop, outBot;
        buildWavePath (outputWave, outFill, outTop, outBot);

        auto waveAccent = bypassed ? juce::Colour (0xff64748b) : juce::Colour (0xff00e5ff);
        auto waveSecondary = bypassed ? juce::Colour (0xff475569) : juce::Colour (0xff7c4dff);

        juce::ColourGradient fillGrad (waveAccent.withAlpha (0.42f), area.getX(), area.getY(),
                                       waveSecondary.withAlpha (0.28f), area.getRight(), area.getBottom(), false);
        g.setGradientFill (fillGrad);
        g.fillPath (outFill);

        g.setColour (waveAccent.withAlpha (0.25f));
        g.strokePath (outTop, juce::PathStrokeType (4.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
        g.strokePath (outBot, juce::PathStrokeType (4.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

        g.setColour (waveAccent);
        g.strokePath (outTop, juce::PathStrokeType (1.6f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
        g.strokePath (outBot, juce::PathStrokeType (1.6f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

        g.setFont (juce::FontOptions (11.0f, juce::Font::bold));
        g.setColour (juce::Colour (0xff94a3b8));
        g.fillEllipse (area.getX() + 6.0f, area.getY() + 5.0f, 7.0f, 7.0f);
        g.drawText ("RAW", static_cast<int> (area.getX() + 17.0f), static_cast<int> (area.getY() + 1.0f), 40, 16, juce::Justification::centredLeft);

        g.setColour (waveAccent);
        g.fillEllipse (area.getX() + 56.0f, area.getY() + 5.0f, 7.0f, 7.0f);
        g.drawText (bypassed ? "BYPASSED" : "VOXONE", static_cast<int> (area.getX() + 67.0f), static_cast<int> (area.getY() + 1.0f), 80, 16, juce::Justification::centredLeft);

        juce::String grText = "GR -" + juce::String (smoothGrDb, 1) + " dB";
        g.setColour (smoothGrDb > 0.2f ? juce::Colour (0xffff9f43) : juce::Colour (0xff64748b));
        g.drawText (grText, static_cast<int> (area.getRight() - 95.0f), static_cast<int> (area.getY() + 1.0f), 90, 16, juce::Justification::centredRight);

        g.restoreState();
    }

    std::array<float, numPoints> inputWave {};
    std::array<float, numPoints> outputWave {};
    float smoothIn { 0.0f };
    float smoothOut { 0.0f };
    float smoothGr { 0.0f };
    float smoothGrDb { 0.0f };
    bool bypassed { false };
};