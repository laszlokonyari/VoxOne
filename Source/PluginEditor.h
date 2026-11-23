#pragma once

#include "PluginProcessor.h"
#include "VoxOneLookAndFeel.h"
#include "WaveformVisualizer.h"
#include "SpaceVisualizer.h"
#include <array>
#include <functional>

class LogoScaleButton : public juce::Component
{
public:
    LogoScaleButton()
    {
        setMouseCursor (juce::MouseCursor::PointingHandCursor);
    }

    void setScalePercent (int percent)
    {
        if (scalePercent != percent)
        {
            scalePercent = percent;
            repaint();
        }
    }

    bool tickAnimation()
    {
        float targetHover = isMouseOverOrDragging() ? 1.0f : 0.0f;
        float prev = hoverAmount;
        hoverAmount += (targetHover - hoverAmount) * 0.22f;

        if (std::abs (targetHover - hoverAmount) < 0.001f)
            hoverAmount = targetHover;

        bool changed = std::abs (hoverAmount - prev) > 0.0005f;
        if (changed)
            repaint();

        return changed;
    }

    void mouseUp (const juce::MouseEvent& e) override
    {
        if (reallyContains (e.getPosition(), true) && onClick)
            onClick();
    }

    void paint (juce::Graphics& g) override
    {
        auto bounds = getLocalBounds().toFloat().reduced (1.0f);

        if (hoverAmount > 0.01f)
        {
            g.setColour (juce::Colour (0xff00e5ff).withAlpha (0.08f * hoverAmount));
            g.fillRoundedRectangle (bounds, 9.0f);
            g.setColour (juce::Colour (0xff00e5ff).withAlpha (0.45f * hoverAmount));
            g.drawRoundedRectangle (bounds, 9.0f, 1.0f);
        }

        auto titleCol = juce::Colour (0xff00e5ff).interpolatedWith (juce::Colours::white, 0.25f * hoverAmount);
        g.setColour (titleCol);
        g.setFont (juce::FontOptions (22.0f, juce::Font::bold));
        g.drawText ("VOXONE", 8, 2, 108, 25, juce::Justification::centredLeft);

        auto badge = juce::Rectangle<float> (116.0f, 6.0f, 58.0f, 18.0f);
        g.setColour (juce::Colour (0xff161d2e).interpolatedWith (juce::Colour (0xff00e5ff), 0.14f * hoverAmount));
        g.fillRoundedRectangle (badge, 5.0f);
        g.setColour (juce::Colour (0xff27344f).interpolatedWith (juce::Colour (0xff00e5ff), 0.65f * hoverAmount));
        g.drawRoundedRectangle (badge, 5.0f, 0.9f);

        g.setColour (juce::Colour (0xff9fb0ce).interpolatedWith (juce::Colour (0xff00e5ff), 0.75f * hoverAmount));
        g.setFont (juce::FontOptions (10.0f, juce::Font::bold));
        g.drawText (juce::String (scalePercent) + "%",
                    static_cast<int> (badge.getX() + 4.0f),
                    static_cast<int> (badge.getY()),
                    38, static_cast<int> (badge.getHeight()),
                    juce::Justification::centredLeft);

        float cx = badge.getRight() - 9.0f;
        float cy = badge.getCentreY();
        juce::Path chevron;
        chevron.startNewSubPath (cx - 3.2f, cy - 1.4f);
        chevron.lineTo (cx, cy + 2.0f);
        chevron.lineTo (cx + 3.2f, cy - 1.4f);
        g.strokePath (chevron, juce::PathStrokeType (1.4f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

        g.setColour (juce::Colour (0xff6c7a99).interpolatedWith (juce::Colour (0xffa8b8d8), 0.35f * hoverAmount));
        g.setFont (juce::FontOptions (9.6f, juce::Font::bold));
        g.drawText ("VOCAL FINISHING SUITE", 9, 27, 170, 14, juce::Justification::centredLeft);
    }

    std::function<void()> onClick;

private:
    int scalePercent { 100 };
    float hoverAmount { 0.0f };
};

class VoxOneAudioProcessorEditor : public juce::AudioProcessorEditor,
                                   private juce::Timer
{
public:
    static constexpr int designWidth = 1040;
    static constexpr int designHeight = 760;
    static constexpr int defaultWidth = 884;
    static constexpr int defaultHeight = 646;
    static constexpr float minUserScale = 0.60f;
    static constexpr float maxAbsoluteUserScale = 1.50f;

    explicit VoxOneAudioProcessorEditor (VoxOneAudioProcessor&);
    ~VoxOneAudioProcessorEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;
    void parentHierarchyChanged() override;

private:
    class ContentCanvas : public juce::Component
    {
    public:
        explicit ContentCanvas (VoxOneAudioProcessorEditor& ownerEditor) : owner (ownerEditor) {}
        void paint (juce::Graphics& g) override { owner.paintContent (g); }

    private:
        VoxOneAudioProcessorEditor& owner;
    };

    using SliderAttach = std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>;
    using ButtonAttach = std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment>;

    void timerCallback() override;
    void paintContent (juce::Graphics& g);
    void layoutContent();

    juce::Rectangle<int> getCurrentDisplayUserArea() const;
    float getMaxAllowedUserScale() const;
    void updateResizeLimits();
    void applyCanvasTransform();
    void applyUserScale (float requestedUserScale, bool persistScale);
    void showScaleMenu();

    void setupRotarySlider (AnimatedVoxKnob& slider, juce::Label& label,
                            const juce::String& text, const juce::String& suffix,
                            juce::Colour accentColour, double defaultVal,
                            bool bipolar = false, int textBoxWidth = 68);
    void setupHeaderControls();
    void setupSpaceControls();
    void drawModuleCard (juce::Graphics& g, juce::Rectangle<int> bounds,
                         const juce::String& title, juce::Colour accent, float activityGlow);
    void drawSignalChain (juce::Graphics& g, juce::Rectangle<int> area);

    VoxOneAudioProcessor& audioProcessor;
    VoxOneLookAndFeel customLookAndFeel;
    ContentCanvas contentCanvas;
    LogoScaleButton logoScaleButton;

    WaveformVisualizer waveformVisualizer;
    SpaceVisualizer spaceVisualizer;

    juce::ComboBox presetSelector;
    juce::TextButton prevPresetButton { "<" };
    juce::TextButton nextPresetButton { ">" };
    juce::TextButton bypassButton { "ACTIVE" };

    juce::TextButton spaceBypassButton { "SPACE ON" };
    std::array<juce::TextButton, 4> reverbModeButtons;
    juce::TextButton pingPongButton { "STEREO" };

    AnimatedVoxKnob inputGainSlider, hpfSlider, cleanSlider, compSlider, deEssSlider;
    AnimatedVoxKnob airSlider, clipSlider, outputGainSlider, ceilingSlider;

    AnimatedVoxKnob revMixSlider, revSizeSlider, revPreDelaySlider, revToneSlider, revWidthSlider, revDuckSlider;
    AnimatedVoxKnob delMixSlider, delTimeSlider, delFeedbackSlider, delToneSlider, delDuckSlider;

    juce::Label inputGainLabel, hpfLabel, cleanLabel, compLabel, deEssLabel;
    juce::Label airLabel, clipLabel, outputGainLabel, ceilingLabel;

    juce::Label revMixLabel, revSizeLabel, revPreDelayLabel, revToneLabel, revWidthLabel, revDuckLabel;
    juce::Label delMixLabel, delTimeLabel, delFeedbackLabel, delToneLabel, delDuckLabel, pingPongLabel;

    SliderAttach inputGainAttachment, hpfAttachment, cleanAttachment, compAttachment, deEssAttachment;
    SliderAttach airAttachment, clipAttachment, outputGainAttachment, ceilingAttachment;

    SliderAttach revMixAttachment, revSizeAttachment, revPreDelayAttachment, revToneAttachment, revWidthAttachment, revDuckAttachment;
    SliderAttach delMixAttachment, delTimeAttachment, delFeedbackAttachment, delToneAttachment, delDuckAttachment;

    ButtonAttach bypassAttachment, spaceBypassAttachment, pingPongAttachment;

    juce::Rectangle<int> toneCardBounds;
    juce::Rectangle<int> dynamicsCardBounds;
    juce::Rectangle<int> characterCardBounds;
    juce::Rectangle<int> spaceCardBounds;
    juce::Rectangle<int> reverbSubBounds;
    juce::Rectangle<int> delaySubBounds;
    juce::Rectangle<int> masterCardBounds;

    std::array<float, VoxOneAudioProcessor::waveformSize> inWaveCache {};
    std::array<float, VoxOneAudioProcessor::waveformSize> outWaveCache {};

    float toneCardGlow { 0.0f };
    float dynamicsCardGlow { 0.0f };
    float characterCardGlow { 0.0f };
    float spaceCardGlow { 0.0f };
    float masterCardGlow { 0.0f };
    int lastDisplayedPreset { -1 };

    float currentUserScale { 1.0f };
    juce::Rectangle<int> lastDisplayUserArea;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (VoxOneAudioProcessorEditor)
};