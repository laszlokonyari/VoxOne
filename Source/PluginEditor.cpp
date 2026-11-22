#include "PluginProcessor.h"
#include "PluginEditor.h"

VoxOneAudioProcessorEditor::VoxOneAudioProcessorEditor (VoxOneAudioProcessor& p)
    : AudioProcessorEditor (&p), audioProcessor (p)
{
    setLookAndFeel (&customLookAndFeel);

    setupHeaderControls();
    addAndMakeVisible (waveformVisualizer);

    auto cyanAccent = juce::Colour (0xff00e5ff);
    auto violetAccent = juce::Colour (0xff9b7bff);
    auto amberAccent = juce::Colour (0xffff9f43);
    auto silverAccent = juce::Colour (0xff64d2ff);

    setupRotarySlider (hpfSlider, hpfLabel, "HPF", " Hz", cyanAccent, 80.0);
    setupRotarySlider (cleanSlider, cleanLabel, "CLEAN EQ", " %", cyanAccent, 50.0);

    setupRotarySlider (compSlider, compLabel, "COMP", " %", violetAccent, 40.0);
    setupRotarySlider (deEssSlider, deEssLabel, "DE-ESS", " %", violetAccent, 35.0);

    setupRotarySlider (airSlider, airLabel, "FRESH AIR", " %", amberAccent, 30.0);
    setupRotarySlider (clipSlider, clipLabel, "SOFT CLIP", " %", amberAccent, 20.0);

    setupRotarySlider (inputGainSlider, inputGainLabel, "INPUT", " dB", silverAccent, 0.0, true);
    setupRotarySlider (outputGainSlider, outputGainLabel, "OUTPUT", " dB", silverAccent, 0.0, true);
    setupRotarySlider (ceilingSlider, ceilingLabel, "CEILING", " dB", silverAccent, -0.3);

    inputGainAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (
        audioProcessor.apvts, "inputGain", inputGainSlider);

    hpfAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (
        audioProcessor.apvts, "hpfFreq", hpfSlider);

    cleanAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (
        audioProcessor.apvts, "cleanAmount", cleanSlider);

    compAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (
        audioProcessor.apvts, "compAmount", compSlider);

    deEssAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (
        audioProcessor.apvts, "deEssAmount", deEssSlider);

    airAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (
        audioProcessor.apvts, "airAmount", airSlider);

    clipAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (
        audioProcessor.apvts, "clipAmount", clipSlider);

    outputGainAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (
        audioProcessor.apvts, "outputGain", outputGainSlider);

    ceilingAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (
        audioProcessor.apvts, "limiterCeiling", ceilingSlider);

    bypassAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (
        audioProcessor.apvts, "bypass", bypassButton);

    setSize (980, 600);
    startTimerHz (60);
}

VoxOneAudioProcessorEditor::~VoxOneAudioProcessorEditor()
{
    stopTimer();
    setLookAndFeel (nullptr);
}

void VoxOneAudioProcessorEditor::setupHeaderControls()
{
    const auto& presets = audioProcessor.getPresets();
    for (int i = 0; i < static_cast<int> (presets.size()); ++i)
        presetSelector.addItem (presets[static_cast<size_t> (i)].name, i + 1);

    lastDisplayedPreset = audioProcessor.getCurrentPresetIndex();
    presetSelector.setSelectedId (lastDisplayedPreset + 1, juce::dontSendNotification);
    presetSelector.setColour (juce::ComboBox::textColourId, juce::Colour (0xffe8ecf7));
    presetSelector.onChange = [this]()
    {
        int selectedIndex = presetSelector.getSelectedId() - 1;
        lastDisplayedPreset = selectedIndex;
        audioProcessor.loadPreset (selectedIndex);
    };
    addAndMakeVisible (presetSelector);

    auto styleNavButton = [this] (juce::TextButton& btn)
    {
        btn.setColour (juce::TextButton::buttonColourId, juce::Colour (0xff171c29));
        btn.setColour (juce::TextButton::textColourOffId, juce::Colour (0xff00e5ff));
        addAndMakeVisible (btn);
    };

    styleNavButton (prevPresetButton);
    styleNavButton (nextPresetButton);

    prevPresetButton.onClick = [this]()
    {
        int total = static_cast<int> (audioProcessor.getPresets().size());
        int nextIdx = (audioProcessor.getCurrentPresetIndex() - 1 + total) % total;
        presetSelector.setSelectedId (nextIdx + 1, juce::sendNotification);
    };

    nextPresetButton.onClick = [this]()
    {
        int total = static_cast<int> (audioProcessor.getPresets().size());
        int nextIdx = (audioProcessor.getCurrentPresetIndex() + 1) % total;
        presetSelector.setSelectedId (nextIdx + 1, juce::sendNotification);
    };

    bypassButton.setClickingTogglesState (true);
    bypassButton.setColour (juce::TextButton::buttonColourId, juce::Colour (0xff14252e));
    bypassButton.setColour (juce::TextButton::buttonOnColourId, juce::Colour (0xff3a1723));
    bypassButton.setColour (juce::TextButton::textColourOffId, juce::Colour (0xff00e5ff));
    bypassButton.setColour (juce::TextButton::textColourOnId, juce::Colour (0xffff5c7a));
    bypassButton.onStateChange = [this]()
    {
        bypassButton.setButtonText (bypassButton.getToggleState() ? "BYPASSED" : "ACTIVE");
    };
    addAndMakeVisible (bypassButton);
}

void VoxOneAudioProcessorEditor::setupRotarySlider (AnimatedVoxKnob& slider, juce::Label& label,
                                                    const juce::String& text, const juce::String& suffix,
                                                    juce::Colour accentColour, double defaultVal, bool bipolar)
{
    slider.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    slider.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 74, 22);
    slider.setTextValueSuffix (suffix);
    slider.setDoubleClickReturnValue (true, defaultVal);
    slider.setBipolar (bipolar);
    slider.setColour (juce::Slider::rotarySliderFillColourId, accentColour);
    slider.setColour (juce::Slider::rotarySliderOutlineColourId, juce::Colour (0xff23293b));
    slider.setColour (juce::Slider::thumbColourId, juce::Colour (0xffffffff));
    addAndMakeVisible (slider);

    label.setText (text, juce::dontSendNotification);
    label.setJustificationType (juce::Justification::centred);
    label.setFont (juce::FontOptions (11.5f, juce::Font::bold));
    label.setColour (juce::Label::textColourId, juce::Colour (0xff9aa5be));
    addAndMakeVisible (label);
}

void VoxOneAudioProcessorEditor::timerCallback()
{
    audioProcessor.copyWaveformData (inWaveCache, outWaveCache);

    bool isBypassed = audioProcessor.apvts.getRawParameterValue ("bypass")->load() > 0.5f;
    waveformVisualizer.setDisplayData (
        inWaveCache,
        outWaveCache,
        audioProcessor.getInputLevel(),
        audioProcessor.getOutputLevel(),
        audioProcessor.getGainReductionDb(),
        isBypassed);

    bool anyKnobAnim = false;
    anyKnobAnim |= inputGainSlider.tickAnimation();
    anyKnobAnim |= hpfSlider.tickAnimation();
    anyKnobAnim |= cleanSlider.tickAnimation();
    anyKnobAnim |= compSlider.tickAnimation();
    anyKnobAnim |= deEssSlider.tickAnimation();
    anyKnobAnim |= airSlider.tickAnimation();
    anyKnobAnim |= clipSlider.tickAnimation();
    anyKnobAnim |= outputGainSlider.tickAnimation();
    anyKnobAnim |= ceilingSlider.tickAnimation();

    auto calcCardGlow = [] (const AnimatedVoxKnob& a, const AnimatedVoxKnob& b)
    {
        return std::max (a.getHoverAmount() * 0.35f + a.getDragAmount() * 0.65f,
                         b.getHoverAmount() * 0.35f + b.getDragAmount() * 0.65f);
    };

    float nextTone = calcCardGlow (hpfSlider, cleanSlider);
    float nextDyn = calcCardGlow (compSlider, deEssSlider);
    float nextChar = calcCardGlow (airSlider, clipSlider);
    float nextMaster = std::max (calcCardGlow (inputGainSlider, outputGainSlider),
                                 ceilingSlider.getHoverAmount() * 0.35f + ceilingSlider.getDragAmount() * 0.65f);

    bool cardsChanged = std::abs (nextTone - toneCardGlow) > 0.002f
                     || std::abs (nextDyn - dynamicsCardGlow) > 0.002f
                     || std::abs (nextChar - characterCardGlow) > 0.002f
                     || std::abs (nextMaster - masterCardGlow) > 0.002f
                     || anyKnobAnim;

    toneCardGlow = nextTone;
    dynamicsCardGlow = nextDyn;
    characterCardGlow = nextChar;
    masterCardGlow = nextMaster;

    if (cardsChanged)
        repaint();

    int hostPreset = audioProcessor.getCurrentPresetIndex();
    if (hostPreset != lastDisplayedPreset)
    {
        lastDisplayedPreset = hostPreset;
        presetSelector.setSelectedId (hostPreset + 1, juce::dontSendNotification);
    }
}

void VoxOneAudioProcessorEditor::drawModuleCard (juce::Graphics& g, juce::Rectangle<int> bounds,
                                                 const juce::String& title, juce::Colour accent, float activityGlow)
{
    auto fBounds = bounds.toFloat();

    if (activityGlow > 0.01f)
    {
        g.setColour (accent.withAlpha (0.09f * activityGlow));
        g.drawRoundedRectangle (fBounds.expanded (1.5f), 15.0f, 3.5f);
    }

    juce::ColourGradient cardGrad (juce::Colour (0xff171d2c), fBounds.getX(), fBounds.getY(),
                                   juce::Colour (0xff10141f), fBounds.getX(), fBounds.getBottom(), false);
    g.setGradientFill (cardGrad);
    g.fillRoundedRectangle (fBounds, 14.0f);

    if (activityGlow > 0.01f)
    {
        juce::ColourGradient ambient (accent.withAlpha (0.11f * activityGlow), fBounds.getCentreX(), fBounds.getY(),
                                      accent.withAlpha (0.0f), fBounds.getCentreX(), fBounds.getBottom(), false);
        g.setGradientFill (ambient);
        g.fillRoundedRectangle (fBounds, 14.0f);
    }

    g.setColour (juce::Colours::white.withAlpha (0.05f + 0.06f * activityGlow));
    g.drawHorizontalLine (static_cast<int> (fBounds.getY() + 1.0f), fBounds.getX() + 14.0f, fBounds.getRight() - 14.0f);

    auto borderCol = juce::Colour (0xff242c42).interpolatedWith (accent, 0.55f * activityGlow);
    g.setColour (borderCol);
    g.drawRoundedRectangle (fBounds.reduced (0.5f), 14.0f, 1.15f);

    auto accentBar = juce::Rectangle<float> (fBounds.getX() + 14.0f, fBounds.getY() + 11.0f, 3.5f, 13.0f);
    g.setColour (accent.interpolatedWith (juce::Colours::white, 0.25f * activityGlow));
    g.fillRoundedRectangle (accentBar, 1.8f);

    g.setColour (juce::Colour (0xffcbd5e1).interpolatedWith (juce::Colours::white, 0.4f * activityGlow));
    g.setFont (juce::FontOptions (11.5f, juce::Font::bold));
    g.drawText (title, bounds.getX() + 24, bounds.getY() + 9, bounds.getWidth() - 32, 18, juce::Justification::centredLeft);
}

void VoxOneAudioProcessorEditor::drawSignalChain (juce::Graphics& g, juce::Rectangle<int> area)
{
    auto pill = area.toFloat();
    g.setColour (juce::Colour (0xff0c0f18));
    g.fillRoundedRectangle (pill, 10.0f);
    g.setColour (juce::Colour (0xff1f263b));
    g.drawRoundedRectangle (pill, 10.0f, 1.0f);

    bool isBypassed = audioProcessor.apvts.getRawParameterValue ("bypass")->load() > 0.5f;

    struct Node
    {
        const char* name;
        juce::Colour colour;
        bool active;
        float focus;
    };

    std::array<Node, 8> nodes {{
        { "IN",     juce::Colour (0xff64d2ff), ! isBypassed, inputGainSlider.getDragAmount() },
        { "HPF",    juce::Colour (0xff00e5ff), ! isBypassed && hpfSlider.getValue() > 21.0, hpfSlider.getDragAmount() },
        { "EQ",     juce::Colour (0xff00e5ff), ! isBypassed && cleanSlider.getValue() > 0.5, cleanSlider.getDragAmount() },
        { "COMP",   juce::Colour (0xff9b7bff), ! isBypassed && compSlider.getValue() > 0.5, compSlider.getDragAmount() },
        { "DE-ESS", juce::Colour (0xff9b7bff), ! isBypassed && deEssSlider.getValue() > 0.5, deEssSlider.getDragAmount() },
        { "AIR",    juce::Colour (0xffff9f43), ! isBypassed && airSlider.getValue() > 0.5, airSlider.getDragAmount() },
        { "CLIP",   juce::Colour (0xffff9f43), ! isBypassed && clipSlider.getValue() > 0.5, clipSlider.getDragAmount() },
        { "LIMIT",  juce::Colour (0xff64d2ff), ! isBypassed, std::max (outputGainSlider.getDragAmount(), ceilingSlider.getDragAmount()) }
    }};

    auto inner = area.reduced (10, 6);
    float slotW = static_cast<float> (inner.getWidth()) / static_cast<float> (nodes.size());

    g.setFont (juce::FontOptions (10.5f, juce::Font::bold));

    for (size_t i = 0; i < nodes.size(); ++i)
    {
        auto slot = juce::Rectangle<float> (inner.getX() + static_cast<float> (i) * slotW,
                                            static_cast<float> (inner.getY()),
                                            slotW,
                                            static_cast<float> (inner.getHeight()));
        auto chip = slot.reduced (4.0f, 5.0f);

        if (nodes[i].focus > 0.02f)
        {
            g.setColour (nodes[i].colour.withAlpha (0.22f * nodes[i].focus));
            g.fillRoundedRectangle (chip, 6.0f);
            g.setColour (nodes[i].colour.withAlpha (0.75f * nodes[i].focus));
            g.drawRoundedRectangle (chip, 6.0f, 1.0f);
        }

        auto textCol = nodes[i].active
            ? nodes[i].colour.interpolatedWith (juce::Colours::white, 0.35f * nodes[i].focus)
            : juce::Colour (0xff475569);

        g.setColour (textCol);
        g.drawFittedText (nodes[i].name, chip.toNearestInt(), juce::Justification::centred, 1);

        if (i + 1 < nodes.size())
        {
            g.setColour (juce::Colour (0xff38435f));
            g.drawText (">", static_cast<int> (slot.getRight() - 4.0f), inner.getY(), 8, inner.getHeight(), juce::Justification::centred);
        }
    }
}

void VoxOneAudioProcessorEditor::paint (juce::Graphics& g)
{
    juce::ColourGradient bgGrad (juce::Colour (0xff090b12), 0.0f, 0.0f,
                                 juce::Colour (0xff101422), 0.0f, static_cast<float> (getHeight()), false);
    g.setGradientFill (bgGrad);
    g.fillAll();

    juce::ColourGradient topGlow (juce::Colour (0xff00e5ff).withAlpha (0.06f), static_cast<float> (getWidth()) * 0.5f, 0.0f,
                                  juce::Colours::transparentBlack, static_cast<float> (getWidth()) * 0.5f, 220.0f, false);
    g.setGradientFill (topGlow);
    g.fillRect (0, 0, getWidth(), 220);

    g.setColour (juce::Colour (0xff121623));
    g.fillRect (0, 0, getWidth(), 64);

    g.setColour (juce::Colour (0xff20283d));
    g.drawHorizontalLine (64, 0.0f, static_cast<float> (getWidth()));

    g.setColour (juce::Colour (0xff00e5ff));
    g.setFont (juce::FontOptions (23.0f, juce::Font::bold));
    g.drawText ("VOXONE", 22, 11, 130, 28, juce::Justification::centredLeft);

    g.setColour (juce::Colour (0xff6c7a99));
    g.setFont (juce::FontOptions (10.0f, juce::Font::bold));
    g.drawText ("VOCAL FINISHING SUITE", 23, 38, 170, 15, juce::Justification::centredLeft);

    drawModuleCard (g, toneCardBounds, "TONE & CLEAN", juce::Colour (0xff00e5ff), toneCardGlow);
    drawModuleCard (g, dynamicsCardBounds, "DYNAMICS", juce::Colour (0xff9b7bff), dynamicsCardGlow);
    drawModuleCard (g, characterCardBounds, "SHINE & DRIVE", juce::Colour (0xffff9f43), characterCardGlow);
    drawModuleCard (g, masterCardBounds, "STAGE & MASTERING", juce::Colour (0xff64d2ff), masterCardGlow);

    auto flowArea = masterCardBounds.withTrimmedLeft (156).withTrimmedRight (296).withTrimmedTop (50).reduced (8, 22);
    drawSignalChain (g, flowArea);
}

void VoxOneAudioProcessorEditor::resized()
{
    int headerY = 16;
    int presetAreaWidth = 316;
    auto presetArea = juce::Rectangle<int> ((getWidth() - presetAreaWidth) / 2, headerY, presetAreaWidth, 32);
    prevPresetButton.setBounds (presetArea.removeFromLeft (32));
    presetArea.removeFromLeft (6);
    nextPresetButton.setBounds (presetArea.removeFromRight (32));
    presetArea.removeFromRight (6);
    presetSelector.setBounds (presetArea);

    bypassButton.setBounds (getWidth() - 120, headerY, 98, 32);

    auto area = getLocalBounds();
    area.removeFromTop (64);
    area.reduce (20, 14);

    waveformVisualizer.setBounds (area.removeFromTop (158));
    area.removeFromTop (14);

    auto middleRow = area.removeFromTop (192);
    area.removeFromTop (14);
    masterCardBounds = area;

    int gap = 14;
    int cardWidth = (middleRow.getWidth() - gap * 2) / 3;

    toneCardBounds = middleRow.removeFromLeft (cardWidth);
    middleRow.removeFromLeft (gap);
    dynamicsCardBounds = middleRow.removeFromLeft (cardWidth);
    middleRow.removeFromLeft (gap);
    characterCardBounds = middleRow;

    auto layoutPair = [] (juce::Rectangle<int> card,
                          juce::Label& l1, AnimatedVoxKnob& s1,
                          juce::Label& l2, AnimatedVoxKnob& s2)
    {
        auto inner = card.withTrimmedTop (32).reduced (10, 8);
        auto left = inner.removeFromLeft (inner.getWidth() / 2);
        auto right = inner;

        l1.setBounds (left.removeFromTop (18));
        s1.setBounds (left.reduced (4, 2));

        l2.setBounds (right.removeFromTop (18));
        s2.setBounds (right.reduced (4, 2));
    };

    layoutPair (toneCardBounds, hpfLabel, hpfSlider, cleanLabel, cleanSlider);
    layoutPair (dynamicsCardBounds, compLabel, compSlider, deEssLabel, deEssSlider);
    layoutPair (characterCardBounds, airLabel, airSlider, clipLabel, clipSlider);

    auto masterInner = masterCardBounds.withTrimmedTop (28).reduced (16, 6);
    auto inputSlot = masterInner.removeFromLeft (124);
    auto ceilingSlot = masterInner.removeFromRight (124);
    masterInner.removeFromRight (16);
    auto outputSlot = masterInner.removeFromRight (124);

    inputGainLabel.setBounds (inputSlot.removeFromTop (16));
    inputGainSlider.setBounds (inputSlot.reduced (4, 0));

    outputGainLabel.setBounds (outputSlot.removeFromTop (16));
    outputGainSlider.setBounds (outputSlot.reduced (4, 0));

    ceilingLabel.setBounds (ceilingSlot.removeFromTop (16));
    ceilingSlider.setBounds (ceilingSlot.reduced (4, 0));
}