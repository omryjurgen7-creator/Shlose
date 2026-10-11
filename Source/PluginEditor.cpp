#include "PluginEditor.h"

ShloseAudioProcessorEditor::ShloseAudioProcessorEditor (ShloseAudioProcessor& p)
    : AudioProcessorEditor (&p), proc (p),
      freq    (p.apvts, "freq",    "FREQUENCY"),
      q       (p.apvts, "q",       "Q"),
      thr     (p.apvts, "thr",     "THRESHOLD"),
      range   (p.apvts, "range",   "RANGE"),
      ratio   (p.apvts, "ratio",   "RATIO"),
      attack  (p.apvts, "attack",  "ATTACK"),
      release (p.apvts, "release", "RELEASE"),
      mix     (p.apvts, "mix",     "MIX"),
      out     (p.apvts, "out",     "OUTPUT")
{
    setLookAndFeel (&laf);

    for (auto* k : { &freq, &q, &thr, &range, &ratio, &attack, &release, &mix, &out })
        addAndMakeVisible (*k);
    addAndMakeVisible (meter);

    if (auto* choice = dynamic_cast<juce::AudioParameterChoice*> (p.apvts.getParameter ("mode")))
        modeBox.addItemList (choice->choices, 1);
    addAndMakeVisible (modeBox);
    modeAttach = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment> (p.apvts, "mode", modeBox);

    listenBtn.setClickingTogglesState (true);
    addAndMakeVisible (listenBtn);
    listenAttach = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (p.apvts, "listen", listenBtn);

    setSize (740, 450);
    startTimerHz (30);
}

ShloseAudioProcessorEditor::~ShloseAudioProcessorEditor()
{
    stopTimer();
    setLookAndFeel (nullptr);
}

void ShloseAudioProcessorEditor::timerCallback()
{
    meter.setGR (proc.grMeter.load());
}

void ShloseAudioProcessorEditor::paint (juce::Graphics& g)
{
    using namespace ShloseUI;
    g.fillAll (bg);

    // header
    auto f = font (30.0f, true);
    f.setExtraKerningFactor (0.2f);
    g.setFont (f);
    g.setColour (text);
    g.drawText ("SHLOSE", 24, 14, 200, 36, juce::Justification::centredLeft);
    g.setColour (accent);
    g.fillRoundedRectangle (24.0f, 52.0f, 40.0f, 3.0f, 1.5f);
    g.setColour (dim);
    g.setFont (font (11.0f, true));
    g.drawText ("PROFESSIONAL DE-ESSER", 74, 46, 220, 14, juce::Justification::centredLeft);

    // panels
    g.setColour (panel);
    g.fillRoundedRectangle (16.0f, 78.0f, 568.0f, 190.0f, 10.0f);
    g.fillRoundedRectangle (16.0f, 278.0f, 568.0f, 156.0f, 10.0f);
    g.setColour (edge);
    g.drawRoundedRectangle (16.5f, 78.5f, 567.0f, 189.0f, 10.0f, 1.0f);
    g.drawRoundedRectangle (16.5f, 278.5f, 567.0f, 155.0f, 10.0f, 1.0f);
}

void ShloseAudioProcessorEditor::resized()
{
    auto b = getLocalBounds();

    auto header = b.removeFromTop (70).reduced (24, 18);
    listenBtn.setBounds (header.removeFromRight (90));
    header.removeFromRight (10);
    modeBox.setBounds (header.removeFromRight (150));

    meter.setBounds (600, 78, 124, 356);

    auto row1 = juce::Rectangle<int> (16, 78, 568, 190).reduced (12, 8);
    const int w1 = row1.getWidth() / 4;
    for (auto* k : { &freq, &q, &thr, &range })
        k->setBounds (row1.removeFromLeft (w1));

    auto row2 = juce::Rectangle<int> (16, 278, 568, 156).reduced (12, 6);
    const int w2 = row2.getWidth() / 5;
    for (auto* k : { &ratio, &attack, &release, &mix, &out })
        k->setBounds (row2.removeFromLeft (w2));
}
