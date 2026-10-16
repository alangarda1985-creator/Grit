#include "PluginEditor.h"

GritAudioProcessorEditor::GritAudioProcessorEditor (GritAudioProcessor& p)
    : AudioProcessorEditor (&p), processor (p)
{
    addKnob (knobs[0], "input_gain",  "INPUT");
    addKnob (knobs[1], "gate",        "GATE");
    addKnob (knobs[2], "chaos",       "CHAOS");
    addKnob (knobs[3], "speed",       "SPEED");
    addKnob (knobs[4], "noise",       "NOISE");
    addKnob (knobs[5], "mix",         "MIX");
    addKnob (knobs[6], "output_gain", "OUTPUT");

    setSize (770, 260);
}

void GritAudioProcessorEditor::addKnob (Knob& k, const juce::String& id, const juce::String& text)
{
    k.slider.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 80, 18);
    addAndMakeVisible (k.slider);

    k.label.setText (text, juce::dontSendNotification);
    k.label.setJustificationType (juce::Justification::centred);
    k.label.setColour (juce::Label::textColourId, juce::Colours::white.withAlpha (0.8f));
    addAndMakeVisible (k.label);

    k.attachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (
        processor.apvts, id, k.slider);
}

void GritAudioProcessorEditor::paint (juce::Graphics& g)
{
    g.fillAll (juce::Colour (0xff1b1b1f));
    g.setColour (juce::Colours::white);
    g.setFont (juce::FontOptions (22.f, juce::Font::bold));
    g.drawText ("GRIT", getLocalBounds().removeFromTop (44), juce::Justification::centred);
}

void GritAudioProcessorEditor::resized()
{
    auto area = getLocalBounds().reduced (10);
    area.removeFromTop (40);
    const int w = area.getWidth() / (int) knobs.size();

    for (auto& k : knobs)
    {
        auto cell = area.removeFromLeft (w);
        k.label.setBounds  (cell.removeFromTop (24));
        k.slider.setBounds (cell.reduced (4));
    }
}
