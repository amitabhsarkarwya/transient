#include "PluginProcessor.h"
#include "PluginEditor.h"

namespace
{
constexpr auto attackID = "ATTACK";
constexpr auto sustainID = "SUSTAIN";
constexpr auto sensitivityID = "SENSITIVITY";
constexpr auto outputID = "OUTPUT";
}

AudioPluginAudioProcessor::AudioPluginAudioProcessor()
    : AudioProcessor (BusesProperties().withInput ("Input", juce::AudioChannelSet::stereo(), true)
                                        .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "Parameters", createParameterLayout())
{
    // Cache parameter atomics here so the audio callback does no string lookup.
    attackParameter = apvts.getRawParameterValue (attackID);
    sustainParameter = apvts.getRawParameterValue (sustainID);
    sensitivityParameter = apvts.getRawParameterValue (sensitivityID);
    outputParameter = apvts.getRawParameterValue (outputID);
}

void AudioPluginAudioProcessor::prepareToPlay (double sampleRate, int)
{
    sampleRateHz = sampleRate > 0.0 ? sampleRate : 44100.0;
    fastEnvelope.fill (0.0f);
    bodyEnvelope.fill (0.0f);
    gainSmoothing.fill (0.0f);
    inputPeakDb.store (-100.0f, std::memory_order_relaxed);
    outputPeakDb.store (-100.0f, std::memory_order_relaxed);
    attackActivity.store (0.0f, std::memory_order_relaxed);
}

bool AudioPluginAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    const auto input = layouts.getMainInputChannelSet();
    const auto output = layouts.getMainOutputChannelSet();
    return (output == juce::AudioChannelSet::mono() || output == juce::AudioChannelSet::stereo())
        && input == output;
}

float AudioPluginAudioProcessor::getParameter (const juce::String& id) const noexcept
{
    if (auto* parameter = apvts.getRawParameterValue (id))
        return parameter->load (std::memory_order_relaxed);
    return 0.0f;
}

float AudioPluginAudioProcessor::coefficient (double sampleRate, float milliseconds) noexcept
{
    const auto seconds = juce::jmax (0.00001, static_cast<double> (milliseconds) * 0.001);
    return static_cast<float> (std::exp (-1.0 / (sampleRate * seconds)));
}

void AudioPluginAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;
    const int channels = juce::jmin (getTotalNumInputChannels(), buffer.getNumChannels());
    const int samples = buffer.getNumSamples();
    for (int channel = channels; channel < getTotalNumOutputChannels(); ++channel)
        buffer.clear (channel, 0, samples);
    if (channels == 0 || samples == 0)
        return;

    const float attackDb = attackParameter->load (std::memory_order_relaxed);
    const float sustainDb = sustainParameter->load (std::memory_order_relaxed);
    const float sensitivity = sensitivityParameter->load (std::memory_order_relaxed);
    const float outputDb = outputParameter->load (std::memory_order_relaxed);
    const float outputGain = juce::Decibels::decibelsToGain (outputDb);
    const float fastAttack = coefficient (sampleRateHz, 0.8f);
    const float fastRelease = coefficient (sampleRateHz, 55.0f);
    const float bodyAttack = coefficient (sampleRateHz, 18.0f);
    const float bodyRelease = coefficient (sampleRateHz, 135.0f);
    const float gainCoeff = coefficient (sampleRateHz, 3.5f);
    float peakIn = 0.0f, peakOut = 0.0f, activityTotal = 0.0f;
    const float activityScale = sensitivity * 2.5f;

    for (int channel = 0; channel < channels; ++channel)
    {
        auto* data = buffer.getWritePointer (channel);
        auto& fast = fastEnvelope[static_cast<size_t> (channel)];
        auto& body = bodyEnvelope[static_cast<size_t> (channel)];
        auto& smoothedGain = gainSmoothing[static_cast<size_t> (channel)];

        for (int sample = 0; sample < samples; ++sample)
        {
            const float input = data[sample];
            const float level = std::abs (input);
            peakIn = juce::jmax (peakIn, level);

            const float fastCoeff = level > fast ? fastAttack : fastRelease;
            const float bodyCoeff = level > body ? bodyAttack : bodyRelease;
            fast = fastCoeff * fast + (1.0f - fastCoeff) * level;
            body = bodyCoeff * body + (1.0f - bodyCoeff) * level;

            // A fast-over-slow envelope difference marks a transient onset.
            const float transient = juce::jlimit (0.0f, 1.0f,
                (fast - body) / (fast + 1.0e-5f) * activityScale);
            const float bodyWeight = 1.0f - transient;
            const float targetDb = attackDb * transient + sustainDb * bodyWeight;
            smoothedGain = gainCoeff * smoothedGain + (1.0f - gainCoeff) * targetDb;

            const float output = input * juce::Decibels::decibelsToGain (smoothedGain) * outputGain;
            data[sample] = output;
            peakOut = juce::jmax (peakOut, std::abs (output));
            activityTotal += transient;
        }
    }

    const auto toDb = [] (float value) noexcept { return value > 1.0e-8f ? 20.0f * std::log10 (value) : -160.0f; };
    inputPeakDb.store (toDb (peakIn), std::memory_order_relaxed);
    outputPeakDb.store (toDb (peakOut), std::memory_order_relaxed);
    attackActivity.store (activityTotal / static_cast<float> (samples * channels), std::memory_order_relaxed);
}

juce::AudioProcessorValueTreeState::ParameterLayout AudioPluginAudioProcessor::createParameterLayout()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> parameters;
    parameters.push_back (std::make_unique<juce::AudioParameterFloat> (attackID, "Attack",
        juce::NormalisableRange<float> (-12.0f, 12.0f, 0.1f), 0.0f));
    parameters.push_back (std::make_unique<juce::AudioParameterFloat> (sustainID, "Sustain",
        juce::NormalisableRange<float> (-12.0f, 12.0f, 0.1f), 0.0f));
    parameters.push_back (std::make_unique<juce::AudioParameterFloat> (sensitivityID, "Sensitivity",
        juce::NormalisableRange<float> (0.25f, 4.0f, 0.01f, 0.4f), 1.0f));
    parameters.push_back (std::make_unique<juce::AudioParameterFloat> (outputID, "Output",
        juce::NormalisableRange<float> (-12.0f, 12.0f, 0.1f), 0.0f));
    return { parameters.begin(), parameters.end() };
}

void AudioPluginAudioProcessor::getStateInformation (juce::MemoryBlock& destination)
{
    if (auto xml = apvts.copyState().createXml())
        copyXmlToBinary (*xml, destination);
}

void AudioPluginAudioProcessor::setStateInformation (const void* data, int size)
{
    if (std::unique_ptr<juce::XmlElement> xml (getXmlFromBinary (data, size)); xml && xml->hasTagName (apvts.state.getType()))
        apvts.replaceState (juce::ValueTree::fromXml (*xml));
}

juce::AudioProcessorEditor* AudioPluginAudioProcessor::createEditor() { return new AudioPluginAudioProcessorEditor (*this); }
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new AudioPluginAudioProcessor(); }
