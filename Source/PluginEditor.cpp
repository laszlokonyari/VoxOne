#include "PluginProcessor.h"
#include "PluginEditor.h"

VoxOneAudioProcessorEditor::VoxOneAudioProcessorEditor (VoxOneAudioProcessor& p)
    : AudioProcessorEditor (&p), audioProcessor (p)
{
    setLookAndFeel (&customLookAndFeel);

    setupHeaderControls();
    addAndMakeVisible (waveformVisualizer);

    auto cyanAccent = juce::Colour (0xff00e5ff);
    auto violetAccent = juce::Colour (0xff8b7bff);
    auto amberAccent = juce::Colour (0xffff9f43);
    auto silverAccent = juce::Colour (0xff64d2ff);

    setupRotarySlider (hpfSlider, hpfLabel, "HPF", " Hz", cyanAccent);
    setupRotarySlider (cleanSlider, cleanLabel, "CLEAN EQ", " %", cyanAccent);

    setupRotarySlider (compSlider, compLabel, "COMP", " %", violetAccent);
    setupRotarySlider (deEssSlider, deEssLabel, "DE-ESS", " %", violetAccent);

    setupRotarySlider (airSlider, airLabel, "FRESH AIR", " %", amberAccent);
    setupRotarySlider (clipSlider, clipLabel, "SOFT CLIP", " %", amberAccent);

    setupRotarySlider (inputGainSlider, inputGainLabel, "INPUT", " dB", silverAccent);
    setupRotarySlider (outputGainSlider, outputGainLabel, "OUTPUT", " dB", silverAccent);
    setupRotarySlider (ceilingSlider, ceilingLabel, "CEILING", " dB", silverAccent);

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

    setSize (960, 580);
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
    bypassButton.setColour (juce::TextButton::buttonColourId, juce::Colour (0xff17232c));
    bypassButton.setColour (juce::TextButton::buttonOnColourId, juce::Colour (0xff3a1723));
    bypassButton.setColour (juce::TextButton::textColourOffId, juce::Colour (0xff00e5ff));
    bypassButton.setColour (juce::TextButton::textColourOnId, juce::Colour (0xffff5c7a));
    bypassButton.onStateChange = [this]()
    {
        bypassButton.setButtonText (bypassButton.getToggleState() ? "BYPASSED" : "ACTIVE");
    };
    addAndMakeVisible (bypassButton);
}

void VoxOneAudioProcessorEditor::setupRotarySlider (juce::Slider& slider, juce::Label& label,
                                                    const juce::String& text, const juce::String& suffix,
                                                    juce::Colour accentColour)
{
    slider.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    slider.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 72, 22);
    slider.setTextValueSuffix (suffix);
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

    int hostPreset = audioProcessor.getCurrentPresetIndex();
    if (hostPreset != lastDisplayedPreset)
    {
        lastDisplayedPreset = hostPreset;
        presetSelector.setSelectedId (hostPreset + 1, juce::dontSendNotification);
    }
}

void VoxOneAudioProcessorEditor::drawModuleCard (juce::Graphics& g, juce::Rectangle<int> bounds,
                                                 const juce::String& title, juce::Colour accent)
{
    auto fBounds = bounds.toFloat();

    juce::ColourGradient cardGrad (juce::Colour (0xff171c29), fBounds.getX(), fBounds.getY(),
                                   juce::Colour (0xff121621), fBounds.getX(), fBounds.getBottom(), false);
    g.setGradientFill (cardGrad);
    g.fillRoundedRectangle (fBounds, 12.0f);

    g.setColour (juce::Colour (0xff252c40));
    g.drawRoundedRectangle (fBounds.reduced (0.5f), 12.0f, 1.1f);

    auto accentBar = juce::Rectangle<float> (fBounds.getX() + 14.0f, fBounds.getY() + 10.0f, 3.0f, 13.0f);
    g.setColour (accent);
    g.fillRoundedRectangle (accentBar, 1.5f);

    g.setColour (juce::Colour (0xffcbd5e1));
    g.setFont (juce::FontOptions (11.5f, juce::Font::bold));
    g.drawText (title, bounds.getX() + 24, bounds.getY() + 8, bounds.getWidth() - 32, 18, juce::Justification::centredLeft);
}

void VoxOneAudioProcessorEditor::paint (juce::Graphics& g)
{
    juce::ColourGradient bgGrad (juce::Colour (0xff0b0d14), 0.0f, 0.0f,
                                 juce::Colour (0xff111522), 0.0f, static_cast<float> (getHeight()), false);
    g.setGradientFill (bgGrad);
    g.fillAll();

    g.setColour (juce::Colour (0xff131724));
    g.fillRect (0, 0, getWidth(), 62);

    g.setColour (juce::Colour (0xff22293d));
    g.drawHorizontalLine (62, 0.0f, static_cast<float> (getWidth()));

    g.setColour (juce::Colour (0xff00e5ff));
    g.setFont (juce::FontOptions (23.0f, juce::Font::bold));
    g.drawText ("VOXONE", 22, 10, 125, 28, juce::Justification::centredLeft);

    g.setColour (juce::Colour (0xff64748b));
    g.setFont (juce::FontOptions (10.5f, juce::Font::bold));
    g.drawText ("VOCAL FINISHING SUITE", 23, 36, 160, 16, juce::Justification::centredLeft);

    drawModuleCard (g, toneCardBounds, "TONE & CLEAN", juce::Colour (0xff00e5ff));
    drawModuleCard (g, dynamicsCardBounds, "DYNAMICS", juce::Colour (0xff8b7bff));
    drawModuleCard (g, characterCardBounds, "SHINE & DRIVE", juce::Colour (0xffff9f43));
    drawModuleCard (g, masterCardBounds, "STAGE & MASTERING", juce::Colour (0xff64d2ff));

    auto flowArea = masterCardBounds.withTrimmedLeft (150).withTrimmedRight (290).withTrimmedTop (24);
    g.setColour (juce::Colour (0xff10141f));
    g.fillRoundedRectangle (flowArea.reduced (12, 26).toFloat(), 8.0f);
    g.setColour (juce::Colour (0xff22293d));
    g.drawRoundedRectangle (flowArea.reduced (12, 26).toFloat(), 8.0f, 1.0f);

    g.setColour (juce::Colour (0xff7d89a6));
    g.setFont (juce::FontOptions (11.0f, juce::Font::bold));
    g.drawFittedText ("IN  >  HPF  >  EQ  >  COMP  >  DE-ESS  >  AIR  >  CLIP  >  LIMIT",
                      flowArea.reduced (18, 26), juce::Justification::centred, 1);
}

void VoxOneAudioProcessorEditor::resized()
{
    int headerY = 15;
    int presetAreaWidth = 310;
    auto presetArea = juce::Rectangle<int> ((getWidth() - presetAreaWidth) / 2, headerY, presetAreaWidth, 32);
    prevPresetButton.setBounds (presetArea.removeFromLeft (32));
    presetArea.removeFromLeft (6);
    nextPresetButton.setBounds (presetArea.removeFromRight (32));
    presetArea.removeFromRight (6);
    presetSelector.setBounds (presetArea);

    bypassButton.setBounds (getWidth() - 118, headerY, 96, 32);

    auto area = getLocalBounds();
    area.removeFromTop (62);
    area.reduce (20, 14);

    waveformVisualizer.setBounds (area.removeFromTop (154));
    area.removeFromTop (14);

    auto middleRow = area.removeFromTop (184);
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
                          juce::Label& l1, juce::Slider& s1,
                          juce::Label& l2, juce::Slider& s2)
    {
        auto inner = card.withTrimmedTop (30).reduced (10, 8);
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

    auto masterInner = masterCardBounds.withTrimmedTop (26).reduced (16, 6);
    auto inputSlot = masterInner.removeFromLeft (120);
    auto ceilingSlot = masterInner.removeFromRight (120);
    masterInner.removeFromRight (16);
    auto outputSlot = masterInner.removeFromRight (120);

    inputGainLabel.setBounds (inputSlot.removeFromTop (16));
    inputGainSlider.setBounds (inputSlot.reduced (4, 0));

    outputGainLabel.setBounds (outputSlot.removeFromTop (16));
    outputGainSlider.setBounds (outputSlot.reduced (4, 0));

    ceilingLabel.setBounds (ceilingSlot.removeFromTop (16));
    ceilingSlider.setBounds (ceilingSlot.reduced (4, 0));
}