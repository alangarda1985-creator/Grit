#pragma once
#include "PluginProcessor.h"

class GritAudioProcessorEditor : public juce::AudioProcessorEditor
{
public:
    explicit GritAudioProcessorEditor (GritAudioProcessor&);
    void paint (juce::Graphics&) override;
    void resized() override;

private:
    struct Knob
    {
        juce::Slider slider { juce::Slider::RotaryHorizontalVerticalDrag, juce::Slider::TextBoxBelow };
        juce::Label  label;
        std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> attachment;
    };

    void addKnob (Knob&, const juce::String& paramId, const juce::String& text);

    GritAudioProcessor& processor;
    std::array<Knob, 7> knobs;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (GritAudioProcessorEditor)
};
