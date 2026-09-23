#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include "PluginProcessor.h"

class TransferCurveVisualizer : public juce::Component
{
public:
    explicit TransferCurveVisualizer (AudioPluginAudioProcessor& p)
        : processor (p)
    {
        setOpaque (true);
    }

    void updateData()
    {
        currentDetDb = processor.getInputPeakDb();
        currentGrDb = processor.getGainReductionDb();
        repaint();
    }

    void paint (juce::Graphics& g) override
    {
        auto bounds = getLocalBounds().toFloat();

        // Compact graph panel in the editor's slate palette.
        juce::ColourGradient bgGrad (juce::Colour (0xff101820), bounds.getX(), bounds.getY(),
                                     juce::Colour (0xff0d141a), bounds.getX(), bounds.getBottom(), false);
        g.setGradientFill (bgGrad);
        g.fillRoundedRectangle (bounds, 6.0f);

        // Subtle zinc border
        g.setColour (juce::Colour (0xff26343b));
        g.drawRoundedRectangle (bounds.reduced (0.5f), 6.0f, 1.0f);

        // Internal plot area (leave room for axis numbers)
        auto plotArea = bounds.reduced (16.0f, 10.0f);
        plotArea.removeFromBottom (10.0f);
        plotArea.removeFromLeft (10.0f);

        const float minDb = -50.0f;
        const float maxDb = 0.0f;

        auto dbToX = [&] (float db) -> float
        {
            float norm = juce::jlimit (0.0f, 1.0f, (db - minDb) / (maxDb - minDb));
            return plotArea.getX() + norm * plotArea.getWidth();
        };

        auto dbToY = [&] (float db) -> float
        {
            float norm = juce::jlimit (0.0f, 1.0f, (db - minDb) / (maxDb - minDb));
            return plotArea.getBottom() - norm * plotArea.getHeight();
        };

        // 2. Grid lines & Axis labels (-40, -30, -20, -10, 0 dB)
        const float gridDbs[] = { -40.0f, -20.0f, 0.0f };
        g.setFont (juce::FontOptions (8.0f));

        for (float db : gridDbs)
        {
            float x = dbToX (db);
            float y = dbToY (db);

            // Vertical grid line
            g.setColour (juce::Colour (0xff202d35));
            g.drawVerticalLine (static_cast<int> (x), plotArea.getY(), plotArea.getBottom());

            // Horizontal grid line
            g.drawHorizontalLine (static_cast<int> (y), plotArea.getX(), plotArea.getRight());

            // Axis labels (monochrome light grey)
            g.setColour (juce::Colour (0xff75868d));
            // X-axis label (bottom)
            g.drawText (juce::String (static_cast<int> (db)),
                        juce::Rectangle<float> (x - 14.0f, plotArea.getBottom() + 4.0f, 28.0f, 12.0f),
                        juce::Justification::centred);

            // Y-axis label (left)
            g.drawText (juce::String (static_cast<int> (db)),
                        juce::Rectangle<float> (plotArea.getX() - 22.0f, y - 6.0f, 18.0f, 12.0f),
                        juce::Justification::centredRight);
        }

        // Axis units ("dB")
        g.setColour (juce::Colour (0xff52525b));
        g.drawText ("dB", juce::Rectangle<float> (plotArea.getX() - 22.0f, plotArea.getY() - 14.0f, 18.0f, 12.0f), juce::Justification::centredRight);
        g.drawText ("dB", juce::Rectangle<float> (plotArea.getRight() + 4.0f, plotArea.getBottom() + 3.0f, 18.0f, 12.0f), juce::Justification::centredLeft);

        // 3. 1:1 Reference Diagonal Line (where In == Out)
        g.setColour (juce::Colour (0xff405159));
        const float dotLengths[2] = { 2.0f, 3.0f };
        g.drawDashedLine (juce::Line<float> (plotArea.getX(), plotArea.getBottom(),
                                            plotArea.getRight(), plotArea.getY()),
                          dotLengths, 2, 1.0f);

        // 4. Calculate Theoretical Transfer Function Points
        constexpr int numSteps = 120;
        float xStep = (maxDb - minDb) / static_cast<float> (numSteps);

        juce::Path curvePath;
        juce::Path fillAreaPath;

        fillAreaPath.startNewSubPath (plotArea.getX(), plotArea.getBottom());

        for (int i = 0; i <= numSteps; ++i)
        {
            float inDb = minDb + static_cast<float> (i) * xStep;
            float outDb = AudioPluginAudioProcessor::transferOutputDb (inDb, processor.getParameter ("THRESHOLD"), processor.getParameter ("RATIO"), processor.getParameter ("KNEE"));
            outDb = juce::jlimit (minDb, maxDb, outDb);

            float px = dbToX (inDb);
            float py = dbToY (outDb);

            if (i == 0)
                curvePath.startNewSubPath (px, py);
            else
                curvePath.lineTo (px, py);

            fillAreaPath.lineTo (px, py);
        }

        // Close fill area along the 1:1 diagonal back to start
        fillAreaPath.lineTo (plotArea.getRight(), plotArea.getY());
        fillAreaPath.lineTo (plotArea.getX(), plotArea.getBottom());
        fillAreaPath.closeSubPath();

        // Shaded Expansion Area (pure white translucent fill)
        juce::ColourGradient fillGrad (juce::Colour (0x2856dbc8), plotArea.getCentreX(), plotArea.getY(),
                                       juce::Colour (0x0456dbc8), plotArea.getCentreX(), plotArea.getBottom(), false);
        g.setGradientFill (fillGrad);
        g.fillPath (fillAreaPath);

        // 5. Draw Dynamic Transfer Curve (Luminous White)
        // Outer soft glow stroke
        g.setColour (juce::Colour (0x3556dbc8));
        g.strokePath (curvePath, juce::PathStrokeType (4.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

        // Sharp pure white stroke
        g.setColour (juce::Colour (0xff56dbc8));
        g.strokePath (curvePath, juce::PathStrokeType (2.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

        // 6. Threshold Marker & Guideline
        float thrVal = processor.getParameter ("THRESHOLD");
        float thrX = dbToX (thrVal);
        float thrY = dbToY (AudioPluginAudioProcessor::transferOutputDb (thrVal, processor.getParameter ("THRESHOLD"), processor.getParameter ("RATIO"), processor.getParameter ("KNEE")));

        // Vertical dashed line at threshold
        g.setColour (isDraggingThreshold ? juce::Colour (0xff56dbc8) : juce::Colour (0x9956dbc8));
        const float thrDash[2] = { 4.0f, 3.0f };
        g.drawDashedLine (juce::Line<float> (thrX, plotArea.getY(), thrX, plotArea.getBottom()),
                          thrDash, 2, 1.0f);

        // Threshold knee point dot on curve
        g.setColour (juce::Colour (0xff18181b));
        g.fillEllipse (thrX - 4.5f, thrY - 4.5f, 9.0f, 9.0f);
        g.setColour (juce::Colour (0xffdffaf5));
        g.drawEllipse (thrX - 4.5f, thrY - 4.5f, 9.0f, 9.0f, 1.5f);

        // 7. Live Dynamic Audio Tracking Puck (Bouncing Glow Ball)
        if (currentDetDb > minDb + 1.0f)
        {
            float ballInDb = juce::jlimit (minDb, maxDb, currentDetDb);
            float ballOutDb = juce::jlimit (minDb, maxDb, AudioPluginAudioProcessor::transferOutputDb (ballInDb, processor.getParameter ("THRESHOLD"), processor.getParameter ("RATIO"), processor.getParameter ("KNEE")));

            float bx = dbToX (ballInDb);
            float by = dbToY (ballOutDb);

            // Halo glow
            g.setColour (juce::Colour (0x3fffffff));
            g.fillEllipse (bx - 8.0f, by - 8.0f, 16.0f, 16.0f);

            // Solid center puck
            g.setColour (juce::Colours::white);
            g.fillEllipse (bx - 3.5f, by - 3.5f, 7.0f, 7.0f);
        }

        // 8. Live Gain Reduction Readout in top right
        g.setFont (juce::FontOptions (10.0f).withStyle ("Bold"));
        juce::String grStr;
        if (std::abs (currentGrDb) < 0.1f)
            grStr = "GR: 0.0 dB";
        else
            grStr = "GR: " + juce::String (currentGrDb, 1) + " dB";

        float grAlpha = juce::jlimit (0.4f, 1.0f, std::abs (currentGrDb) / 12.0f);
        g.setColour (juce::Colours::white.withAlpha (grAlpha));
        g.drawText (grStr, juce::Rectangle<float> (plotArea.getRight() - 90.0f, plotArea.getY() + 4.0f, 86.0f, 14.0f),
                    juce::Justification::centredRight);
    }

    void mouseDown (const juce::MouseEvent& e) override
    {
        auto plotArea = getLocalBounds().toFloat().reduced (16.0f, 10.0f);
        plotArea.removeFromBottom (10.0f);
        plotArea.removeFromLeft (10.0f);

        float thrVal = processor.getParameter ("THRESHOLD");
        float thrX = plotArea.getX() + juce::jlimit (0.0f, 1.0f, (thrVal - (-50.0f)) / 50.0f) * plotArea.getWidth();

        if (std::abs (static_cast<float> (e.x) - thrX) < 14.0f || plotArea.contains (e.position))
        {
            isDraggingThreshold = true;
            updateThresholdFromMouse (e.x, plotArea);
        }
    }

    void mouseDrag (const juce::MouseEvent& e) override
    {
        if (isDraggingThreshold)
        {
            auto plotArea = getLocalBounds().toFloat().reduced (16.0f, 10.0f);
            plotArea.removeFromBottom (10.0f);
            plotArea.removeFromLeft (10.0f);
            updateThresholdFromMouse (e.x, plotArea);
        }
    }

    void mouseUp (const juce::MouseEvent&) override
    {
        isDraggingThreshold = false;
        repaint();
    }

private:
    void updateThresholdFromMouse (int mouseX, const juce::Rectangle<float>& plotArea)
    {
        float norm = juce::jlimit (0.0f, 1.0f, (static_cast<float> (mouseX) - plotArea.getX()) / plotArea.getWidth());
        float db = -50.0f + norm * 50.0f;

        auto* param = processor.apvts.getParameter ("THRESHOLD");
        if (param != nullptr)
        {
            float paramNorm = param->getNormalisableRange().convertTo0to1 (juce::jlimit (-60.0f, 0.0f, db));
            param->setValueNotifyingHost (paramNorm);
        }
        repaint();
    }

    AudioPluginAudioProcessor& processor;
    float currentDetDb = -60.0f;
    float currentGrDb = 0.0f;
    bool isDraggingThreshold = false;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (TransferCurveVisualizer)
};
