#include "ComparisonBar.h"
#include "ParityLookAndFeel.h"

void ComparisonBar::paint (juce::Graphics& g)
{
    using LF = ParityLookAndFeel;

    const auto width = (float) getWidth();
    auto positionX = [width] (float norm) { return juce::jlimit (0.0f, width - 2.0f, norm * width); };

    // Track with accent fill from the anchor to the reference position.
    g.setColour (LF::line);
    g.fillRect (0.0f, 4.0f, width, 3.0f);

    if (refPosition >= 0.0f)
    {
        const auto anchorX = positionX (fillAnchor);
        const auto refX = positionX (refPosition);

        g.setColour (LF::accent.withAlpha (0.5f));
        g.fillRect (juce::jmin (anchorX, refX), 4.0f, std::abs (refX - anchorX), 3.0f);

        g.setColour (LF::accent);
        g.fillRect (refX, 0.0f, 2.0f, 10.0f);
    }

    if (mixPosition >= 0.0f)
    {
        g.setColour (LF::ink);
        g.fillRect (positionX (mixPosition), 0.0f, 2.0f, 10.0f);
    }

    // Scale captions.
    g.setColour (LF::inkFaint);
    g.setFont (ParityLookAndFeel::getFont (10.0f));
    const auto labelArea = juce::Rectangle<float> (0.0f, 11.0f, width, 12.0f);
    g.drawText (leftLabel, labelArea, juce::Justification::topLeft);
    g.drawText (centreLabel, labelArea, juce::Justification::centredTop);
    g.drawText (rightLabel, labelArea, juce::Justification::topRight);
}
