#pragma once

#include "PluginProcessor.h"
#include "CustomLookAndFeel.h"

class EnvelopeDisplay final : public juce::Component
{
public:
    explicit EnvelopeDisplay (AudioPluginAudioProcessor& p) : processor (p) {}
    void paint (juce::Graphics&) override;
private:
    AudioPluginAudioProcessor& processor;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (EnvelopeDisplay)
};

class AudioPluginAudioProcessorEditor final : public juce::AudioProcessorEditor, private juce::Timer
{
public:
    explicit AudioPluginAudioProcessorEditor (AudioPluginAudioProcessor&);
    ~AudioPluginAudioProcessorEditor() override;
    void paint (juce::Graphics&) override;
    void resized() override;
private:
    void timerCallback() override;
    void setupKnob (juce::Slider&, juce::Label&, const juce::String&);
    AudioPluginAudioProcessor& processor;
    CustomLookAndFeel lookAndFeel;
    EnvelopeDisplay envelopeDisplay;
    juce::Slider attackSlider, sustainSlider, sensitivitySlider, outputSlider;
    juce::Label title, subtitle, attackLabel, sustainLabel, sensitivityLabel, outputLabel, inputMeter, outputMeter;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> attackAttachment, sustainAttachment,
        sensitivityAttachment, outputAttachment;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AudioPluginAudioProcessorEditor)
};
