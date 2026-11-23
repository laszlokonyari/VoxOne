#include "PluginProcessor.h"
#include "PluginEditor.h"

VoxOneAudioProcessorEditor::VoxOneAudioProcessorEditor (VoxOneAudioProcessor& p)
    : AudioProcessorEditor (&p), audioProcessor (p)
{
    setLookAndFeel (&customLookAndFeel);

    setupHeaderControls();
    setupSpaceControls();

    addAndMakeVisible (waveformVisualizer);
    addAndMakeVisible (spaceVisualizer);

    auto cyanAccent = juce::Colour (0xff00e5ff);
    auto violetAccent = juce::Colour (0xff9b7bff);
    auto amberAccent = juce::Colour (0xffff9f43);
    auto tealAccent = juce::Colour (0xff14f1b2);
    auto magentaAccent = juce::Colour (0xffe879f9);
    auto silverAccent = juce::Colour (0xff64d2ff);

    setupRotarySlider (hpfSlider, hpfLabel, "HPF", " Hz", cyanAccent, 80.0);
    setupRotarySlider (cleanSlider, cleanLabel, "CLEAN EQ", " %", cyanAccent, 50.0);

    setupRotarySlider (compSlider, compLabel, "COMP", " %", violetAccent, 40.0);
    setupRotarySlider (deEssSlider, deEssLabel, "DE-ESS", " %", violetAccent, 35.0);

    setupRotarySlider (airSlider, airLabel, "FRESH AIR", " %", amberAccent, 30.0);
    setupRotarySlider (clipSlider, clipLabel, "SOFT CLIP", " %", amberAccent, 20.0);

    setupRotarySlider (revMixSlider, revMixLabel, "MIX", " %", tealAccent, 20.0, false, 58);
    setupRotarySlider (revSizeSlider, revSizeLabel, "SIZE", " %", tealAccent, 42.0, false, 58);
    setupRotarySlider (revPreDelaySlider, revPreDelayLabel, "PRE-DLY", " ms", tealAccent, 24.0, false, 60);
    setupRotarySlider (revToneSlider, revToneLabel, "TONE", " %", tealAccent, 60.0, false, 58);
    setupRotarySlider (revWidthSlider, revWidthLabel, "WIDTH", " %", tealAccent, 75.0, false, 58);
    setupRotarySlider (revDuckSlider, revDuckLabel, "DUCK", " %", tealAccent, 35.0, false, 58);

    setupRotarySlider (delMixSlider, delMixLabel, "MIX", " %", magentaAccent, 16.0, false, 58);
    setupRotarySlider (delTimeSlider, delTimeLabel, "TIME", " ms", magentaAccent, 115.0, false, 60);
    setupRotarySlider (delFeedbackSlider, delFeedbackLabel, "FDBK", " %", magentaAccent, 22.0, false, 58);
    setupRotarySlider (delToneSlider, delToneLabel, "TONE", " %", magentaAccent, 48.0, false, 58);
    setupRotarySlider (delDuckSlider, delDuckLabel, "DUCK", " %", magentaAccent, 40.0, false, 58);

    setupRotarySlider (inputGainSlider, inputGainLabel, "INPUT", " dB", silverAccent, 0.0, true, 64);
    setupRotarySlider (outputGainSlider, outputGainLabel, "OUTPUT", " dB", silverAccent, 0.0, true, 64);
    setupRotarySlider (ceilingSlider, ceilingLabel, "CEILING", " dB", silverAccent, -0.3, false, 64);

    inputGainAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (audioProcessor.apvts, "inputGain", inputGainSlider);
    hpfAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (audioProcessor.apvts, "hpfFreq", hpfSlider);
    cleanAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (audioProcessor.apvts, "cleanAmount", cleanSlider);
    compAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (audioProcessor.apvts, "compAmount", compSlider);
    deEssAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (audioProcessor.apvts, "deEssAmount", deEssSlider);
    airAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (audioProcessor.apvts, "airAmount", airSlider);
    clipAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (audioProcessor.apvts, "clipAmount", clipSlider);
    outputGainAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (audioProcessor.apvts, "outputGain", outputGainSlider);
    ceilingAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (audioProcessor.apvts, "limiterCeiling", ceilingSlider);

    revMixAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (audioProcessor.apvts, "reverbMix", revMixSlider);
    revSizeAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (audioProcessor.apvts, "reverbSize", revSizeSlider);
    revPreDelayAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (audioProcessor.apvts, "reverbPreDelay", revPreDelaySlider);
    revToneAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (audioProcessor.apvts, "reverbTone", revToneSlider);
    revWidthAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (audioProcessor.apvts, "reverbWidth", revWidthSlider);
    revDuckAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (audioProcessor.apvts, "reverbDuck", revDuckSlider);

    delMixAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (audioProcessor.apvts, "delayMix", delMixSlider);
    delTimeAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (audioProcessor.apvts, "delayTime", delTimeSlider);
    delFeedbackAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (audioProcessor.apvts, "delayFeedback", delFeedbackSlider);
    delToneAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (audioProcessor.apvts, "delayTone", delToneSlider);
    delDuckAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (audioProcessor.apvts, "delayDuck", delDuckSlider);

    bypassAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (audioProcessor.apvts, "bypass", bypassButton);
    spaceBypassAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (audioProcessor.apvts, "spaceBypass", spaceBypassButton);
    pingPongAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (audioProcessor.apvts, "delayPingPong", pingPongButton);

    setSize (1040, 760);
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

void VoxOneAudioProcessorEditor::setupSpaceControls()
{
    static constexpr const char* modeTitles[] = { "ROOM", "PLATE", "HALL", "AMBIENCE" };

    for (int i = 0; i < 4; ++i)
    {
        auto& btn = reverbModeButtons[static_cast<size_t> (i)];
        btn.setButtonText (modeTitles[i]);
        btn.setClickingTogglesState (false);
        btn.setColour (juce::TextButton::buttonColourId, juce::Colour (0xff131926));
        btn.setColour (juce::TextButton::buttonOnColourId, juce::Colour (0xff123230));
        btn.setColour (juce::TextButton::textColourOffId, juce::Colour (0xff8a96b2));
        btn.setColour (juce::TextButton::textColourOnId, juce::Colour (0xff14f1b2));
        btn.onClick = [this, i]()
        {
            if (auto* param = audioProcessor.apvts.getParameter ("reverbMode"))
            {
                auto norm = audioProcessor.apvts.getParameterRange ("reverbMode").convertTo0to1 (static_cast<float> (i));
                param->setValueNotifyingHost (norm);
            }
        };
        addAndMakeVisible (btn);
    }

    spaceBypassButton.setClickingTogglesState (true);
    spaceBypassButton.setColour (juce::TextButton::buttonColourId, juce::Colour (0xff122c2b));
    spaceBypassButton.setColour (juce::TextButton::buttonOnColourId, juce::Colour (0xff361824));
    spaceBypassButton.setColour (juce::TextButton::textColourOffId, juce::Colour (0xff14f1b2));
    spaceBypassButton.setColour (juce::TextButton::textColourOnId, juce::Colour (0xffff5c7a));
    spaceBypassButton.onStateChange = [this]()
    {
        spaceBypassButton.setButtonText (spaceBypassButton.getToggleState() ? "SPACE OFF" : "SPACE ON");
    };
    addAndMakeVisible (spaceBypassButton);

    pingPongLabel.setText ("ROUTING", juce::dontSendNotification);
    pingPongLabel.setJustificationType (juce::Justification::centred);
    pingPongLabel.setFont (juce::FontOptions (10.5f, juce::Font::bold));
    pingPongLabel.setColour (juce::Label::textColourId, juce::Colour (0xff9aa5be));
    addAndMakeVisible (pingPongLabel);

    pingPongButton.setClickingTogglesState (true);
    pingPongButton.setColour (juce::TextButton::buttonColourId, juce::Colour (0xff181d2c));
    pingPongButton.setColour (juce::TextButton::buttonOnColourId, juce::Colour (0xff31193c));
    pingPongButton.setColour (juce::TextButton::textColourOffId, juce::Colour (0xff9aa5be));
    pingPongButton.setColour (juce::TextButton::textColourOnId, juce::Colour (0xffe879f9));
    pingPongButton.onStateChange = [this]()
    {
        pingPongButton.setButtonText (pingPongButton.getToggleState() ? "PING-PONG" : "STEREO");
    };
    addAndMakeVisible (pingPongButton);
}

void VoxOneAudioProcessorEditor::setupRotarySlider (AnimatedVoxKnob& slider, juce::Label& label,
                                                    const juce::String& text, const juce::String& suffix,
                                                    juce::Colour accentColour, double defaultVal,
                                                    bool bipolar, int textBoxWidth)
{
    slider.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    slider.setTextBoxStyle (juce::Slider::TextBoxBelow, false, textBoxWidth, 19);
    slider.setTextValueSuffix (suffix);
    slider.setDoubleClickReturnValue (true, defaultVal);
    slider.setBipolar (bipolar);
    slider.setColour (juce::Slider::rotarySliderFillColourId, accentColour);
    slider.setColour (juce::Slider::rotarySliderOutlineColourId, juce::Colour (0xff23293b));
    slider.setColour (juce::Slider::thumbColourId, juce::Colour (0xffffffff));
    addAndMakeVisible (slider);

    label.setText (text, juce::dontSendNotification);
    label.setJustificationType (juce::Justification::centred);
    label.setFont (juce::FontOptions (10.5f, juce::Font::bold));
    label.setColour (juce::Label::textColourId, juce::Colour (0xff9aa5be));
    addAndMakeVisible (label);
}

void VoxOneAudioProcessorEditor::timerCallback()
{
    audioProcessor.copyWaveformData (inWaveCache, outWaveCache);

    bool isBypassed = audioProcessor.apvts.getRawParameterValue ("bypass")->load() > 0.5f;
    bool isSpaceBypassed = audioProcessor.apvts.getRawParameterValue ("spaceBypass")->load() > 0.5f;

    waveformVisualizer.setDisplayData (
        inWaveCache, outWaveCache,
        audioProcessor.getInputLevel(),
        audioProcessor.getOutputLevel(),
        audioProcessor.getGainReductionDb(),
        isBypassed);

    int activeRevMode = static_cast<int> (std::round (audioProcessor.apvts.getRawParameterValue ("reverbMode")->load()));
    for (int i = 0; i < 4; ++i)
        reverbModeButtons[static_cast<size_t> (i)].setToggleState (i == activeRevMode, juce::dontSendNotification);

    SpaceVisualState sv;
    sv.reverbMode = activeRevMode;
    sv.reverbMix = audioProcessor.apvts.getRawParameterValue ("reverbMix")->load();
    sv.reverbSize = audioProcessor.apvts.getRawParameterValue ("reverbSize")->load();
    sv.reverbPreDelay = audioProcessor.apvts.getRawParameterValue ("reverbPreDelay")->load();
    sv.reverbTone = audioProcessor.apvts.getRawParameterValue ("reverbTone")->load();
    sv.reverbWidth = audioProcessor.apvts.getRawParameterValue ("reverbWidth")->load();
    sv.reverbDuck = audioProcessor.apvts.getRawParameterValue ("reverbDuck")->load();
    sv.delayMix = audioProcessor.apvts.getRawParameterValue ("delayMix")->load();
    sv.delayTime = audioProcessor.apvts.getRawParameterValue ("delayTime")->load();
    sv.delayFeedback = audioProcessor.apvts.getRawParameterValue ("delayFeedback")->load();
    sv.delayTone = audioProcessor.apvts.getRawParameterValue ("delayTone")->load();
    sv.delayDuck = audioProcessor.apvts.getRawParameterValue ("delayDuck")->load();
    sv.delayPingPong = audioProcessor.apvts.getRawParameterValue ("delayPingPong")->load() > 0.5f;
    sv.reverbEnergy = audioProcessor.getReverbEnergy();
    sv.delayEnergy = audioProcessor.getDelayEnergy();
    sv.duckActivity = audioProcessor.getSpaceDuckActivity();
    sv.spaceActive = (! isBypassed && ! isSpaceBypassed);
    spaceVisualizer.setState (sv);

    bool anyKnobAnim = false;
    AnimatedVoxKnob* allKnobs[] = {
        &inputGainSlider, &hpfSlider, &cleanSlider, &compSlider, &deEssSlider,
        &airSlider, &clipSlider, &outputGainSlider, &ceilingSlider,
        &revMixSlider, &revSizeSlider, &revPreDelaySlider, &revToneSlider, &revWidthSlider, &revDuckSlider,
        &delMixSlider, &delTimeSlider, &delFeedbackSlider, &delToneSlider, &delDuckSlider
    };

    for (auto* k : allKnobs)
        anyKnobAnim |= k->tickAnimation();

    auto knobGlow = [] (const AnimatedVoxKnob& k)
    {
        return k.getHoverAmount() * 0.35f + k.getDragAmount() * 0.65f;
    };

    float nextTone = std::max (knobGlow (hpfSlider), knobGlow (cleanSlider));
    float nextDyn = std::max (knobGlow (compSlider), knobGlow (deEssSlider));
    float nextChar = std::max (knobGlow (airSlider), knobGlow (clipSlider));
    float nextMaster = std::max ({ knobGlow (inputGainSlider), knobGlow (outputGainSlider), knobGlow (ceilingSlider) });

    float nextSpace = std::max ({
        knobGlow (revMixSlider), knobGlow (revSizeSlider), knobGlow (revPreDelaySlider),
        knobGlow (revToneSlider), knobGlow (revWidthSlider), knobGlow (revDuckSlider),
        knobGlow (delMixSlider), knobGlow (delTimeSlider), knobGlow (delFeedbackSlider),
        knobGlow (delToneSlider), knobGlow (delDuckSlider)
    });

    bool cardsChanged = std::abs (nextTone - toneCardGlow) > 0.002f
                     || std::abs (nextDyn - dynamicsCardGlow) > 0.002f
                     || std::abs (nextChar - characterCardGlow) > 0.002f
                     || std::abs (nextSpace - spaceCardGlow) > 0.002f
                     || std::abs (nextMaster - masterCardGlow) > 0.002f
                     || anyKnobAnim;

    toneCardGlow = nextTone;
    dynamicsCardGlow = nextDyn;
    characterCardGlow = nextChar;
    spaceCardGlow = nextSpace;
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
        g.drawRoundedRectangle (fBounds.expanded (1.5f), 15.0f, 3.2f);
    }

    juce::ColourGradient cardGrad (juce::Colour (0xff171d2c), fBounds.getX(), fBounds.getY(),
                                   juce::Colour (0xff10141f), fBounds.getX(), fBounds.getBottom(), false);
    g.setGradientFill (cardGrad);
    g.fillRoundedRectangle (fBounds, 13.0f);

    if (activityGlow > 0.01f)
    {
        juce::ColourGradient ambient (accent.withAlpha (0.10f * activityGlow), fBounds.getCentreX(), fBounds.getY(),
                                      accent.withAlpha (0.0f), fBounds.getCentreX(), fBounds.getBottom(), false);
        g.setGradientFill (ambient);
        g.fillRoundedRectangle (fBounds, 13.0f);
    }

    g.setColour (juce::Colours::white.withAlpha (0.05f + 0.06f * activityGlow));
    g.drawHorizontalLine (static_cast<int> (fBounds.getY() + 1.0f), fBounds.getX() + 14.0f, fBounds.getRight() - 14.0f);

    auto borderCol = juce::Colour (0xff242c42).interpolatedWith (accent, 0.55f * activityGlow);
    g.setColour (borderCol);
    g.drawRoundedRectangle (fBounds.reduced (0.5f), 13.0f, 1.1f);

    auto accentBar = juce::Rectangle<float> (fBounds.getX() + 13.0f, fBounds.getY() + 10.0f, 3.5f, 12.0f);
    g.setColour (accent.interpolatedWith (juce::Colours::white, 0.25f * activityGlow));
    g.fillRoundedRectangle (accentBar, 1.8f);

    g.setColour (juce::Colour (0xffcbd5e1).interpolatedWith (juce::Colours::white, 0.4f * activityGlow));
    g.setFont (juce::FontOptions (11.0f, juce::Font::bold));
    g.drawText (title, bounds.getX() + 23, bounds.getY() + 8, bounds.getWidth() - 32, 16, juce::Justification::centredLeft);
}

void VoxOneAudioProcessorEditor::drawSignalChain (juce::Graphics& g, juce::Rectangle<int> area)
{
    auto pill = area.toFloat();
    g.setColour (juce::Colour (0xff0c0f18));
    g.fillRoundedRectangle (pill, 9.0f);
    g.setColour (juce::Colour (0xff1f263b));
    g.drawRoundedRectangle (pill, 9.0f, 1.0f);

    bool isBypassed = audioProcessor.apvts.getRawParameterValue ("bypass")->load() > 0.5f;
    bool spaceOff = audioProcessor.apvts.getRawParameterValue ("spaceBypass")->load() > 0.5f;
    bool spaceActive = ! isBypassed && ! spaceOff && (revMixSlider.getValue() > 0.5 || delMixSlider.getValue() > 0.5);

    float spaceFocus = std::max ({
        revMixSlider.getDragAmount(), revSizeSlider.getDragAmount(), revPreDelaySlider.getDragAmount(),
        revToneSlider.getDragAmount(), revWidthSlider.getDragAmount(), revDuckSlider.getDragAmount(),
        delMixSlider.getDragAmount(), delTimeSlider.getDragAmount(), delFeedbackSlider.getDragAmount(),
        delToneSlider.getDragAmount(), delDuckSlider.getDragAmount()
    });

    struct Node
    {
        const char* name;
        juce::Colour colour;
        bool active;
        float focus;
    };

    std::array<Node, 9> nodes {{
        { "IN",     juce::Colour (0xff64d2ff), ! isBypassed, inputGainSlider.getDragAmount() },
        { "HPF",    juce::Colour (0xff00e5ff), ! isBypassed && hpfSlider.getValue() > 21.0, hpfSlider.getDragAmount() },
        { "EQ",     juce::Colour (0xff00e5ff), ! isBypassed && cleanSlider.getValue() > 0.5, cleanSlider.getDragAmount() },
        { "COMP",   juce::Colour (0xff9b7bff), ! isBypassed && compSlider.getValue() > 0.5, compSlider.getDragAmount() },
        { "DE-ESS", juce::Colour (0xff9b7bff), ! isBypassed && deEssSlider.getValue() > 0.5, deEssSlider.getDragAmount() },
        { "AIR",    juce::Colour (0xffff9f43), ! isBypassed && airSlider.getValue() > 0.5, airSlider.getDragAmount() },
        { "CLIP",   juce::Colour (0xffff9f43), ! isBypassed && clipSlider.getValue() > 0.5, clipSlider.getDragAmount() },
        { "SPACE",  juce::Colour (0xff14f1b2), spaceActive, spaceFocus },
        { "LIMIT",  juce::Colour (0xff64d2ff), ! isBypassed, std::max (outputGainSlider.getDragAmount(), ceilingSlider.getDragAmount()) }
    }};

    auto inner = area.reduced (8, 4);
    float slotW = static_cast<float> (inner.getWidth()) / static_cast<float> (nodes.size());

    g.setFont (juce::FontOptions (10.0f, juce::Font::bold));

    for (size_t i = 0; i < nodes.size(); ++i)
    {
        auto slot = juce::Rectangle<float> (inner.getX() + static_cast<float> (i) * slotW,
                                            static_cast<float> (inner.getY()),
                                            slotW,
                                            static_cast<float> (inner.getHeight()));
        auto chip = slot.reduced (3.0f, 4.0f);

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

    juce::ColourGradient topGlow (juce::Colour (0xff00e5ff).withAlpha (0.055f), static_cast<float> (getWidth()) * 0.5f, 0.0f,
                                  juce::Colours::transparentBlack, static_cast<float> (getWidth()) * 0.5f, 200.0f, false);
    g.setGradientFill (topGlow);
    g.fillRect (0, 0, getWidth(), 200);

    g.setColour (juce::Colour (0xff121623));
    g.fillRect (0, 0, getWidth(), 58);

    g.setColour (juce::Colour (0xff20283d));
    g.drawHorizontalLine (58, 0.0f, static_cast<float> (getWidth()));

    g.setColour (juce::Colour (0xff00e5ff));
    g.setFont (juce::FontOptions (22.0f, juce::Font::bold));
    g.drawText ("VOXONE", 22, 8, 130, 26, juce::Justification::centredLeft);

    g.setColour (juce::Colour (0xff6c7a99));
    g.setFont (juce::FontOptions (9.8f, juce::Font::bold));
    g.drawText ("VOCAL FINISHING SUITE", 23, 34, 170, 14, juce::Justification::centredLeft);

    drawModuleCard (g, toneCardBounds, "TONE & CLEAN", juce::Colour (0xff00e5ff), toneCardGlow);
    drawModuleCard (g, dynamicsCardBounds, "DYNAMICS", juce::Colour (0xff9b7bff), dynamicsCardGlow);
    drawModuleCard (g, characterCardBounds, "SHINE & DRIVE", juce::Colour (0xffff9f43), characterCardGlow);
    drawModuleCard (g, spaceCardBounds, "SPACE & DIMENSION", juce::Colour (0xff14f1b2), spaceCardGlow);
    drawModuleCard (g, masterCardBounds, "STAGE & MASTERING", juce::Colour (0xff64d2ff), masterCardGlow);

    auto drawSubBox = [&] (juce::Rectangle<int> sub, const juce::String& label, juce::Colour col)
    {
        g.setColour (juce::Colour (0xff0c101a));
        g.fillRoundedRectangle (sub.toFloat(), 10.0f);
        g.setColour (juce::Colour (0xff1e263b));
        g.drawRoundedRectangle (sub.toFloat().reduced (0.5f), 10.0f, 1.0f);

        g.setColour (col);
        g.setFont (juce::FontOptions (10.2f, juce::Font::bold));
        g.drawText (label, sub.getX() + 12, sub.getY() + 5, 140, 15, juce::Justification::centredLeft);
    };

    drawSubBox (reverbSubBounds, "VOCAL REVERB", juce::Colour (0xff14f1b2));
    drawSubBox (delaySubBounds, "SLAP DELAY", juce::Colour (0xffe879f9));

    auto flowArea = masterCardBounds.withTrimmedLeft (148).withTrimmedRight (280).withTrimmedTop (34).reduced (8, 18);
    drawSignalChain (g, flowArea);
}

void VoxOneAudioProcessorEditor::resized()
{
    int headerY = 13;
    int presetAreaWidth = 320;
    auto presetArea = juce::Rectangle<int> ((getWidth() - presetAreaWidth) / 2, headerY, presetAreaWidth, 32);
    prevPresetButton.setBounds (presetArea.removeFromLeft (32));
    presetArea.removeFromLeft (6);
    nextPresetButton.setBounds (presetArea.removeFromRight (32));
    presetArea.removeFromRight (6);
    presetSelector.setBounds (presetArea);

    bypassButton.setBounds (getWidth() - 120, headerY, 98, 32);

    auto area = getLocalBounds();
    area.removeFromTop (58);
    area.reduce (18, 10);

    waveformVisualizer.setBounds (area.removeFromTop (124));
    area.removeFromTop (10);

    auto vocalRow = area.removeFromTop (156);
    area.removeFromTop (10);

    spaceCardBounds = area.removeFromTop (248);
    area.removeFromTop (10);

    masterCardBounds = area;

    int gap = 12;
    int cardWidth = (vocalRow.getWidth() - gap * 2) / 3;
    toneCardBounds = vocalRow.removeFromLeft (cardWidth);
    vocalRow.removeFromLeft (gap);
    dynamicsCardBounds = vocalRow.removeFromLeft (cardWidth);
    vocalRow.removeFromLeft (gap);
    characterCardBounds = vocalRow;

    auto layoutPair = [] (juce::Rectangle<int> card,
                          juce::Label& l1, AnimatedVoxKnob& s1,
                          juce::Label& l2, AnimatedVoxKnob& s2)
    {
        auto inner = card.withTrimmedTop (26).reduced (10, 4);
        auto left = inner.removeFromLeft (inner.getWidth() / 2);
        auto right = inner;

        l1.setBounds (left.removeFromTop (15));
        s1.setBounds (left.reduced (4, 1));

        l2.setBounds (right.removeFromTop (15));
        s2.setBounds (right.reduced (4, 1));
    };

    layoutPair (toneCardBounds, hpfLabel, hpfSlider, cleanLabel, cleanSlider);
    layoutPair (dynamicsCardBounds, compLabel, compSlider, deEssLabel, deEssSlider);
    layoutPair (characterCardBounds, airLabel, airSlider, clipLabel, clipSlider);

    auto spaceHeader = spaceCardBounds.withHeight (34);
    spaceBypassButton.setBounds (spaceHeader.getRight() - 102, spaceHeader.getY() + 6, 90, 22);

    int modeW = 70;
    int modeGap = 6;
    int totalModesW = modeW * 4 + modeGap * 3;
    int modeStartX = spaceHeader.getCentreX() - totalModesW / 2;
    for (int i = 0; i < 4; ++i)
        reverbModeButtons[static_cast<size_t> (i)].setBounds (modeStartX + i * (modeW + modeGap), spaceHeader.getY() + 6, modeW, 22);

    auto spaceBody = spaceCardBounds.withTrimmedTop (34).reduced (10, 8);
    int sideW = 326;
    reverbSubBounds = spaceBody.removeFromLeft (sideW);
    spaceBody.removeFromLeft (10);
    delaySubBounds = spaceBody.removeFromRight (sideW);
    spaceBody.removeFromRight (10);
    spaceVisualizer.setBounds (spaceBody);

    auto layoutGridCell = [] (juce::Rectangle<int> cell, juce::Label& lbl, AnimatedVoxKnob& knob)
    {
        lbl.setBounds (cell.removeFromTop (14));
        knob.setBounds (cell.reduced (2, 0));
    };

    auto revGrid = reverbSubBounds.withTrimmedTop (22).reduced (6, 4);
    auto revTop = revGrid.removeFromTop (revGrid.getHeight() / 2);
    auto revBot = revGrid;
    int revColW = revTop.getWidth() / 3;

    layoutGridCell (revTop.removeFromLeft (revColW), revMixLabel, revMixSlider);
    layoutGridCell (revTop.removeFromLeft (revColW), revSizeLabel, revSizeSlider);
    layoutGridCell (revTop, revPreDelayLabel, revPreDelaySlider);

    layoutGridCell (revBot.removeFromLeft (revColW), revToneLabel, revToneSlider);
    layoutGridCell (revBot.removeFromLeft (revColW), revWidthLabel, revWidthSlider);
    layoutGridCell (revBot, revDuckLabel, revDuckSlider);

    auto delGrid = delaySubBounds.withTrimmedTop (22).reduced (6, 4);
    auto delTop = delGrid.removeFromTop (delGrid.getHeight() / 2);
    auto delBot = delGrid;
    int delColW = delTop.getWidth() / 3;

    layoutGridCell (delTop.removeFromLeft (delColW), delMixLabel, delMixSlider);
    layoutGridCell (delTop.removeFromLeft (delColW), delTimeLabel, delTimeSlider);
    layoutGridCell (delTop, delFeedbackLabel, delFeedbackSlider);

    layoutGridCell (delBot.removeFromLeft (delColW), delToneLabel, delToneSlider);
    layoutGridCell (delBot.removeFromLeft (delColW), delDuckLabel, delDuckSlider);

    auto ppCell = delBot;
    pingPongLabel.setBounds (ppCell.removeFromTop (14));
    pingPongButton.setBounds (ppCell.reduced (8, 20));

    auto masterInner = masterCardBounds.withTrimmedTop (22).reduced (16, 4);
    auto inputSlot = masterInner.removeFromLeft (118);
    auto ceilingSlot = masterInner.removeFromRight (118);
    masterInner.removeFromRight (14);
    auto outputSlot = masterInner.removeFromRight (118);

    inputGainLabel.setBounds (inputSlot.removeFromTop (14));
    inputGainSlider.setBounds (inputSlot.reduced (4, 0));

    outputGainLabel.setBounds (outputSlot.removeFromTop (14));
    outputGainSlider.setBounds (outputSlot.reduced (4, 0));

    ceilingLabel.setBounds (ceilingSlot.removeFromTop (14));
    ceilingSlider.setBounds (ceilingSlot.reduced (4, 0));
}