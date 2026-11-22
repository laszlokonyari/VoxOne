#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <cmath>

class VoxOneLookAndFeel : public juce::LookAndFeel_V4
{
public:
    VoxOneLookAndFeel()
    {
        setColour (juce::PopupMenu::backgroundColourId, juce::Colour (0xff161a26));
        setColour (juce::PopupMenu::textColourId, juce::Colour (0xffe6eaf4));
        setColour (juce::PopupMenu::highlightedBackgroundColourId, juce::Colour (0xff00e5ff).withAlpha (0.22f));
        setColour (juce::PopupMenu::highlightedTextColourId, juce::Colour (0xff00e5ff));
    }

    void drawRotarySlider (juce::Graphics& g, int x, int y, int width, int height,
                           float sliderPosProportional, float rotaryStartAngle,
                           float rotaryEndAngle, juce::Slider& slider) override
    {
        auto bounds = juce::Rectangle<float> (static_cast<float> (x), static_cast<float> (y),
                                              static_cast<float> (width), static_cast<float> (height)).reduced (6.0f);
        float radius = std::min (bounds.getWidth(), bounds.getHeight()) * 0.5f;
        float centreX = bounds.getCentreX();
        float centreY = bounds.getCentreY();
        float arcRadius = radius - 3.0f;
        float angle = rotaryStartAngle + sliderPosProportional * (rotaryEndAngle - rotaryStartAngle);

        auto accent = slider.findColour (juce::Slider::rotarySliderFillColourId);
        if (! slider.isEnabled())
            accent = accent.withAlpha (0.35f);

        juce::Path trackArc;
        trackArc.addCentredArc (centreX, centreY, arcRadius, arcRadius, 0.0f,
                                rotaryStartAngle, rotaryEndAngle, true);
        g.setColour (juce::Colour (0xff23293b));
        g.strokePath (trackArc, juce::PathStrokeType (4.2f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

        if (sliderPosProportional > 0.001f)
        {
            juce::Path valueArc;
            valueArc.addCentredArc (centreX, centreY, arcRadius, arcRadius, 0.0f,
                                    rotaryStartAngle, angle, true);

            g.setColour (accent.withAlpha (0.24f));
            g.strokePath (valueArc, juce::PathStrokeType (8.5f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

            g.setColour (accent);
            g.strokePath (valueArc, juce::PathStrokeType (4.2f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
        }

        float bodyRadius = arcRadius - 7.5f;
        auto bodyBounds = juce::Rectangle<float> (centreX - bodyRadius, centreY - bodyRadius,
                                                  bodyRadius * 2.0f, bodyRadius * 2.0f);

        g.setColour (juce::Colours::black.withAlpha (0.45f));
        g.fillEllipse (bodyBounds.translated (0.0f, 3.0f));

        juce::ColourGradient knobGrad (juce::Colour (0xff262d42), centreX, bodyBounds.getY(),
                                       juce::Colour (0xff121520), centreX, bodyBounds.getBottom(), false);
        g.setGradientFill (knobGrad);
        g.fillEllipse (bodyBounds);

        g.setColour (juce::Colour (0xff343d57));
        g.drawEllipse (bodyBounds, 1.2f);

        juce::Path pointer;
        float pointerLen = bodyRadius * 0.55f;
        pointer.addRoundedRectangle (-1.6f, -bodyRadius + 4.5f, 3.2f, pointerLen, 1.6f);
        pointer.applyTransform (juce::AffineTransform::rotation (angle).translated (centreX, centreY));

        g.setColour (accent);
        g.fillPath (pointer);
    }

    void drawComboBox (juce::Graphics& g, int width, int height, bool isButtonDown,
                       int buttonX, int buttonY, int buttonW, int buttonH,
                       juce::ComboBox& box) override
    {
        juce::ignoreUnused (isButtonDown, buttonX, buttonY, buttonW, buttonH);
        auto bounds = juce::Rectangle<float> (0.0f, 0.0f, static_cast<float> (width), static_cast<float> (height)).reduced (0.5f);

        g.setColour (juce::Colour (0xff171c29));
        g.fillRoundedRectangle (bounds, 8.0f);

        g.setColour (box.hasKeyboardFocus (true) ? juce::Colour (0xff00e5ff) : juce::Colour (0xff2b334a));
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
            base = base.brighter (0.08f);

        g.setColour (base);
        g.fillRoundedRectangle (bounds, 8.0f);

        auto borderCol = button.getToggleState()
            ? juce::Colour (0xffff4d6d)
            : juce::Colour (0xff2d364f);

        g.setColour (borderCol);
        g.drawRoundedRectangle (bounds, 8.0f, 1.2f);
    }

    juce::Label* createSliderTextBox (juce::Slider& slider) override
    {
        auto* l = LookAndFeel_V4::createSliderTextBox (slider);
        l->setFont (juce::FontOptions (12.5f, juce::Font::bold));
        l->setColour (juce::Label::backgroundColourId, juce::Colour (0xff12151f));
        l->setColour (juce::Label::outlineColourId, juce::Colour (0xff22283a));
        l->setColour (juce::Label::textColourId, juce::Colour (0xffe8ecf7));
        return l;
    }
};