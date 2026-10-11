#include "PluginEditor.h"

ShloseAudioProcessorEditor::ShloseAudioProcessorEditor (ShloseAudioProcessor& p)
    : AudioProcessorEditor (&p), proc (p), eq (p),
      freq    (p.apvts, "freq",    "FREQUENCY", ShloseUI::yellow),
      thr     (p.apvts, "thr",     "THRESHOLD", ShloseUI::pink),
      range   (p.apvts, "range",   "RANGE",     ShloseUI::orange),
      q       (p.apvts, "q",       "Q",         ShloseUI::green),
      ratio   (p.apvts, "ratio",   "RATIO",     ShloseUI::purple),
      attack  (p.apvts, "attack",  "ATTACK",    ShloseUI::cyan),
      release (p.apvts, "release", "RELEASE",   ShloseUI::cyan),
      mix     (p.apvts, "mix",     "MIX",       ShloseUI::lime),
      out     (p.apvts, "out",     "OUTPUT",    ShloseUI::lime)
{
    setLookAndFeel (&laf);

    addAndMakeVisible (face);
    addAndMakeVisible (eq);
    addAndMakeVisible (grBar);
    for (auto* k : { &freq, &thr, &range, &q, &ratio, &attack, &release, &mix, &out })
        addAndMakeVisible (*k);

    if (auto* choice = dynamic_cast<juce::AudioParameterChoice*> (p.apvts.getParameter ("mode")))
        modeBox.addItemList (choice->choices, 1);
    addAndMakeVisible (modeBox);
    modeAttach = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment> (p.apvts, "mode", modeBox);

    listenBtn.setClickingTogglesState (true);
    addAndMakeVisible (listenBtn);
    listenAttach = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (p.apvts, "listen", listenBtn);

    setSize (1240, 620);
    startTimerHz (30);
}

ShloseAudioProcessorEditor::~ShloseAudioProcessorEditor()
{
    stopTimer();
    setLookAndFeel (nullptr);
}

void ShloseAudioProcessorEditor::timerCallback()
{
    const float gr = proc.grMeter.load();
    face.setGR (gr);
    grBar.setGR (gr);
    eq.update();
}

void ShloseAudioProcessorEditor::paint (juce::Graphics& g)
{
    using namespace ShloseUI;
    const float W = (float) getWidth(), H = (float) getHeight();

    g.setGradientFill (juce::ColourGradient (skyTop, 0.0f, 0.0f, skyBottom, 0.0f, H, false));
    g.fillAll();

    auto cloud = [&] (float x, float y, float s)
    {
        g.setColour (white.withAlpha (0.28f));
        g.fillEllipse (x, y + 12.0f * s, 130.0f * s, 46.0f * s);
        g.fillEllipse (x + 22.0f * s, y, 64.0f * s, 54.0f * s);
        g.fillEllipse (x + 64.0f * s, y + 6.0f * s, 58.0f * s, 46.0f * s);
    };
    cloud (60.0f, 480.0f, 1.2f);
    cloud (880.0f, 14.0f, 1.0f);
    cloud (1090.0f, 30.0f, 0.7f);
    cloud (300.0f, 570.0f, 0.9f);
    cloud (700.0f, 6.0f, 0.8f);

    auto sparkle = [&] (float x, float y, float s)
    {
        juce::Path st;
        st.addStar ({ x, y }, 4, s * 0.28f, s, 0.0f);
        g.setColour (white.withAlpha (0.85f));
        g.fillPath (st);
    };
    sparkle (330.0f, 30.0f, 9.0f);
    sparkle (860.0f, 56.0f, 7.0f);
    sparkle (30.0f, 440.0f, 8.0f);
    sparkle (1215.0f, 40.0f, 10.0f);
    sparkle (1100.0f, 8.0f + 20.0f, 6.0f);

    // title
    {
        juce::Font f = font (50.0f);
        f.setExtraKerningFactor (0.12f);
        juce::GlyphArrangement ga;
        ga.addLineOfText (f, "SHLOSE", 346.0f, 54.0f);
        juce::Path title;
        ga.createPath (title);
        g.setColour (navy);
        g.strokePath (title, juce::PathStrokeType (9.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
        g.setGradientFill (juce::ColourGradient (yellow, 0.0f, 14.0f, orange, 0.0f, 56.0f, false));
        g.fillPath (title);
    }
    drawShadowText (g, "THE S-MUNCHER!", { 600, 30, 200, 24 }, juce::Justification::centredLeft, font (15.0f));

    // right panel
    auto panel = juce::Rectangle<float> (868.0f, 72.0f, 356.0f, 536.0f);
    g.setColour (navy.withAlpha (0.25f));
    g.fillRoundedRectangle (panel.translated (0.0f, 4.0f), 26.0f);
    g.setColour (white.withAlpha (0.16f));
    g.fillRoundedRectangle (panel, 26.0f);
    g.setColour (white.withAlpha (0.7f));
    g.drawRoundedRectangle (panel.reduced (1.5f), 26.0f, 3.0f);

    juce::ignoreUnused (W);
}

void ShloseAudioProcessorEditor::resized()
{
    face.setBounds (16, 8, 300, 456);
    modeBox.setBounds (24, 474, 176, 46);
    listenBtn.setBounds (208, 474, 100, 46);
    grBar.setBounds (24, 530, 284, 66);

    eq.setBounds (332, 72, 520, 536);

    // right column: biggest = most important
    freq.setBounds  (878, 78, 168, 206);
    thr.setBounds   (1046, 78, 168, 206);

    range.setBounds (878, 290, 112, 164);
    q.setBounds     (990, 290, 112, 164);
    ratio.setBounds (1102, 290, 112, 164);

    attack.setBounds  (878, 460, 84, 130);
    release.setBounds (962, 460, 84, 130);
    mix.setBounds     (1046, 460, 84, 130);
    out.setBounds     (1130, 460, 84, 130);
}
