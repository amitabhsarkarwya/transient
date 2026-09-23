#pragma once

#include <juce_audio_basics/juce_audio_basics.h>
#include <algorithm>
#include <cmath>
#include <vector>

// Real-time expander DSP. The detector filters operate on a copy of the input;
// only the delayed program audio is changed by the computed gain.
class ExpanderDSP
{
public:
    enum class Mode { Expander = 0, Gate, Upward };

    void prepare (double newSampleRate, int)
    {
        sampleRate = newSampleRate > 0.0 ? newSampleRate : 44100.0;
        delayL.assign (maxDelaySamples + 1, 0.0f);
        delayR.assign (maxDelaySamples + 1, 0.0f);
        writePosition = 0;
        detectorEnvelope = 0.0f;
        gainDb = 0.0f;
        hpfInL = hpfInR = hpfOutL = hpfOutR = 0.0f;
        lpfOutL = lpfOutR = 0.0f;
        updateFilterCoefficients();
        detectorDb.store (-60.0f);
        gainReductionDb.store (0.0f);
    }

    void setParameters (Mode newMode, float newThresholdDb, float newRatio,
                        float newRangeDb, float newKneeDb, float attackMs,
                        float, float releaseMs, float newLookaheadMs,
                        float newHpfHz, float newLpfHz, bool newScListen,
                        bool newDeltaListen, float newMakeupDb, float newMix)
    {
        mode = newMode;
        thresholdDb = newThresholdDb;
        ratio = juce::jmax (1.0f, newRatio);
        rangeDb = juce::jmax (0.0f, newRangeDb);
        kneeDb = juce::jmax (0.0f, newKneeDb);
        attackCoeff = timeCoefficient (attackMs);
        releaseCoeff = timeCoefficient (releaseMs);
        lookaheadSamples = juce::jlimit (0, maxDelaySamples,
            static_cast<int> (juce::jlimit (0.0f, 20.0f, newLookaheadMs) * 0.001 * sampleRate));
        hpfHz = newHpfHz;
        lpfHz = newLpfHz;
        updateFilterCoefficients();
        sidechainListen = newScListen;
        deltaListen = newDeltaListen;
        makeupLinear = std::pow (10.0f, newMakeupDb / 20.0f);
        mix = juce::jlimit (0.0f, 1.0f, newMix);
    }

    int getLatencySamples() const noexcept { return lookaheadSamples; }
    float getDetectorDb() const noexcept { return detectorDb.load (std::memory_order_relaxed); }
    float getGainReductionDb() const noexcept { return gainReductionDb.load (std::memory_order_relaxed); }

    // Return static output level for the transfer curve (before gain smoothing).
    float calculateOutputDb (float inputDb) const noexcept
    {
        return inputDb + calculateGainDb (inputDb);
    }

    float calculateOutputDbWithMakeup (float inputDb) const noexcept
    {
        return calculateOutputDb (inputDb) + 20.0f * std::log10 (juce::jmax (makeupLinear, 1.0e-8f));
    }

    void processBlock (juce::AudioBuffer<float>& buffer)
    {
        const int channels = buffer.getNumChannels();
        const int samples = buffer.getNumSamples();
        if (channels == 0 || samples == 0)
            return;

        auto* left = buffer.getWritePointer (0);
        auto* right = channels > 1 ? buffer.getWritePointer (1) : left;
        float peakDb = -160.0f;
        float minGainDb = 0.0f;

        for (int i = 0; i < samples; ++i)
        {
            const float inL = left[i];
            const float inR = channels > 1 ? right[i] : inL;
            delayL[writePosition] = inL;
            delayR[writePosition] = inR;
            const int readPosition = (writePosition - lookaheadSamples + maxDelaySamples + 1)
                                   % (maxDelaySamples + 1);
            const float delayedL = delayL[readPosition];
            const float delayedR = delayR[readPosition];
            writePosition = (writePosition + 1) % (maxDelaySamples + 1);

            // High-pass then low-pass only the sidechain signal.
            const float scL = filterSidechain (inL, hpfInL, hpfOutL, lpfOutL);
            const float scR = channels > 1 ? filterSidechain (inR, hpfInR, hpfOutR, lpfOutR) : scL;
            const float detectorPeak = juce::jmax (std::abs (scL), std::abs (scR));

            // Attack on rising level, release on falling level.
            const float detectorCoeff = detectorPeak > detectorEnvelope ? attackCoeff : releaseCoeff;
            detectorEnvelope = detectorCoeff * detectorEnvelope + (1.0f - detectorCoeff) * detectorPeak;
            const float levelDb = detectorEnvelope > 1.0e-8f
                                ? 20.0f * std::log10 (detectorEnvelope) : -160.0f;

            // Smooth the gain in dB to avoid zippering while retaining the curve's shape.
            const float targetGainDb = calculateGainDb (levelDb);
            const float gainCoeff = targetGainDb > gainDb ? attackCoeff : releaseCoeff;
            gainDb = gainCoeff * gainDb + (1.0f - gainCoeff) * targetGainDb;
            const float linearGain = std::pow (10.0f, gainDb / 20.0f);

            float outL = delayedL * linearGain;
            float outR = delayedR * linearGain;
            if (sidechainListen)
            {
                outL = scL;
                outR = scR;
            }
            else if (deltaListen)
            {
                outL = delayedL - outL;
                outR = delayedR - outR;
            }
            else
            {
                outL *= makeupLinear;
                outR *= makeupLinear;
                outL = delayedL + mix * (outL - delayedL);
                outR = delayedR + mix * (outR - delayedR);
            }

            left[i] = outL;
            if (channels > 1)
                right[i] = outR;
            peakDb = juce::jmax (peakDb, levelDb);
            minGainDb = juce::jmin (minGainDb, gainDb);
        }

        detectorDb.store (peakDb, std::memory_order_relaxed);
        gainReductionDb.store (minGainDb, std::memory_order_relaxed);
    }

private:
    float timeCoefficient (float milliseconds) const noexcept
    {
        const float seconds = juce::jmax (0.00005f, milliseconds * 0.001f);
        return std::exp (-1.0f / static_cast<float> (sampleRate * seconds));
    }

    void updateFilterCoefficients() noexcept
    {
        const float nyquist = static_cast<float> (sampleRate * 0.49);
        const float hp = juce::jlimit (0.0f, nyquist, hpfHz);
        const float lp = juce::jlimit (0.0f, nyquist, lpfHz);
        const float dt = 1.0f / static_cast<float> (sampleRate);
        const float hpRc = hp > 0.0f ? 1.0f / (2.0f * juce::MathConstants<float>::pi * hp) : 0.0f;
        hpfAlpha = hp > 0.0f ? hpRc / (hpRc + dt) : 0.0f;
        lpfCoeff = lp > 0.0f ? 1.0f - std::exp (-2.0f * juce::MathConstants<float>::pi * lp * dt) : 1.0f;
    }

    float filterSidechain (float input, float& previousInput, float& previousHpf,
                           float& previousLpf) const noexcept
    {
        const float highPassed = hpfHz > 0.0f ? hpfAlpha * (previousHpf + input - previousInput) : input;
        previousInput = input;
        previousHpf = highPassed;
        previousLpf += lpfCoeff * (highPassed - previousLpf);
        return previousLpf;
    }

    float calculateGainDb (float inputDb) const noexcept
    {
        const float halfKnee = kneeDb * 0.5f;
        float gain = 0.0f;

        if (mode == Mode::Upward)
        {
            if (kneeDb <= 0.0f)
                gain = (inputDb > thresholdDb) ? (ratio - 1.0f) * (inputDb - thresholdDb) : 0.0f;
            else if (inputDb >= thresholdDb + halfKnee)
                gain = (ratio - 1.0f) * (inputDb - thresholdDb);
            else if (inputDb > thresholdDb - halfKnee)
            {
                const float d = inputDb - (thresholdDb - halfKnee);
                gain = (ratio - 1.0f) * d * d / (2.0f * kneeDb);
            }
            return juce::jlimit (0.0f, rangeDb, gain);
        }

        if (mode == Mode::Gate)
        {
            if (kneeDb <= 0.0f)
                return inputDb < thresholdDb ? -rangeDb : 0.0f;
            const float t = juce::jlimit (0.0f, 1.0f, (inputDb - (thresholdDb - halfKnee)) / kneeDb);
            const float smooth = t * t * (3.0f - 2.0f * t);
            return -rangeDb * (1.0f - smooth);
        }

        // Downward expander.
        if (kneeDb <= 0.0f)
            gain = inputDb < thresholdDb ? (ratio - 1.0f) * (inputDb - thresholdDb) : 0.0f;
        else if (inputDb <= thresholdDb - halfKnee)
            gain = (ratio - 1.0f) * (inputDb - thresholdDb);
        else if (inputDb < thresholdDb + halfKnee)
        {
            const float d = inputDb - (thresholdDb + halfKnee);
            gain = -(ratio - 1.0f) * d * d / (2.0f * kneeDb);
        }
        return juce::jmax (-rangeDb, gain);
    }

    static constexpr int maxDelaySamples = 3840; // 20 ms at 192 kHz
    double sampleRate = 44100.0;
    Mode mode = Mode::Expander;
    float thresholdDb = -24.0f, ratio = 3.0f, rangeDb = 30.0f, kneeDb = 4.0f;
    float attackCoeff = 0.99f, releaseCoeff = 0.999f;
    float detectorEnvelope = 0.0f, gainDb = 0.0f;
    float hpfHz = 80.0f, lpfHz = 20000.0f, hpfAlpha = 0.0f, lpfCoeff = 1.0f;
    float hpfInL = 0.0f, hpfInR = 0.0f, hpfOutL = 0.0f, hpfOutR = 0.0f;
    float lpfOutL = 0.0f, lpfOutR = 0.0f;
    bool sidechainListen = false, deltaListen = false;
    float makeupLinear = 1.0f, mix = 1.0f;
    int lookaheadSamples = 0, writePosition = 0;
    std::vector<float> delayL, delayR;
    std::atomic<float> detectorDb { -60.0f }, gainReductionDb { 0.0f };
};
