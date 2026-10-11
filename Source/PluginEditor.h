#pragma once
#include <JuceHeader.h>
#include "PluginProcessor.h"
#include "UI.h"

class ShloseAudioProcessorEditor : public juce::AudioProcessorEditor, private juce::Timer
{
public:
    explicit ShloseAudioProcessorEditor (ShloseAudioProcessor&);
    ~ShloseAudioProcessorEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    void timerCallback() override;

    ShloseAudioProcessor& proc;
    ShloseLookAndFeel laf;

    Knob freq, q, thr, range, ratio, attack, release, mix, out;
    GRMeter meter;

    juce::ComboBox modeBox;
    juce::TextButton listenBtn { "LISTEN" };
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> modeAttach;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> listenAttach;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ShloseAudioProcessorEditor)
};
