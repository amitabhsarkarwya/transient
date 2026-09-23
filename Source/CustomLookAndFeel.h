#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

class CustomLookAndFeel : public juce::LookAndFeel_V4
{
public:
    CustomLookAndFeel()
    {
        setColour (juce::ResizableWindow::backgroundColourId, juce::Colour (0xff173665));
        setColour (juce::Label::textColourId, juce::Colour (0xff17365e));
        setColour (juce::ComboBox::backgroundColourId, juce::Colour (0xffdcecff));
        setColour (juce::ComboBox::textColourId, juce::Colour (0xffffffff));
        setColour (juce::ComboBox::outlineColourId, juce::Colour (0xff5b82b4));
        setColour (juce::Slider::textBoxTextColourId, juce::Colour (0xffffffff));
        setColour (juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
        setColour (juce::Slider::rotarySliderFillColourId, juce::Colour (0xff168be1));
        setColour (juce::Slider::thumbColourId, juce::Colour (0xfffff9ff));
    }

    // 1. Monochromatic Rotary Slider (Attack & Release)
    void drawRotarySlider (juce::Graphics& g, int x, int y, int width, int height,
                           float sliderPosProportional, float rotaryStartAngle,
                           float rotaryEndAngle, juce::Slider& slider) override
    {
        auto bounds = juce::Rectangle<int> (x, y, width, height).toFloat().reduced (3.0f);
        auto radius = juce::jmin (bounds.getWidth(), bounds.getHeight()) / 2.0f;
        auto toAngle = rotaryStartAngle + sliderPosProportional * (rotaryEndAngle - rotaryStartAngle);
        auto centre = bounds.getCentre();
        auto lineW = juce::jlimit (2.5f, 4.5f, radius * 0.12f);
        auto arcRadius = radius - lineW * 1.5f;

        // Outer background track
        juce::Path bgArc;
        bgArc.addCentredArc (centre.x, centre.y, arcRadius, arcRadius, 0.0f, rotaryStartAngle, rotaryEndAngle, true);
        g.setColour (juce::Colour (0xff526d9b));
        g.strokePath (bgArc, juce::PathStrokeType (lineW, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

        // Neon cyan value arc with a soft magenta glow.
        if (slider.isEnabled() && sliderPosProportional > 0.001f)
        {
            juce::Path valArc;
            valArc.addCentredArc (centre.x, centre.y, arcRadius, arcRadius, 0.0f, rotaryStartAngle, toAngle, true);

            // Subtle glow
            g.setColour (juce::Colour (0x65ff54d8));
            g.strokePath (valArc, juce::PathStrokeType (lineW + 2.5f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

            // Sharp white stroke
            g.setColour (juce::Colour (0xff168be1));
            g.strokePath (valArc, juce::PathStrokeType (lineW, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
        }

        // Dial face body
        auto dialRadius = arcRadius - lineW * 1.4f;
        auto dialBounds = juce::Rectangle<float> (centre.x - dialRadius, centre.y - dialRadius, dialRadius * 2.0f, dialRadius * 2.0f);

        // Radial dial face gradient
        juce::ColourGradient dialGrad (juce::Colour (0xfff5faff), centre.x, centre.y - dialRadius,
                                      juce::Colour (0xff91acd0), centre.x, centre.y + dialRadius, false);
        g.setGradientFill (dialGrad);
        g.fillEllipse (dialBounds);

        // Bezel ring
        g.setColour (juce::Colour (0xfff9fcff));
        g.drawEllipse (dialBounds, 1.5f);
        g.setColour (juce::Colour (0xff5878a3));
        g.drawEllipse (dialBounds.reduced (2.0f), 1.0f);

        // White indicator needle
        juce::Path needle;
        auto needleLen = dialRadius * 0.65f;
        needle.addRoundedRectangle (-1.2f, -dialRadius + 2.0f, 2.4f, needleLen * 0.45f, 1.0f);
        needle.applyTransform (juce::AffineTransform::rotation (toAngle).translated (centre.x, centre.y));

        g.setColour (juce::Colour (0xff17365e));
        g.fillPath (needle);

        // Center dot
        g.setColour (juce::Colour (0xffff54d8));
        g.fillEllipse (centre.x - 3.0f, centre.y - 3.0f, 6.0f, 6.0f);
    }

    // 2. Monochromatic Linear Sliders (Horizontal Sliders & Vertical Faders)
    void drawLinearSlider (juce::Graphics& g, int x, int y, int width, int height,
                           float sliderPos, float /*minSliderPos*/, float /*maxSliderPos*/,
                           juce::Slider::SliderStyle style, juce::Slider& slider) override
    {
        auto bounds = juce::Rectangle<float> (static_cast<float> (x), static_cast<float> (y),
                                              static_cast<float> (width), static_cast<float> (height));

        if (style == juce::Slider::LinearVertical)
        {
            // Vertical Fader (Threshold / Gain)
            float trackW = 8.0f;
            float trackX = bounds.getCentreX() - trackW * 0.5f;
            auto trackArea = juce::Rectangle<float> (trackX, bounds.getY() + 10.0f, trackW, bounds.getHeight() - 20.0f);

            // Track groove
            g.setColour (juce::Colour (0xff141418));
            g.fillRoundedRectangle (trackArea, trackW * 0.5f);
            g.setColour (juce::Colour (0xff2b2b34));
            g.drawRoundedRectangle (trackArea, trackW * 0.5f, 1.0f);

            // Active fill from bottom to slider thumb
            float fillH = juce::jmax (0.0f, trackArea.getBottom() - sliderPos);
            auto fillRect = juce::Rectangle<float> (trackX, sliderPos, trackW, fillH);
            g.setColour (juce::Colour (0x66ffffff));
            g.fillRoundedRectangle (fillRect, trackW * 0.5f);

            // Console Fader Thumb Cap
            float thumbW = 32.0f;
            float thumbH = 18.0f;
            auto thumbBounds = juce::Rectangle<float> (bounds.getCentreX() - thumbW * 0.5f,
                                                       sliderPos - thumbH * 0.5f,
                                                       thumbW, thumbH);

            // Drop shadow
            g.setColour (juce::Colours::black.withAlpha (0.45f));
            g.fillRoundedRectangle (thumbBounds.translated (0.0f, 2.0f), 3.0f);

            // Cap gradient
            juce::ColourGradient capGrad (juce::Colour (0xff2e2e36), thumbBounds.getX(), thumbBounds.getY(),
                                          juce::Colour (0xff16161a), thumbBounds.getX(), thumbBounds.getBottom(), false);
            g.setGradientFill (capGrad);
            g.fillRoundedRectangle (thumbBounds, 3.0f);

            // Silver border
            g.setColour (juce::Colour (0xff484854));
            g.drawRoundedRectangle (thumbBounds, 3.0f, 1.0f);

            // Center white indicator line
            g.setColour (juce::Colours::white);
            g.fillRect (thumbBounds.getX() + 4.0f, thumbBounds.getCentreY() - 1.0f, thumbBounds.getWidth() - 8.0f, 2.0f);
        }
        else
        {
            // Horizontal Slider (Ratio, Knee, Range, Low Cut)
            float trackH = 5.0f;
            float trackY = bounds.getCentreY() - trackH * 0.5f;
            auto trackArea = juce::Rectangle<float> (bounds.getX() + 6.0f, trackY, bounds.getWidth() - 12.0f, trackH);

            // Track background
            g.setColour (juce::Colour (0xff18181c));
            g.fillRoundedRectangle (trackArea, trackH * 0.5f);
            g.setColour (juce::Colour (0xff2a2a32));
            g.drawRoundedRectangle (trackArea, trackH * 0.5f, 1.0f);

            // Active bar from left to thumb
            auto activeBar = juce::Rectangle<float> (trackArea.getX(), trackY,
                                                     juce::jmax (0.0f, sliderPos - trackArea.getX()), trackH);
            g.setColour (juce::Colour (0x99ffffff));
            g.fillRoundedRectangle (activeBar, trackH * 0.5f);

            // Modern pill thumb
            float thumbW = 12.0f;
            float thumbH = 16.0f;
            auto thumbBounds = juce::Rectangle<float> (sliderPos - thumbW * 0.5f, bounds.getCentreY() - thumbH * 0.5f, thumbW, thumbH);

            g.setColour (juce::Colours::black.withAlpha (0.4f));
            g.fillRoundedRectangle (thumbBounds.translated (0.0f, 1.0f), 3.0f);

            juce::ColourGradient thumbGrad (juce::Colour (0xff32323a), thumbBounds.getX(), thumbBounds.getY(),
                                           juce::Colour (0xff1c1c20), thumbBounds.getX(), thumbBounds.getBottom(), false);
            g.setGradientFill (thumbGrad);
            g.fillRoundedRectangle (thumbBounds, 3.0f);

            g.setColour (juce::Colour (0xff52525e));
            g.drawRoundedRectangle (thumbBounds, 3.0f, 1.0f);

            // White notch in thumb
            g.setColour (juce::Colours::white);
            g.fillRect (thumbBounds.getCentreX() - 1.0f, thumbBounds.getY() + 3.0f, 2.0f, thumbH - 6.0f);
        }
    }

    // 3. Monochromatic Pill Buttons (Modes & Toggles)
    void drawButtonBackground (juce::Graphics& g, juce::Button& button, const juce::Colour& /*backgroundColour*/,
                                bool shouldDrawButtonAsHighlighted, bool shouldDrawButtonAsDown) override
    {
        auto bounds = button.getLocalBounds().toFloat().reduced (1.0f);
        bool isToggled = button.getToggleState();

        if (isToggled)
        {
            // Active: Solid Pure White Pill
            g.setColour (juce::Colours::white);
            g.fillRoundedRectangle (bounds, bounds.getHeight() * 0.5f);
        }
        else
        {
            // Inactive: Dark Charcoal Pill with subtle outline
            juce::Colour bg = juce::Colour (0xff141418);
            if (shouldDrawButtonAsDown)
                bg = juce::Colour (0xff0a0a0d);
            else if (shouldDrawButtonAsHighlighted)
                bg = juce::Colour (0xff202026);

            g.setColour (bg);
            g.fillRoundedRectangle (bounds, bounds.getHeight() * 0.5f);

            g.setColour (shouldDrawButtonAsHighlighted ? juce::Colour (0xff4b4b58) : juce::Colour (0xff2c2c36));
            g.drawRoundedRectangle (bounds, bounds.getHeight() * 0.5f, 1.0f);
        }
    }

    void drawButtonText (juce::Graphics& g, juce::TextButton& button, bool isMouseOverButton, bool /*isButtonDown*/) override
    {
        bool isToggled = button.getToggleState();
        g.setFont (juce::FontOptions (10.5f).withStyle ("Bold"));

        juce::Colour textCol = isToggled ? juce::Colour (0xff0a0a0d)
                                         : (isMouseOverButton ? juce::Colours::white : juce::Colour (0xffa1a1aa));

        g.setColour (textCol);
        g.drawText (button.getButtonText(), button.getLocalBounds(), juce::Justification::centred);
    }
};
