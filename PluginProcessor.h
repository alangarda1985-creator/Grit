#pragma once
#include <juce_audio_processors/juce_audio_processors.h>

class GritAudioProcessor : public juce::AudioProcessor
{
public:
    GritAudioProcessor();
    ~GritAudioProcessor() override = default;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported (const BusesLayout&) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return "Grit"; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return {}; }
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock&) override;
    void setStateInformation (const void*, int) override;

    static juce::AudioProcessorValueTreeState::ParameterLayout createLayout();

    juce::AudioProcessorValueTreeState apvts;

private:
    std::atomic<float>* pInGain  = nullptr;
    std::atomic<float>* pOutGain = nullptr;
    std::atomic<float>* pMix     = nullptr;
    std::atomic<float>* pChaos   = nullptr;
    std::atomic<float>* pSpeed   = nullptr;
    std::atomic<float>* pNoise   = nullptr;
    std::atomic<float>* pGate    = nullptr;

    juce::SmoothedValue<float> inGain, outGain, mix;
    juce::Random rng;

    double sr = 44100.0;

    // Modulatore casuale (sample & hold con slew), condiviso tra i canali
    float phase = 0.f, mod = 0.f, modTarget = 0.f;
    int   holdCount = 0;

    // Stato per canale
    float held[2]      { 0.f, 0.f };
    float env[2]       { 0.f, 0.f };
    float gateGain[2]  { 1.f, 1.f };

    // Coefficienti one-pole
    float envAtk = 0.f, envRel = 0.f, gateSlew = 0.f, modSlew = 0.f;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (GritAudioProcessor)
};
