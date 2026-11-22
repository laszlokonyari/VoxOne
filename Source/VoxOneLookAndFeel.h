#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <algorithm>
#include <cmath>

class AnimatedVoxKnob : public juce::Slider
{
public:
    void setBipolar (bool shouldBeBipolar)
    {
        bipolar = shouldBeBipolar;
    }

    bool isBipolar() const
    {
        return bipolar;
    }

    float getHoverAmount() const
    {
        return hoverAmount;
    }

    float getDragAmount() const
    {
        return dragAmount;
    }

    float getPulsePhase() const
    {
        return pulsePhase;
    }

    bool tickAnimation()
    {
        float targetHover = isMouseOverOrDragging() ? 1.0f : 0.0f;
        float targetDrag = isMouseButtonDown() ? 1.0f : 0.0f;

        float prevHover = hoverAmount;
        float prevDrag = dragAmount;

        hoverAmount += (targetHover - hoverAmount) * 0.22f;
        dragAmount += (targetDrag - dragAmount) * 0.28f;

        if (std::abs (targetHover - hoverAmount) < 0.001f)
            hoverAmount = targetHover;

        if (std::abs (targetDrag - dragAmount) < 0.001f)
            dragAmount = targetDrag;

        bool activePulse = dragAmount > 0.01f;
        if (activePulse)
        {
            pulsePhase += 0.085f;
            if (pulsePhase > juce::MathConstants<float>::twoPi)
                pulsePhase -= juce::MathConstants<float>::twoPi;
        }
        else
        {
            pulsePhase = 0.0f;
        }

        bool changed = (std::abs (hoverAmount - prevHover) > 0.0005f)
                    || (std::abs (dragAmount - prevDrag) > 0.0005f)
                    || activePulse;

        if (changed)
            repaint();

        return changed;
    }

private:
    bool bipolar { false };
    float hoverAmount { 0.0f };
    float dragAmount { 0.0f };
    float pulsePhase { 0.0f };
};

class VoxOneLookAndFeel : public juce::LookAndFeel_V4
{
public:
    VoxOneLookAndFeel()
    {
        setColour (juce::PopupMenu::backgroundColourId, juce::Colour (0xff141824));
        setColour (juce::PopupMenu::textColourId, juce::Colour (0xffe8ecf7));
        setColour (juce::PopupMenu::highlightedBackgroundColourId, juce::Colour (0xff00e5ff).withAlpha (0.20f));
        setColour (juce::PopupMenu::highlightedTextColourId, juce::Colour (0xff00e5ff));
    }

    void drawRotarySlider (juce::Graphics& g, int x, int y, int width, int height,
                           float sliderPosProportional, float rotaryStartAngle,
                           float rotaryEndAngle, juce::Slider& slider) override
    {
        auto fullBounds = juce::Rectangle<float> (static_cast<float> (x), static_cast<float> (y),
                                                  static_cast<float> (width), static_cast<float> (height));
        float minDim = std::min (fullBounds.getWidth(), fullBounds.getHeight());
        float pad = std::clamp (minDim * 0.11f, 6.0f, 11.0f);
        auto bounds = fullBounds.reduced (pad);

        float radius = std::min (bounds.getWidth(), bounds.getHeight()) * 0.5f;
        float centreX = bounds.getCentreX();
        float centreY = bounds.getCentreY();
        float tickRadius = radius;
        float arcRadius = radius - 5.5f;
        float bodyRadius = std::max (10.0f, arcRadius - 7.5f);

        float angle = rotaryStartAngle + sliderPosProportional * (rotaryEndAngle - rotaryStartAngle);
        float midAngle = (rotaryStartAngle + rotaryEndAngle) * 0.5f;

        float hoverAmt = 0.0f;
        float dragAmt = 0.0f;
        float pulsePhase = 0.0f;
        bool isBipolar = false;

        if (auto* animKnob = dynamic_cast<AnimatedVoxKnob*> (&slider))
        {
            hoverAmt = animKnob->getHoverAmount();
            dragAmt = animKnob->getDragAmount();
            pulsePhase = animKnob->getPulsePhase();
            isBipolar = animKnob->isBipolar();
        }

        float glowLevel = hoverAmt * 0.35f + dragAmt * 0.65f;
        float pulse = 1.0f + 0.10f * std::sin (pulsePhase) * dragAmt;

        auto accent = slider.findColour (juce::Slider::rotarySliderFillColourId);
        if (! slider.isEnabled())
            accent = accent.withAlpha (0.35f);

        float arcStart = isBipolar ? midAngle : rotaryStartAngle;
        float arcEnd = angle;
        bool hasColoredArc = std::abs (arcEnd - arcStart) > 0.005f;
        float fromA = std::min (arcStart, arcEnd);
        float toA = std::max (arcStart, arcEnd);

        if (glowLevel > 0.01f && hasColoredArc)
        {
            float haloRadius = arcRadius + pad * 0.95f;
            juce::ColourGradient haloGrad (accent.withAlpha (0.17f * glowLevel * pulse), centreX, centreY,
                                           accent.withAlpha (0.0f), centreX + haloRadius, centreY, true);
            g.setGradientFill (haloGrad);
            g.fillEllipse (centreX - haloRadius, centreY - haloRadius, haloRadius * 2.0f, haloRadius * 2.0f);
        }

        constexpr int numTicks = 15;
        for (int i = 0; i < numTicks; ++i)
        {
            float t = static_cast<float> (i) / static_cast<float> (numTicks - 1);
            float tickAngle = rotaryStartAngle + t * (rotaryEndAngle - rotaryStartAngle);
            float tx = centreX + std::sin (tickAngle) * tickRadius;
            float ty = centreY - std::cos (tickAngle) * tickRadius;

            bool lit = hasColoredArc && (tickAngle >= fromA - 0.02f) && (tickAngle <= toA + 0.02f);
            float dotSize = (i == 0 || i == numTicks - 1 || (isBipolar && i == numTicks / 2)) ? 2.4f : 1.7f;

            if (lit)
                g.setColour (accent.withAlpha (0.55f + 0.45f * glowLevel));
            else
                g.setColour (juce::Colour (0xff2b334a).withAlpha (0.75f));

            g.fillEllipse (tx - dotSize * 0.5f, ty - dotSize * 0.5f, dotSize, dotSize);
        }

        juce::Path trackArc;
        trackArc.addCentredArc (centreX, centreY, arcRadius, arcRadius, 0.0f,
                                rotaryStartAngle, rotaryEndAngle, true);
        g.setColour (juce::Colour (0xff1b2030));
        g.strokePath (trackArc, juce::PathStrokeType (4.2f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
        g.setColour (juce::Colour (0xff2a3248).withAlpha (0.7f));
        g.strokePath (trackArc, juce::PathStrokeType (1.2f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

        if (hasColoredArc)
        {
            juce::Path valueArc;
            valueArc.addCentredArc (centreX, centreY, arcRadius, arcRadius, 0.0f, fromA, toA, true);

            if (glowLevel > 0.005f)
            {
                g.setColour (accent.withAlpha (0.14f * glowLevel * pulse));
                g.strokePath (valueArc, juce::PathStrokeType (12.0f + 4.5f * dragAmt, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

                g.setColour (accent.withAlpha (0.24f * glowLevel * pulse));
                g.strokePath (valueArc, juce::PathStrokeType (7.5f + 2.5f * dragAmt, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
            }

            g.setColour (accent.withAlpha (0.20f));
            g.strokePath (valueArc, juce::PathStrokeType (6.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

            auto coreCol = accent.interpolatedWith (juce::Colours::white, 0.22f * dragAmt);
            g.setColour (coreCol);
            g.strokePath (valueArc, juce::PathStrokeType (3.5f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
        }

        auto bodyBounds = juce::Rectangle<float> (centreX - bodyRadius, centreY - bodyRadius,
                                                  bodyRadius * 2.0f, bodyRadius * 2.0f);

        g.setColour (juce::Colours::black.withAlpha (0.55f));
        g.fillEllipse (bodyBounds.translated (0.0f, 3.0f).expanded (0.5f));

        juce::ColourGradient outerBezel (juce::Colour (0xff2e3750), centreX, bodyBounds.getY(),
                                         juce::Colour (0xff0e111a), centreX, bodyBounds.getBottom(), false);
        g.setGradientFill (outerBezel);
        g.fillEllipse (bodyBounds);

        auto innerBounds = bodyBounds.reduced (2.2f);
        juce::ColourGradient capGrad (juce::Colour (0xff22293d), centreX - bodyRadius * 0.25f, innerBounds.getY(),
                                      juce::Colour (0xff10131d), centreX + bodyRadius * 0.25f, innerBounds.getBottom(), false);
        g.setGradientFill (capGrad);
        g.fillEllipse (innerBounds);

        auto rimCol = juce::Colour (0xff3c4766).interpolatedWith (accent, 0.45f * glowLevel);
        g.setColour (rimCol.withAlpha (0.75f));
        g.drawEllipse (bodyBounds.reduced (0.5f), 1.1f);

        juce::Path topSheen;
        topSheen.addCentredArc (centreX, centreY, innerBounds.getWidth() * 0.5f - 0.8f,
                                innerBounds.getHeight() * 0.5f - 0.8f, 0.0f, -0.85f, 0.85f, true);
        g.setColour (juce::Colours::white.withAlpha (0.12f + 0.08f * hoverAmt));
        g.strokePath (topSheen, juce::PathStrokeType (1.0f));

        float pointerLen = bodyRadius * 0.52f;
        float pointerY = -bodyRadius + 4.5f;

        if (glowLevel > 0.01f)
        {
            juce::Path pointerGlow;
            pointerGlow.addRoundedRectangle (-3.2f, pointerY - 1.5f, 6.4f, pointerLen + 3.0f, 3.2f);
            pointerGlow.applyTransform (juce::AffineTransform::rotation (angle).translated (centreX, centreY));
            g.setColour (accent.withAlpha (0.32f * glowLevel * pulse));
            g.fillPath (pointerGlow);
        }

        juce::Path pointer;
        pointer.addRoundedRectangle (-1.5f, pointerY, 3.0f, pointerLen, 1.5f);
        pointer.applyTransform (juce::AffineTransform::rotation (angle).translated (centreX, centreY));

        g.setColour (accent.interpolatedWith (juce::Colours::white, 0.30f * dragAmt));
        g.fillPath (pointer);
    }

    void drawLabel (juce::Graphics& g, juce::Label& label) override
    {
        if (auto* animKnob = dynamic_cast<AnimatedVoxKnob*> (label.getParentComponent()))
        {
            auto bounds = label.getLocalBounds().toFloat().reduced (1.0f, 1.0f);
            float hoverAmt = animKnob->getHoverAmount();
            float dragAmt = animKnob->getDragAmount();
            float glowLevel = hoverAmt * 0.35f + dragAmt * 0.65f;
            auto accent = animKnob->findColour (juce::Slider::rotarySliderFillColourId);

            auto fillCol = juce::Colour (0xff0e111a).interpolatedWith (accent, 0.09f * dragAmt);
            g.setColour (fillCol);
            g.fillRoundedRectangle (bounds, 6.0f);

            auto borderCol = juce::Colour (0xff232a3e).interpolatedWith (accent, 0.70f * glowLevel);
            g.setColour (borderCol);
            g.drawRoundedRectangle (bounds, 6.0f, 1.0f + 0.35f * dragAmt);

            if (! label.isBeingEdited())
            {
                auto textCol = juce::Colour (0xffdce3f2).interpolatedWith (juce::Colours::white, 0.5f * dragAmt);
                g.setColour (textCol);
                g.setFont (label.getFont());
                g.drawFittedText (label.getText(), label.getLocalBounds().reduced (4, 0),
                                  label.getJustificationType(), 1);
            }
            return;
        }

        LookAndFeel_V4::drawLabel (g, label);
    }

    void drawComboBox (juce::Graphics& g, int width, int height, bool isButtonDown,
                       int buttonX, int buttonY, int buttonW, int buttonH,
                       juce::ComboBox& box) override
    {
        juce::ignoreUnused (isButtonDown, buttonX, buttonY, buttonW, buttonH);
        auto bounds = juce::Rectangle<float> (0.0f, 0.0f, static_cast<float> (width), static_cast<float> (height)).reduced (0.5f);

        juce::ColourGradient grad (juce::Colour (0xff1a2030), 0.0f, 0.0f,
                                   juce::Colour (0xff121622), 0.0f, static_cast<float> (height), false);
        g.setGradientFill (grad);
        g.fillRoundedRectangle (bounds, 8.0f);

        bool active = box.hasKeyboardFocus (true) || box.isPopupActive() || box.isMouseOver();
        g.setColour (active ? juce::Colour (0xff00e5ff).withAlpha (0.75f) : juce::Colour (0xff2b334a));
        g.drawRoundedRectangle (bounds, 8.0f, 1.2f);

        auto arrowZone = juce::Rectangle<float> (static_cast<float> (width - 26), 0.0f, 20.0f, static_cast<float> (height));
        juce::Path arrow;
        float cx = arrowZone.getCentreX();
        float cy = arrowZone.getCentreY();
        arrow.startNewSubPath (cx - 4.5f, cy - 2.0f);
        arrow.lineTo (cx, cy + 3.0f);
        arrow.lineTo (cx + 4.5f, cy - 2.0f);

        g.setColour (juce::Colour (0xff00e5ff));
        g.strokePath (arrow, juce::PathStrokeType (1.8f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    }

    void drawButtonBackground (juce::Graphics& g, juce::Button& button,
                               const juce::Colour& backgroundColour,
                               bool shouldDrawButtonAsHighlighted,
                               bool shouldDrawButtonAsDown) override
    {
        auto bounds = button.getLocalBounds().toFloat().reduced (0.5f);
        auto base = backgroundColour;

        if (shouldDrawButtonAsDown)
            base = base.brighter (0.18f);
        else if (shouldDrawButtonAsHighlighted)
            base = base.brighter (0.10f);

        bool toggled = button.getToggleState();
        bool isToggleBtn = button.getClickingTogglesState();
        auto glowCol = toggled ? juce::Colour (0xffff4d6d) : juce::Colour (0xff00e5ff);

        if (isToggleBtn || shouldDrawButtonAsHighlighted)
        {
            g.setColour (glowCol.withAlpha (toggled ? 0.16f : (shouldDrawButtonAsHighlighted ? 0.14f : 0.08f)));
            g.drawRoundedRectangle (bounds.expanded (1.0f), 9.0f, 2.5f);
        }

        juce::ColourGradient btnGrad (base.brighter (0.06f), bounds.getX(), bounds.getY(),
                                      base.darker (0.12f), bounds.getX(), bounds.getBottom(), false);
        g.setGradientFill (btnGrad);
        g.fillRoundedRectangle (bounds, 8.0f);

        auto borderCol = toggled
            ? juce::Colour (0xffff4d6d)
            : (shouldDrawButtonAsHighlighted ? juce::Colour (0xff00e5ff).withAlpha (0.8f) : juce::Colour (0xff2d364f));

        g.setColour (borderCol);
        g.drawRoundedRectangle (bounds, 8.0f, 1.2f);
    }

    juce::Label* createSliderTextBox (juce::Slider& slider) override
    {
        auto* l = LookAndFeel_V4::createSliderTextBox (slider);
        l->setFont (juce::FontOptions (12.0f, juce::Font::bold));
        l->setColour (juce::Label::backgroundColourId, juce::Colours::transparentBlack);
        l->setColour (juce::Label::outlineColourId, juce::Colours::transparentBlack);
        l->setColour (juce::Label::textColourId, juce::Colour (0xffe8ecf7));
        return l;
    }
};