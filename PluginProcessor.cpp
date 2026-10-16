#include "PluginProcessor.h"
#include "PluginEditor.h"
#include <cmath>

using APVTS = juce::AudioProcessorValueTreeState;

APVTS::ParameterLayout GritAudioProcessor::createLayout()
{
    using juce::NormalisableRange;
    using juce::ParameterID;
    using juce::AudioParameterFloat;
    using Attr = juce::AudioParameterFloatAttributes;

    auto percent = [] (float v, int) { return juce::String (juce::roundToInt (v * 100.f)) + " %"; };
    auto decibel = [] (float v, int) { return juce::String (v, 1) + " dB"; };
    auto hertz   = [] (float v, int) { return juce::String (v, 2) + " Hz"; };
    auto gateTxt = [] (float v, int) { return v <= -89.9f ? juce::String ("Off")
                                                          : juce::String (v, 1) + " dB"; };

    std::vector<std::unique_ptr<juce::RangedAudioParameter>> p;

    p.push_back (std::make_unique<AudioParameterFloat> (ParameterID { "input_gain", 1 }, "Input",
        NormalisableRange<float> (-24.f, 24.f, 0.1f), 0.f, Attr().withStringFromValueFunction (decibel)));
    p.push_back (std::make_unique<AudioParameterFloat> (ParameterID { "output_gain", 1 }, "Output",
        NormalisableRange<float> (-24.f, 24.f, 0.1f), 0.f, Attr().withStringFromValueFunction (decibel)));
    p.push_back (std::make_unique<AudioParameterFloat> (ParameterID { "mix", 1 }, "Mix",
        NormalisableRange<float> (0.f, 1.f, 0.001f), 1.f, Attr().withStringFromValueFunction (percent)));
    p.push_back (std::make_unique<AudioParameterFloat> (ParameterID { "chaos", 1 }, "Chaos",
        NormalisableRange<float> (0.f, 1.f, 0.001f), 0.4f, Attr().withStringFromValueFunction (percent)));
    p.push_back (std::make_unique<AudioParameterFloat> (ParameterID { "speed", 1 }, "Speed",
        NormalisableRange<float> (0.1f, 50.f, 0.01f, 0.4f), 4.f, Attr().withStringFromValueFunction (hertz)));
    p.push_back (std::make_unique<AudioParameterFloat> (ParameterID { "noise", 1 }, "Noise",
        NormalisableRange<float> (0.f, 1.f, 0.001f), 0.2f, Attr().withStringFromValueFunction (percent)));
    p.push_back (std::make_unique<AudioParameterFloat> (ParameterID { "gate", 1 }, "Gate",
        NormalisableRange<float> (-90.f, 0.f, 0.1f), -90.f, Attr().withStringFromValueFunction (gateTxt)));

    return { p.begin(), p.end() };
}

GritAudioProcessor::GritAudioProcessor()
    : AudioProcessor (BusesProperties()
                          .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                          .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "STATE", createLayout())
{
    pInGain  = apvts.getRawParameterValue ("input_gain");
    pOutGain = apvts.getRawParameterValue ("output_gain");
    pMix     = apvts.getRawParameterValue ("mix");
    pChaos   = apvts.getRawParameterValue ("chaos");
    pSpeed   = apvts.getRawParameterValue ("speed");
    pNoise   = apvts.getRawParameterValue ("noise");
    pGate    = apvts.getRawParameterValue ("gate");
}

bool GritAudioProcessor::isBusesLayoutSupported (const BusesLayout& l) const
{
    const auto out = l.getMainOutputChannelSet();
    if (out != juce::AudioChannelSet::mono() && out != juce::AudioChannelSet::stereo())
        return false;
    return out == l.getMainInputChannelSet();
}

void GritAudioProcessor::prepareToPlay (double sampleRate, int)
{
    sr = sampleRate;

    auto coef = [this] (float ms) { return 1.f - std::exp (-1.f / (0.001f * ms * (float) sr)); };
    envAtk   = coef (1.f);
    envRel   = coef (80.f);
    gateSlew = coef (10.f);
    modSlew  = coef (5.f);

    inGain.reset  (sr, 0.02);
    outGain.reset (sr, 0.02);
    mix.reset     (sr, 0.02);
    inGain.setCurrentAndTargetValue  (juce::Decibels::decibelsToGain (pInGain->load()));
    outGain.setCurrentAndTargetValue (juce::Decibels::decibelsToGain (pOutGain->load()));
    mix.setCurrentAndTargetValue     (pMix->load());

    rng.setSeedRandomly();
    phase = mod = modTarget = 0.f;
    holdCount = 0;
    for (int c = 0; c < 2; ++c) { held[c] = 0.f; env[c] = 0.f; gateGain[c] = 1.f; }
}

void GritAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;

    const int numCh = juce::jmin (buffer.getNumChannels(), 2);
    const int n     = buffer.getNumSamples();

    for (int c = getTotalNumInputChannels(); c < getTotalNumOutputChannels(); ++c)
        buffer.clear (c, 0, n);

    // Parametri letti una volta per blocco
    const float chaos    = pChaos->load();
    const float noiseAmt = pNoise->load();
    const float phaseInc = pSpeed->load() / (float) sr;
    const float gateDb   = pGate->load();
    const float thr      = gateDb <= -89.9f ? 0.f : juce::Decibels::decibelsToGain (gateDb);

    inGain.setTargetValue  (juce::Decibels::decibelsToGain (pInGain->load()));
    outGain.setTargetValue (juce::Decibels::decibelsToGain (pOutGain->load()));
    mix.setTargetValue     (pMix->load());

    float* data[2] { buffer.getWritePointer (0), numCh > 1 ? buffer.getWritePointer (1) : nullptr };

    for (int i = 0; i < n; ++i)
    {
        const float gIn  = inGain.getNextValue();
        const float gOut = outGain.getNextValue();
        const float m    = mix.getNextValue();

        // Modulatore casuale: nuovo valore a ogni ciclo, raggiunto con slew di ~5 ms
        phase += phaseInc;
        if (phase >= 1.f)
        {
            phase -= std::floor (phase);
            modTarget = rng.nextFloat() * 2.f - 1.f;
        }
        mod += (modTarget - mod) * modSlew;

        // Sample-rate reduction con fattore casuale (1..64 campioni)
        bool capture = false;
        if (--holdCount <= 0)
        {
            const int holdLen = 1 + (int) std::lround (chaos * (0.5f + 0.5f * mod) * 63.f);
            holdCount = holdLen;
            capture = true;
        }

        const float flutter = 1.f + 0.6f * chaos * mod;

        for (int ch = 0; ch < numCh; ++ch)
        {
            const float dry = data[ch][i] * gIn;
            if (capture) held[ch] = dry;

            // Gate: segue l'inviluppo dell'ingresso; sotto soglia si torna al segnale pulito
            const float a = std::abs (dry);
            env[ch] += (a - env[ch]) * (a > env[ch] ? envAtk : envRel);
            const float target = env[ch] >= thr ? 1.f : 0.f;
            gateGain[ch] += (target - gateGain[ch]) * gateSlew;

            // Rumore che "cavalca" la dinamica dell'ingresso
            const float noise = (rng.nextFloat() * 2.f - 1.f) * noiseAmt * (0.05f + env[ch]);

            const float wet      = std::tanh (held[ch] * flutter + noise);
            const float wetGated = dry + (wet - dry) * gateGain[ch];

            data[ch][i] = (dry + (wetGated - dry) * m) * gOut;
        }
    }
}

juce::AudioProcessorEditor* GritAudioProcessor::createEditor()
{
    return new GritAudioProcessorEditor (*this);
}

void GritAudioProcessor::getStateInformation (juce::MemoryBlock& dest)
{
    if (auto xml = apvts.copyState().createXml())
        copyXmlToBinary (*xml, dest);
}

void GritAudioProcessor::setStateInformation (const void* data, int size)
{
    if (auto xml = getXmlFromBinary (data, size))
        if (xml->hasTagName (apvts.state.getType()))
            apvts.replaceState (juce::ValueTree::fromXml (*xml));
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new GritAudioProcessor();
}
