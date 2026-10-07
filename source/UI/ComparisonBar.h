#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

//==============================================================================
/**
    Horizontal comparison scale (Figma "Correlation/Width comparison"): a
    hairline track with an accent fill from an anchor point to the reference
    position, tick markers for mix (pale) and reference (orange), and scale
    captions underneath.
*/
class ComparisonBar final : public juce::Component,
                            public juce::SettableTooltipClient
{
public:
    ComparisonBar() { setInterceptsMouseClicks (false, false); }

    /** Scale captions, left / centre / right. */
    void setScaleLabels (juce::String left, juce::String centre, juce::String right)
    {
        leftLabel = std::move (left);
        centreLabel = std::move (centre);
        rightLabel = std::move (right);
    }

    /** The normalised position (0..1) the accent fill grows from. */
    void setFillAnchor (float anchorNorm)  { fillAnchor = anchorNorm; }

    /** Normalised 0..1 positions; pass a negative value to hide a marker. */
    void setValues (float mixNorm, float refNorm)
    {
        if (std::abs (mixNorm - mixPosition) > 0.002f || std::abs (refNorm - refPosition) > 0.002f)
        {
            mixPosition = mixNorm;
            refPosition = refNorm;
            repaint();
        }
    }

    void paint (juce::Graphics&) override;

private:
    juce::String leftLabel, centreLabel, rightLabel;
    float fillAnchor = 0.0f;
    float mixPosition = -1.0f, refPosition = -1.0f;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ComparisonBar)
};
