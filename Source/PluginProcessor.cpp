#include "PluginProcessor.h"
#include "PluginEditor.h"

namespace
{
    juce::String fmtFreq (float v)
    {
        return v >= 1000.0f ? juce::String (v / 1000.0f, 2) + " kHz"
                            : juce::String ((int) v) + " Hz";
    }
}

juce::AudioProcessorValueTreeState::ParameterLayout ShloseAudioProcessor::createLayout()
{
    using namespace juce;
    using Attr = AudioParameterFloatAttributes;
    std::vector<std::unique_ptr<RangedAudioParameter>> p;

    auto addSimple = [&] (const char* id, const char* name, NormalisableRange<float> r,
                          float def, String suffix, int decimals)
    {
        p.push_back (std::make_unique<AudioParameterFloat> (
            ParameterID { id, 1 }, name, r, def,
            Attr().withStringFromValueFunction ([suffix, decimals] (float v, int)
                                                { return String (v, decimals) + suffix; })
                  .withValueFromStringFunction ([] (const String& t)
                                                { return t.retainCharacters ("0123456789.-").getFloatValue(); })));
    };

    // Frequency (custom formatting)
    p.push_back (std::make_unique<AudioParameterFloat> (
        ParameterID { "freq", 1 }, "Frequency",
        NormalisableRange<float> (2000.0f, 16000.0f, 1.0f, 0.5f), 7000.0f,
        Attr().withStringFromValueFunction ([] (float v, int) { return fmtFreq (v); })
              .withValueFromStringFunction ([] (const String& t)
              {
                  float v = t.retainCharacters ("0123456789.").getFloatValue();
                  if (t.containsIgnoreCase ("k") || v < 100.0f) v *= 1000.0f;
                  return jlimit (2000.0f, 16000.0f, v);
              })));

    addSimple ("q",       "Q",         { 0.5f, 4.0f, 0.01f },          1.4f,  "",     2);
    addSimple ("thr",     "Threshold", { -60.0f, 0.0f, 0.1f },         -30.0f, " dB", 1);
    addSimple ("ratio",   "Ratio",     { 1.5f, 20.0f, 0.1f, 0.5f },    4.0f,  ":1",   1);
    addSimple ("range",   "Range",     { 0.0f, 24.0f, 0.1f },          12.0f, " dB",  1);
    addSimple ("attack",  "Attack",    { 0.05f, 20.0f, 0.01f, 0.4f },  0.5f,  " ms",  2);
    addSimple ("release", "Release",   { 5.0f, 500.0f, 0.1f, 0.4f },   80.0f, " ms",  0);
    addSimple ("mix",     "Mix",       { 0.0f, 100.0f, 0.1f },         100.0f," %",   0);
    addSimple ("out",     "Output",    { -12.0f, 12.0f, 0.1f },        0.0f,  " dB",  1);

    p.push_back (std::make_unique<AudioParameterChoice> (
        ParameterID { "mode", 1 }, "Mode", StringArray { "Split-band", "Wideband" }, 0));
    p.push_back (std::make_unique<AudioParameterBool> (
        ParameterID { "listen", 1 }, "Listen", false));

    return { p.begin(), p.end() };
}

ShloseAudioProcessor::ShloseAudioProcessor()
    : AudioProcessor (BusesProperties()
                          .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                          .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "STATE", createLayout())
{
    pFreq    = apvts.getRawParameterValue ("freq");
    pQ       = apvts.getRawParameterValue ("q");
    pThr     = apvts.getRawParameterValue ("thr");
    pRatio   = apvts.getRawParameterValue ("ratio");
    pRange   = apvts.getRawParameterValue ("range");
    pAttack  = apvts.getRawParameterValue ("attack");
    pRelease = apvts.getRawParameterValue ("release");
    pMode    = apvts.getRawParameterValue ("mode");
    pListen  = apvts.getRawParameterValue ("listen");
    pMix     = apvts.getRawParameterValue ("mix");
    pOut     = apvts.getRawParameterValue ("out");
}

bool ShloseAudioProcessor::isBusesLayoutSupported (const BusesLayout& l) const
{
    const auto& o = l.getMainOutputChannelSet();
    if (o != juce::AudioChannelSet::mono() && o != juce::AudioChannelSet::stereo())
        return false;
    return o == l.getMainInputChannelSet();
}

void ShloseAudioProcessor::prepareToPlay (double sampleRate, int)
{
    sr = sampleRate;
    for (auto& f : filt) f.reset();
    lastFreq = lastQ = -1.0f;
    detEnv = 0.0;
    grDb = 0.0;
    mixSm.reset (sr, 0.03);  mixSm.setCurrentAndTargetValue (*pMix * 0.01f);
    outSm.reset (sr, 0.03);  outSm.setCurrentAndTargetValue (juce::Decibels::decibelsToGain (pOut->load()));
}

void ShloseAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;

    const int numCh = juce::jmin (buffer.getNumChannels(), 2);
    const int n = buffer.getNumSamples();
    for (int ch = numCh; ch < buffer.getNumChannels(); ++ch)
        buffer.clear (ch, 0, n);

    const float freq = pFreq->load(), q = pQ->load();
    if (freq != lastFreq || q != lastQ)
    {
        for (auto& f : filt) f.setBandpass (sr, freq, q);
        lastFreq = freq;
        lastQ = q;
    }

    const double thr     = pThr->load();
    const double ratio   = pRatio->load();
    const double range   = pRange->load();
    const bool   wide    = pMode->load() > 0.5f;
    const bool   listen  = pListen->load() > 0.5f;
    const double slope   = 1.0 - 1.0 / ratio;
    const double knee    = 6.0;

    const double atkCoef = std::exp (-1.0 / (juce::jmax (0.01f, pAttack->load())  * 0.001 * sr));
    const double relCoef = std::exp (-1.0 / (juce::jmax (1.0f,  pRelease->load()) * 0.001 * sr));
    const double detRel  = std::exp (-1.0 / (0.010 * sr));

    mixSm.setTargetValue (pMix->load() * 0.01f);
    outSm.setTargetValue (juce::Decibels::decibelsToGain (pOut->load()));

    float* data[2] = { buffer.getWritePointer (0), numCh > 1 ? buffer.getWritePointer (1) : nullptr };
    double blockMaxGR = 0.0;

    for (int i = 0; i < n; ++i)
    {
        double bp[2] = { 0.0, 0.0 };
        double det = 0.0;
        for (int ch = 0; ch < numCh; ++ch)
        {
            bp[ch] = filt[ch].process ((double) data[ch][i]);
            det = juce::jmax (det, std::abs (bp[ch]));
        }

        // peak detector with fast attack / 10 ms release
        detEnv = det > detEnv ? det : det + detRel * (detEnv - det);
        const double levelDb = 20.0 * std::log10 (juce::jmax (detEnv, 1.0e-6));

        // soft-knee gain computer (dB of reduction, positive)
        const double over = levelDb - thr;
        double target = 0.0;
        if (2.0 * over > knee)
            target = over * slope;
        else if (2.0 * over > -knee)
            target = slope * (over + knee * 0.5) * (over + knee * 0.5) / (2.0 * knee);
        target = juce::jmin (target, range);

        const double coef = target > grDb ? atkCoef : relCoef;
        grDb = target + coef * (grDb - target);
        blockMaxGR = juce::jmax (blockMaxGR, grDb);

        const double g = std::pow (10.0, -grDb / 20.0);
        const float mix = mixSm.getNextValue();
        const float outG = outSm.getNextValue();

        for (int ch = 0; ch < numCh; ++ch)
        {
            const double dry = data[ch][i];
            double outS;
            if (listen)
                outS = bp[ch];
            else
            {
                const double wet = wide ? dry * g : dry - (1.0 - g) * bp[ch];
                outS = (dry * (1.0 - mix) + wet * mix) * outG;
            }
            data[ch][i] = (float) outS;
        }
    }

    grMeter.store ((float) blockMaxGR);
}

juce::AudioProcessorEditor* ShloseAudioProcessor::createEditor()
{
    return new ShloseAudioProcessorEditor (*this);
}

void ShloseAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    if (auto xml = apvts.copyState().createXml())
        copyXmlToBinary (*xml, destData);
}

void ShloseAudioProcessor::setStateInformation (const void* data, int size)
{
    if (auto xml = getXmlFromBinary (data, size))
        if (xml->hasTagName (apvts.state.getType()))
            apvts.replaceState (juce::ValueTree::fromXml (*xml));
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new ShloseAudioProcessor();
}
