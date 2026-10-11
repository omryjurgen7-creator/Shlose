#pragma once
#include <JuceHeader.h>
#include "Dsp.h"

class ShloseAudioProcessor : public juce::AudioProcessor
{
public:
    ShloseAudioProcessor();
    ~ShloseAudioProcessor() override = default;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return "Shlose"; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return {}; }
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    juce::AudioProcessorValueTreeState apvts;

    std::atomic<float> grMeter { 0.0f };   // current gain reduction in dB (positive)

private:
    static juce::AudioProcessorValueTreeState::ParameterLayout createLayout();

    std::atomic<float> *pFreq, *pQ, *pThr, *pRatio, *pRange, *pAttack, *pRelease,
                       *pMode, *pListen, *pMix, *pOut;

    Biquad filt[2];
    double sr = 44100.0;
    float lastFreq = -1.0f, lastQ = -1.0f;
    double detEnv = 0.0, grDb = 0.0;

    juce::SmoothedValue<float> mixSm, outSm;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ShloseAudioProcessor)
};
