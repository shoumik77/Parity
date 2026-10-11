#include "ComparisonBar.h"
#include "ParityLookAndFeel.h"

void ComparisonBar::paint (juce::Graphics& g)
{
    using LF = ParityLookAndFeel;

    const auto width = (float) getWidth();
    const auto track = juce::Rectangle<float> (0.0f, 2.0f, width, 6.0f);
    auto positionX = [width] (float norm) { return juce::jlimit (1.0f, width - 1.0f, norm * width); };

    LF::paintRecessed (g, track, 3.0f);

    // Accent fill from the anchor to the reference position.
    if (refPosition >= 0.0f)
    {
        const auto anchorX = positionX (fillAnchor);
        const auto refX = positionX (refPosition);

        g.setColour (LF::accent.withAlpha (0.55f));
        g.fillRoundedRectangle (juce::jmin (anchorX, refX), track.getY() + 1.5f,
                                juce::jmax (1.0f, std::abs (refX - anchorX)), track.getHeight() - 3.0f, 1.5f);
    }

    // Anchor tick.
    g.setColour (LF::line.brighter (0.4f));
    g.fillRect (positionX (fillAnchor) - 0.5f, track.getY() - 2.0f, 1.0f, track.getHeight() + 4.0f);

    auto drawMarker = [&] (float norm, juce::Colour colour)
    {
        const auto x = positionX (norm);
        const auto marker = juce::Rectangle<float> (4.0f, 12.0f).withCentre ({ x, track.getCentreY() });

        g.setColour (colour.withAlpha (0.35f));
        g.fillRoundedRectangle (marker.expanded (2.0f, 1.0f), 3.0f);
        g.setColour (colour);
        g.fillRoundedRectangle (marker, 1.5f);
    };

    if (refPosition >= 0.0f)
        drawMarker (refPosition, LF::accent);

    if (mixPosition >= 0.0f)
        drawMarker (mixPosition, LF::ink);

    // Scale captions.
    g.setColour (LF::inkDim);
    g.setFont (LF::getMonoFont (9.0f));
    const auto labelArea = juce::Rectangle<float> (0.0f, 12.0f, width, 12.0f);
    g.drawText (leftLabel, labelArea, juce::Justification::topLeft);
    g.drawText (centreLabel, labelArea, juce::Justification::centredTop);
    g.drawText (rightLabel, labelArea, juce::Justification::topRight);
}
