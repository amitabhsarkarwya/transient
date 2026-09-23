#include "PluginEditor.h"

namespace
{
constexpr auto teal = 0xff75eaff;
constexpr auto magenta = 0xffff54d8;
constexpr auto muted = 0xffb4c9e6;

juce::String signedDb (double value)
{
    return juce::String (value > 0.0 ? "+" : "") + juce::String (value, 1) + " dB";
}
}

void EnvelopeDisplay::paint (juce::Graphics& g)
{
    auto bounds = getLocalBounds().toFloat();
    juce::ColourGradient panel (juce::Colour (0xff152d58), bounds.getX(), bounds.getY(),
                                juce::Colour (0xff08172f), bounds.getX(), bounds.getBottom(), false);
    g.setGradientFill (panel);
    g.fillRoundedRectangle (bounds, 9.0f);
    g.setColour (juce::Colour (0xff89b7ed));
    g.drawRoundedRectangle (bounds.reduced (0.5f), 9.0f, 1.0f);
    g.setColour (juce::Colour (0xff243d6b));
    g.drawRoundedRectangle (bounds.reduced (2.0f), 8.0f, 1.0f);
    // Symmetrical cybersigil accents decorate the bezel, clear of the graph area.
    for (const bool rightSide : { false, true })
    {
        const float cx = rightSide ? bounds.getRight() - 11.0f : bounds.getX() + 11.0f;
        const float cy = bounds.getY() + 28.0f;
        const float dir = rightSide ? -1.0f : 1.0f;
        juce::Path glyph;
        glyph.startNewSubPath (cx, cy - 8.0f);
        glyph.lineTo (cx + dir * 4.0f, cy - 2.0f);
        glyph.lineTo (cx + dir * 9.0f, cy - 6.0f);
        glyph.lineTo (cx + dir * 6.0f, cy + 1.0f);
        glyph.lineTo (cx + dir * 10.0f, cy + 6.0f);
        glyph.lineTo (cx, cy + 4.0f);
        glyph.lineTo (cx - dir * 10.0f, cy + 6.0f);
        glyph.lineTo (cx - dir * 6.0f, cy + 1.0f);
        glyph.lineTo (cx - dir * 9.0f, cy - 6.0f);
        glyph.lineTo (cx - dir * 4.0f, cy - 2.0f);
        glyph.closeSubPath();
        g.setColour (juce::Colour (0xffb332aa));
        g.strokePath (glyph, juce::PathStrokeType (1.2f));
    }
    auto plot = bounds.reduced (24.0f, 20.0f);
    plot.removeFromBottom (14.0f);

    // Reference grid makes the attack front and sustain tail easy to read.
    for (int i = 0; i <= 4; ++i)
    {
        const float y = plot.getBottom() - plot.getHeight() * static_cast<float> (i) / 4.0f;
        g.setColour (i == 0 ? juce::Colour (0xff5988bf) : juce::Colour (0xff1d3458));
        g.drawHorizontalLine (static_cast<int> (y), plot.getX(), plot.getRight());
    }
    for (int i = 0; i <= 4; ++i)
    {
        const float x = plot.getX() + plot.getWidth() * static_cast<float> (i) / 4.0f;
        g.setColour (juce::Colour (0xff1b3153));
        g.drawVerticalLine (static_cast<int> (x), plot.getY(), plot.getBottom());
    }

    const float attack = processor.getParameter ("ATTACK");
    const float sustain = processor.getParameter ("SUSTAIN");
    const float sensitivity = processor.getParameter ("SENSITIVITY");
    const float attackHeight = juce::jlimit (0.45f, 0.95f, 0.70f + attack / 48.0f);
    const float sustainHeight = juce::jlimit (0.14f, 0.65f, 0.38f + sustain / 40.0f);
    const float riseTime = juce::jlimit (0.035f, 0.16f, 0.11f / std::sqrt (sensitivity));

    juce::Path shape;
    shape.startNewSubPath (plot.getX(), plot.getBottom());
    for (int i = 0; i <= 160; ++i)
    {
        const float t = static_cast<float> (i) / 160.0f;
        const float env = t < riseTime
            ? attackHeight * std::sin (juce::MathConstants<float>::halfPi * t / riseTime)
            : sustainHeight + (attackHeight - sustainHeight) * std::exp (-(t - riseTime) * 7.0f);
        const float x = plot.getX() + t * plot.getWidth();
        const float y = plot.getBottom() - env * plot.getHeight();
        shape.lineTo (x, y);
    }
    shape.lineTo (plot.getRight(), plot.getBottom());
    shape.closeSubPath();
    juce::ColourGradient fill (juce::Colour (0x66ff54d8), plot.getX(), plot.getY(),
                               juce::Colour (0x0875eaff), plot.getRight(), plot.getBottom(), false);
    g.setGradientFill (fill);
    g.fillPath (shape);

    juce::Path outline;
    for (int i = 0; i <= 160; ++i)
    {
        const float t = static_cast<float> (i) / 160.0f;
        const float env = t < riseTime
            ? attackHeight * std::sin (juce::MathConstants<float>::halfPi * t / riseTime)
            : sustainHeight + (attackHeight - sustainHeight) * std::exp (-(t - riseTime) * 7.0f);
        const juce::Point<float> point (plot.getX() + t * plot.getWidth(), plot.getBottom() - env * plot.getHeight());
        if (i == 0) outline.startNewSubPath (point); else outline.lineTo (point);
    }
    g.setColour (juce::Colour (0x65ff54d8));
    g.strokePath (outline, juce::PathStrokeType (7.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    g.setColour (juce::Colour (teal));
    g.strokePath (outline, juce::PathStrokeType (2.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

    const float activity = processor.getAttackActivity();
    const float markerX = plot.getX() + 0.12f * plot.getWidth();
    const float markerY = plot.getBottom() - (attackHeight * (0.6f + activity * 0.4f)) * plot.getHeight();
    g.setColour (juce::Colour (0x7054f2ff));
    g.fillEllipse (markerX - 8.0f, markerY - 8.0f, 16.0f, 16.0f);
    g.setColour (juce::Colour (0xfffff5fe));
    g.fillEllipse (markerX - 3.0f, markerY - 3.0f, 6.0f, 6.0f);
    g.setColour (juce::Colour (muted));
    g.setFont (juce::FontOptions (9.0f));
    g.drawText ("ATTACK", static_cast<int> (plot.getX()), static_cast<int> (plot.getBottom() + 2), 60, 12, juce::Justification::centredLeft);
    g.drawText ("TIME  >>", static_cast<int> (plot.getCentreX() - 40), static_cast<int> (plot.getBottom() + 2), 80, 12, juce::Justification::centred);
    g.drawText ("SUSTAIN", static_cast<int> (plot.getRight() - 60), static_cast<int> (plot.getBottom() + 2), 60, 12, juce::Justification::centredRight);
}

AudioPluginAudioProcessorEditor::AudioPluginAudioProcessorEditor (AudioPluginAudioProcessor& p)
    : AudioProcessorEditor (&p), processor (p), envelopeDisplay (p)
{
    setLookAndFeel (&lookAndFeel);
    setSize (650, 420);
    setResizable (false, false);
    setupKnob (attackSlider, attackLabel, "ATTACK");
    setupKnob (sustainSlider, sustainLabel, "SUSTAIN");
    setupKnob (sensitivitySlider, sensitivityLabel, "SENSITIVITY");
    setupKnob (outputSlider, outputLabel, "OUTPUT");
    attackSlider.textFromValueFunction = [] (double v) { return signedDb (v); };
    sustainSlider.textFromValueFunction = [] (double v) { return signedDb (v); };
    outputSlider.textFromValueFunction = [] (double v) { return signedDb (v); };
    sensitivitySlider.textFromValueFunction = [] (double v) { return juce::String (v, 2) + " x"; };

    attackAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (processor.apvts, "ATTACK", attackSlider);
    sustainAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (processor.apvts, "SUSTAIN", sustainSlider);
    sensitivityAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (processor.apvts, "SENSITIVITY", sensitivitySlider);
    outputAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (processor.apvts, "OUTPUT", outputSlider);

    title.setText ("TRANSIENT", juce::dontSendNotification);
    title.setFont (juce::FontOptions (17.0f).withStyle ("Bold"));
    title.setColour (juce::Label::textColourId, juce::Colour (0xfff5f3ff));
    addAndMakeVisible (title);
    subtitle.setText ("//  SHAPER  v1.0", juce::dontSendNotification);
    subtitle.setFont (juce::FontOptions (9.0f).withStyle ("Bold"));
    subtitle.setColour (juce::Label::textColourId, juce::Colour (teal));
    addAndMakeVisible (subtitle);
    addAndMakeVisible (envelopeDisplay);

    for (auto* meter : { &inputMeter, &outputMeter })
    {
        meter->setJustificationType (juce::Justification::centredRight);
        meter->setFont (juce::FontOptions (10.0f));
        meter->setColour (juce::Label::textColourId, juce::Colour (0xfff3f8ff));
        addAndMakeVisible (*meter);
    }
    startTimerHz (24);
}

AudioPluginAudioProcessorEditor::~AudioPluginAudioProcessorEditor()
{
    stopTimer();
    setLookAndFeel (nullptr);
}

void AudioPluginAudioProcessorEditor::setupKnob (juce::Slider& slider, juce::Label& label, const juce::String& name)
{
    slider.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    slider.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 94, 22);
    slider.setNumDecimalPlacesToDisplay (1);
    slider.setColour (juce::Slider::rotarySliderFillColourId, juce::Colour (teal));
    slider.setColour (juce::Slider::thumbColourId, juce::Colour (0xffeaf6f5));
    slider.setColour (juce::Slider::textBoxTextColourId, juce::Colour (0xfff3f4ff));
    slider.setColour (juce::Slider::textBoxBackgroundColourId, juce::Colour (0xff0a0d1a));
    slider.setColour (juce::Slider::textBoxOutlineColourId, juce::Colour (0xff3a4670));
    addAndMakeVisible (slider);
    label.setText (name, juce::dontSendNotification);
    label.setFont (juce::FontOptions (9.0f).withStyle ("Bold"));
    label.setColour (juce::Label::textColourId, juce::Colour (0xff19365e));
    label.setJustificationType (juce::Justification::centred);
    addAndMakeVisible (label);
}

void AudioPluginAudioProcessorEditor::timerCallback()
{
    envelopeDisplay.repaint();
    const auto dbText = [] (float db) { return db <= -90.0f ? juce::String ("-90.0 dB") : juce::String (db, 1) + " dB"; };
    inputMeter.setText ("IN  " + dbText (processor.getInputPeakDb()), juce::dontSendNotification);
    outputMeter.setText ("OUT  " + dbText (processor.getOutputPeakDb()), juce::dontSendNotification);
}

void AudioPluginAudioProcessorEditor::paint (juce::Graphics& g)
{
    g.fillAll (juce::Colour (0xff14284b));
    juce::ColourGradient header (juce::Colour (0xff70b5ff), 0.0f, 0.0f,
                                 juce::Colour (0xff1555a9), 0.0f, 54.0f, false);
    g.setGradientFill (header);
    g.fillRect (0, 0, getWidth(), 54);
    g.setColour (juce::Colour (0xffabd8ff));
    g.drawHorizontalLine (1, 0.0f, static_cast<float> (getWidth()));
    g.setColour (juce::Colour (0xff0c3d83));
    g.drawHorizontalLine (53, 0.0f, static_cast<float> (getWidth()));
    g.setColour (juce::Colour (teal));
    g.fillRoundedRectangle (18.0f, 14.0f, 3.0f, 26.0f, 1.5f);
    g.setColour (juce::Colour (magenta));
    g.fillRoundedRectangle (23.0f, 14.0f, 1.0f, 26.0f, 0.5f);

    // Bevelled XP-like silver-blue panels.
    const auto drawPanel = [&g] (juce::Rectangle<float> r)
    {
        juce::ColourGradient gradient (juce::Colour (0xffdcecff), r.getX(), r.getY(),
                                       juce::Colour (0xff7695bc), r.getX(), r.getBottom(), false);
        g.setGradientFill (gradient);
        g.fillRoundedRectangle (r, 9.0f);
        g.setColour (juce::Colour (0xffeff7ff));
        g.drawRoundedRectangle (r.reduced (0.5f), 9.0f, 1.0f);
        g.setColour (juce::Colour (0xff385b86));
        g.drawRoundedRectangle (r.reduced (2.0f), 7.0f, 1.0f);
        g.setColour (juce::Colour (0x9955a8ff));
        g.drawHorizontalLine (static_cast<int> (r.getY() + 2.0f), r.getX() + 14.0f, r.getRight() - 14.0f);
    };
    drawPanel ({ 16, 64, 618, 184 });
    drawPanel ({ 16, 258, 618, 146 });
    g.setColour (juce::Colour (0xff19385f));
    g.setFont (juce::FontOptions (9.0f).withStyle ("Bold"));
    g.drawText ("// ENVELOPE MATRIX", 30, 70, 190, 14, juce::Justification::centredLeft);
    g.setColour (juce::Colour (magenta));
    g.fillEllipse (499.0f, 74.0f, 5.0f, 5.0f);
    g.drawText ("LIVE DETECTION", 508, 70, 110, 14, juce::Justification::centredRight);
    g.setColour (juce::Colour (0xff19385f));
    g.drawText ("// SHAPE CONTROLS", 30, 265, 180, 14, juce::Justification::centredLeft);
    for (int i = 0; i < 4; ++i)
    {
        const float x = 24.0f + static_cast<float> (i) * 153.5f;
        juce::ColourGradient cell (juce::Colour (0xffd8e9ff), x, 281.0f,
                                   juce::Colour (0xff8ba8cc), x, 394.0f, false);
        g.setGradientFill (cell);
        g.fillRoundedRectangle (juce::Rectangle<float> (x, 280.0f, 145.0f, 114.0f), 7.0f);
        g.setColour (juce::Colour (0xfff5faff));
        g.drawRoundedRectangle (juce::Rectangle<float> (x, 280.0f, 145.0f, 114.0f), 7.0f, 1.0f);
        g.setColour (juce::Colour (0xff526f97));
        g.drawRoundedRectangle (juce::Rectangle<float> (x + 2.0f, 282.0f, 141.0f, 110.0f), 6.0f, 1.0f);
        g.setColour (juce::Colour (i < 2 ? 0xff1789de : 0xffcd32ad));
        g.fillRect (juce::Rectangle<float> (x + 11.0f, 281.0f, 25.0f, 1.5f));
    }

}

void AudioPluginAudioProcessorEditor::resized()
{
    title.setBounds (30, 8, 124, 26);
    subtitle.setBounds (158, 10, 100, 22);
    inputMeter.setBounds (430, 10, 88, 22);
    outputMeter.setBounds (526, 10, 100, 22);
    envelopeDisplay.setBounds (28, 88, 594, 148);

    attackLabel.setBounds (28, 281, 132, 16);
    attackSlider.setBounds (32, 293, 124, 102);
    sustainLabel.setBounds (183, 281, 132, 16);
    sustainSlider.setBounds (187, 293, 124, 102);
    sensitivityLabel.setBounds (338, 281, 132, 16);
    sensitivitySlider.setBounds (352, 293, 104, 102);
    outputLabel.setBounds (493, 281, 132, 16);
    outputSlider.setBounds (507, 293, 104, 102);
}
