#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <algorithm>
#include <cmath>

struct SpaceVisualState
{
    int reverbMode { 1 };
    float reverbMix { 22.0f };
    float reverbSize { 45.0f };
    float reverbPreDelay { 24.0f };
    float reverbTone { 60.0f };
    float reverbWidth { 75.0f };
    float reverbDuck { 35.0f };
    float delayMix { 18.0f };
    float delayTime { 115.0f };
    float delayFeedback { 24.0f };
    float delayTone { 45.0f };
    float delayDuck { 40.0f };
    bool delayPingPong { false };
    float reverbEnergy { 0.0f };
    float delayEnergy { 0.0f };
    float duckActivity { 0.0f };
    bool spaceActive { true };
};

class SpaceVisualizer : public juce::Component
{
public:
    void setState (const SpaceVisualState& next)
    {
        target = next;

        smoothRevMix += (target.reverbMix * 0.01f - smoothRevMix) * 0.22f;
        smoothSize += (target.reverbSize * 0.01f - smoothSize) * 0.18f;
        smoothPreDelay += (target.reverbPreDelay / 120.0f - smoothPreDelay) * 0.20f;
        smoothRevTone += (target.reverbTone * 0.01f - smoothRevTone) * 0.20f;
        smoothWidth += (target.reverbWidth * 0.01f - smoothWidth) * 0.20f;
        smoothDelMix += (target.delayMix * 0.01f - smoothDelMix) * 0.22f;
        smoothDelTime += (target.delayTime - smoothDelTime) * 0.22f;
        smoothDelFb += (target.delayFeedback * 0.01f - smoothDelFb) * 0.22f;

        float activeTarget = target.spaceActive ? 1.0f : 0.0f;
        activeBlend += (activeTarget - activeBlend) * 0.16f;

        float revE = juce::jlimit (0.0f, 1.0f, std::pow (target.reverbEnergy * 2.2f, 0.65f));
        float delE = juce::jlimit (0.0f, 1.0f, std::pow (target.delayEnergy * 2.0f, 0.65f));

        smoothRevEnergy = (revE > smoothRevEnergy) ? revE : (smoothRevEnergy * 0.88f + revE * 0.12f);
        smoothDelEnergy = (delE > smoothDelEnergy) ? delE : (smoothDelEnergy * 0.86f + delE * 0.14f);
        smoothDuck = (target.duckActivity > smoothDuck) ? target.duckActivity : (smoothDuck * 0.86f + target.duckActivity * 0.14f);

        animPhase += 0.025f + 0.035f * smoothRevEnergy;
        if (animPhase > 1.0f)
            animPhase -= 1.0f;

        repaint();
    }

    void paint (juce::Graphics& g) override
    {
        auto bounds = getLocalBounds().toFloat();

        juce::ColourGradient bgGrad (juce::Colour (0xff090d16), bounds.getX(), bounds.getY(),
                                     juce::Colour (0xff111827), bounds.getX(), bounds.getBottom(), false);
        g.setGradientFill (bgGrad);
        g.fillRoundedRectangle (bounds, 12.0f);

        auto warmCol = juce::Colour (0xff2dd4bf);
        auto brightCol = juce::Colour (0xff00e5ff);
        auto tealAccent = warmCol.interpolatedWith (brightCol, smoothRevTone)
                                 .interpolatedWith (juce::Colour (0xff475569), 1.0f - activeBlend);
        auto delayAccent = juce::Colour (0xffe879f9)
                               .interpolatedWith (juce::Colour (0xff475569), 1.0f - activeBlend);

        g.setColour (juce::Colour (0xff1f2a40).interpolatedWith (tealAccent, 0.25f * activeBlend));
        g.drawRoundedRectangle (bounds.reduced (0.5f), 12.0f, 1.1f);

        auto content = bounds.reduced (10.0f, 8.0f);
        auto headerArea = content.removeFromTop (18.0f);
        auto timelineArea = content.removeFromBottom (46.0f);
        content.removeFromBottom (6.0f);
        auto chamberArea = content;

        drawHeader (g, headerArea, tealAccent);
        drawChamber (g, chamberArea, tealAccent, delayAccent);
        drawTimeline (g, timelineArea, tealAccent, delayAccent);
    }

private:
    void drawHeader (juce::Graphics& g, juce::Rectangle<float> area, juce::Colour accent)
    {
        static constexpr const char* modeNames[] = { "ROOM", "PLATE", "HALL", "AMBIENCE" };
        int idx = juce::jlimit (0, 3, target.reverbMode);

        g.setFont (juce::FontOptions (10.0f, juce::Font::bold));
        g.setColour (accent);
        g.fillEllipse (area.getX(), area.getCentreY() - 3.0f, 6.0f, 6.0f);
        g.drawText (juce::String ("SPACE: ") + modeNames[idx],
                    static_cast<int> (area.getX() + 10.0f),
                    static_cast<int> (area.getY()),
                    130, static_cast<int> (area.getHeight()),
                    juce::Justification::centredLeft);

        auto duckPill = area.removeFromRight (74.0f).reduced (0.0f, 1.0f);
        float duckGlow = smoothDuck * activeBlend;
        auto duckCol = juce::Colour (0xff475569).interpolatedWith (juce::Colour (0xffff9f43), juce::jlimit (0.0f, 1.0f, duckGlow * 1.6f));

        g.setColour (duckCol.withAlpha (0.14f + 0.18f * duckGlow));
        g.fillRoundedRectangle (duckPill, 5.0f);
        g.setColour (duckCol.withAlpha (0.65f));
        g.drawRoundedRectangle (duckPill, 5.0f, 0.9f);
        g.setColour (duckCol);
        g.drawFittedText (duckGlow > 0.05f ? "DUCKING" : "DUCK IDLE",
                          duckPill.toNearestInt(), juce::Justification::centred, 1);
    }

    void drawChamber (juce::Graphics& g, juce::Rectangle<float> area,
                      juce::Colour revCol, juce::Colour delCol)
    {
        g.saveState();
        g.reduceClipRegion (area.toNearestInt());

        auto outer = area.reduced (6.0f, 4.0f);
        float cx = outer.getCentreX();
        float cy = outer.getCentreY();

        float modeDepthScale = 1.0f;
        if (target.reverbMode == 0)
            modeDepthScale = 0.85f;
        else if (target.reverbMode == 2)
            modeDepthScale = 1.18f;
        else if (target.reverbMode == 3)
            modeDepthScale = 0.68f;

        float depthFactor = juce::jlimit (0.28f, 0.74f, (0.32f + 0.38f * smoothSize) * modeDepthScale);
        float widthFactor = 0.28f + 0.56f * smoothWidth;

        float innerW = outer.getWidth() * (1.0f - depthFactor * 0.65f) * (0.65f + 0.35f * widthFactor);
        float innerH = outer.getHeight() * (1.0f - depthFactor * 0.62f);
        auto inner = juce::Rectangle<float> (cx - innerW * 0.5f, cy - innerH * 0.5f, innerW, innerH);

        float auraR = std::max (outer.getWidth(), outer.getHeight()) * (0.28f + 0.38f * smoothSize + 0.18f * smoothRevEnergy);
        float auraAlpha = (0.08f + 0.32f * smoothRevMix * (0.35f + 0.65f * smoothRevEnergy)) * activeBlend;
        juce::ColourGradient aura (revCol.withAlpha (auraAlpha), cx, cy,
                                   revCol.withAlpha (0.0f), cx + auraR, cy, true);
        g.setGradientFill (aura);
        g.fillEllipse (cx - auraR, cy - auraR * 0.75f, auraR * 2.0f, auraR * 1.5f);

        g.setColour (revCol.withAlpha (0.20f + 0.18f * smoothRevMix));
        g.drawLine (outer.getX(), outer.getY(), inner.getX(), inner.getY(), 1.0f);
        g.drawLine (outer.getRight(), outer.getY(), inner.getRight(), inner.getY(), 1.0f);
        g.drawLine (outer.getX(), outer.getBottom(), inner.getX(), inner.getBottom(), 1.0f);
        g.drawLine (outer.getRight(), outer.getBottom(), inner.getRight(), inner.getBottom(), 1.0f);

        for (int step = 1; step <= 3; ++step)
        {
            float t = static_cast<float> (step) / 4.0f;
            float rx = juce::jmap (t, outer.getX(), inner.getX());
            float ry = juce::jmap (t, outer.getY(), inner.getY());
            float rw = juce::jmap (t, outer.getWidth(), inner.getWidth());
            float rh = juce::jmap (t, outer.getHeight(), inner.getHeight());
            g.setColour (revCol.withAlpha (0.10f + 0.10f * t));
            g.drawRoundedRectangle (rx, ry, rw, rh, 4.0f, 0.9f);
        }

        g.setColour (revCol.withAlpha (0.38f));
        g.drawRoundedRectangle (outer, 6.0f, 1.1f);
        g.setColour (revCol.withAlpha (0.48f + 0.25f * smoothRevEnergy));
        g.drawRoundedRectangle (inner, 4.0f, 1.2f);

        for (int r = 0; r < 3; ++r)
        {
            float p = std::fmod (animPhase + static_cast<float> (r) / 3.0f, 1.0f);
            float radX = (0.12f + p * (0.35f + 0.55f * smoothWidth)) * outer.getWidth() * 0.52f;
            float radY = (0.12f + p * (0.35f + 0.50f * smoothSize)) * outer.getHeight() * 0.48f;
            float alpha = (1.0f - p) * (0.12f + 0.52f * smoothRevMix * (0.4f + 0.6f * smoothRevEnergy)) * activeBlend;

            g.setColour (revCol.withAlpha (alpha));
            g.drawEllipse (cx - radX, cy - radY, radX * 2.0f, radY * 2.0f, 1.35f);
        }

        if (smoothDelMix > 0.01f)
        {
            float spreadX = outer.getWidth() * (target.delayPingPong ? 0.36f : 0.22f);
            float dotR = 3.5f + 4.5f * smoothDelEnergy * smoothDelMix;
            float alpha = (0.25f + 0.65f * smoothDelMix) * activeBlend;

            g.setColour (delCol.withAlpha (alpha * 0.35f));
            g.fillEllipse (cx - spreadX - dotR * 1.8f, cy - dotR * 1.8f, dotR * 3.6f, dotR * 3.6f);
            g.fillEllipse (cx + spreadX - dotR * 1.8f, cy - dotR * 1.8f, dotR * 3.6f, dotR * 3.6f);

            g.setColour (delCol.withAlpha (alpha));
            g.fillEllipse (cx - spreadX - dotR * 0.5f, cy - dotR * 0.5f, dotR, dotR);
            g.fillEllipse (cx + spreadX - dotR * 0.5f, cy - dotR * 0.5f, dotR, dotR);
        }

        float coreR = 5.0f + 2.5f * smoothRevEnergy;
        g.setColour (juce::Colours::white.withAlpha (0.85f * activeBlend + 0.25f * (1.0f - activeBlend)));
        g.fillEllipse (cx - coreR * 0.5f, cy - coreR * 0.5f, coreR, coreR);

        g.restoreState();
    }

    void drawTimeline (juce::Graphics& g, juce::Rectangle<float> area,
                       juce::Colour revCol, juce::Colour delCol)
    {
        g.setColour (juce::Colour (0xff070a11));
        g.fillRoundedRectangle (area, 7.0f);
        g.setColour (juce::Colour (0xff1c2539));
        g.drawRoundedRectangle (area, 7.0f, 0.9f);

        auto inner = area.reduced (8.0f, 5.0f);
        float baseY = target.delayPingPong ? inner.getCentreY() : inner.getBottom() - 2.0f;
        float maxH = target.delayPingPong ? (inner.getHeight() * 0.45f) : (inner.getHeight() - 4.0f);

        g.setColour (juce::Colour (0xff1f293d));
        g.drawHorizontalLine (static_cast<int> (baseY), inner.getX(), inner.getRight());

        float preX = inner.getX() + inner.getWidth() * (smoothPreDelay * 0.24f);

        if (smoothPreDelay > 0.02f && smoothRevMix > 0.01f)
        {
            g.setColour (revCol.withAlpha (0.28f * activeBlend));
            g.drawLine (inner.getX(), baseY, preX, baseY, 2.0f);
        }

        if (smoothRevMix > 0.01f)
        {
            juce::Path decayPath;
            decayPath.startNewSubPath (preX, baseY);
            float decaySpan = inner.getRight() - preX;
            float decayRate = 5.2f - 4.0f * smoothSize;
            float peakAmp = maxH * (0.25f + 0.72f * smoothRevMix) * (0.75f + 0.25f * smoothRevEnergy);

            constexpr int steps = 40;
            for (int i = 0; i <= steps; ++i)
            {
                float t = static_cast<float> (i) / static_cast<float> (steps);
                float x = preX + t * decaySpan;
                float envVal = std::exp (-t * decayRate);
                float y = baseY - envVal * peakAmp;
                decayPath.lineTo (x, y);
            }
            decayPath.lineTo (inner.getRight(), baseY);
            decayPath.closeSubPath();

            g.setColour (revCol.withAlpha (0.26f * activeBlend));
            g.fillPath (decayPath);
            g.setColour (revCol.withAlpha (0.72f * activeBlend));
            g.strokePath (decayPath, juce::PathStrokeType (1.2f));
        }

        if (smoothDelMix > 0.01f)
        {
            float windowMs = 600.0f;
            float tapGain = (0.35f + 0.65f * smoothDelMix) * (0.75f + 0.25f * smoothDelEnergy);

            for (int tap = 1; tap <= 8; ++tap)
            {
                float tMs = smoothDelTime * static_cast<float> (tap);
                if (tMs > windowMs)
                    break;

                float normX = tMs / windowMs;
                float x = inner.getX() + normX * inner.getWidth();
                float h = maxH * tapGain * std::pow (std::max (0.18f, smoothDelFb), static_cast<float> (tap - 1));
                if (h < 2.0f)
                    break;

                float yEnd = (target.delayPingPong && (tap % 2 == 0)) ? (baseY + h) : (baseY - h);

                g.setColour (delCol.withAlpha (0.25f * activeBlend));
                g.drawLine (x, baseY, x, yEnd, 4.2f);
                g.setColour (delCol.withAlpha (0.90f * activeBlend));
                g.drawLine (x, baseY, x, yEnd, 1.8f);
                g.fillEllipse (x - 2.0f, yEnd - 2.0f, 4.0f, 4.0f);
            }
        }
    }

    SpaceVisualState target;
    float smoothRevMix { 0.22f };
    float smoothSize { 0.45f };
    float smoothPreDelay { 0.20f };
    float smoothRevTone { 0.60f };
    float smoothWidth { 0.75f };
    float smoothDelMix { 0.18f };
    float smoothDelTime { 115.0f };
    float smoothDelFb { 0.24f };
    float smoothRevEnergy { 0.0f };
    float smoothDelEnergy { 0.0f };
    float smoothDuck { 0.0f };
    float activeBlend { 1.0f };
    float animPhase { 0.0f };
};