#include "VerticalFader.h"
#include "ParityLookAndFeel.h"

namespace
{
    constexpr float capWidth = 22.0f, capHeight = 30.0f;
    constexpr float fallDbPerSecond = 24.0f;
    constexpr int frameRateHz = 30;
    constexpr juce::uint32 peakHoldMs = 1500;
}

VerticalFader::VerticalFader (bool showMeter, int meterWidth)
    : meterVisible (showMeter), meterBarWidth (meterWidth)
{
    setSliderStyle (juce::Slider::LinearVertical);
    setTextBoxStyle (juce::Slider::NoTextBox, true, 0, 0);
    setPopupDisplayEnabled (false, false, nullptr);
    setMouseCursor (juce::MouseCursor::UpDownResizeCursor);

    // Shift switches to velocity mode for fine adjustment; default double-click
    // return value is filled in by the editor once the parameter is attached.
    setVelocityModeParameters (0.6, 1, 0.08, false, juce::ModifierKeys::shiftModifier);

    if (meterVisible)
        startTimerHz (frameRateHz);
}

void VerticalFader::setMeterLevel (float linearLevel)
{
    targetDb = juce::Decibels::gainToDecibels (linearLevel, -100.0f);

    const auto now = juce::Time::getMillisecondCounter();

    if (targetDb >= peakHoldDb || now > peakHoldUntil)
    {
        peakHoldDb = targetDb;
        peakHoldUntil = now + peakHoldMs;
    }
}

void VerticalFader::setProcessingActive (bool shouldBeActive)
{
    if (processingActive != shouldBeActive)
    {
        processingActive = shouldBeActive;
        repaint();
    }
}

void VerticalFader::timerCallback()
{
    // Instant rise, timed fall - the classic meter ballistics people expect.
    if (targetDb > displayedDb)
        displayedDb = targetDb;
    else
        displayedDb = juce::jmax (targetDb, displayedDb - fallDbPerSecond / (float) frameRateHz);

    if (juce::Time::getMillisecondCounter() > peakHoldUntil)
        peakHoldDb = juce::jmax (displayedDb, peakHoldDb - fallDbPerSecond / (float) frameRateHz);

    repaint();
}

void VerticalFader::mouseEnter (const juce::MouseEvent& e)
{
    hovered = true;
    repaint();
    juce::Slider::mouseEnter (e);
}

void VerticalFader::mouseExit (const juce::MouseEvent& e)
{
    hovered = false;
    repaint();
    juce::Slider::mouseExit (e);
}

float VerticalFader::dbToY (float db, float height) const noexcept
{
    const auto norm = juce::jlimit (0.0f, 1.0f, (float) ((db - getMinimum()) / (getMaximum() - getMinimum())));
    return height - norm * height;
}

//==============================================================================
void VerticalFader::paint (juce::Graphics& g)
{
    using LF = ParityLookAndFeel;

    auto bounds = getLocalBounds().toFloat();
    const auto height = bounds.getHeight();

    // Layout: meter well | 4 px gap | 6 px track centred in the rest, ticks to its right.
    auto meterArea = meterVisible ? bounds.removeFromLeft ((float) meterBarWidth)
                                  : juce::Rectangle<float>();

    if (meterVisible)
        bounds.removeFromLeft (4.0f);

    const auto trackX = bounds.getX() + 8.0f; // leaves room for the cap on the left, ticks on the right
    const auto trackArea = juce::Rectangle<float> (trackX, 0.0f, 6.0f, height);

    const auto norm = (float) valueToProportionOfLength (getValue());
    const auto handleY = juce::jlimit (capHeight * 0.5f, height - capHeight * 0.5f, height * (1.0f - norm));

    //==========================================================================
    if (meterVisible)
    {
        LF::paintRecessed (g, meterArea, 3.0f);

        const auto inner = meterArea.reduced (1.5f);
        const auto fillTop = inner.getY() + dbToY (displayedDb, inner.getHeight());
        const auto thresholdY = inner.getY() + dbToY ((float) getValue(), inner.getHeight());

        if (fillTop < inner.getBottom() - 0.5f)
        {
            // Below the threshold: the signal that passes through untouched.
            const auto passing = inner.withTop (juce::jmax (fillTop, thresholdY));

            juce::ColourGradient gradient (LF::accent, 0.0f, inner.getBottom(),
                                           LF::meterTop, 0.0f, inner.getY(), false);
            gradient.addColour (0.6, LF::meterMid);
            g.setGradientFill (gradient);
            g.fillRect (passing);

            // Soft glow at the top of the lit column.
            g.setColour ((fillTop < thresholdY ? LF::warn : LF::meterTop).withAlpha (0.35f));
            g.fillRect (inner.getX(), fillTop - 1.0f, inner.getWidth(), 2.0f);

            // Above the threshold: the part being clipped / limited away.
            if (fillTop < thresholdY)
            {
                g.setColour (processingActive ? LF::warn : LF::inkFaint.withAlpha (0.45f));
                g.fillRect (inner.withTop (fillTop).withBottom (thresholdY));
            }
        }

        // Peak-hold tick.
        if (peakHoldDb > (float) getMinimum())
        {
            const auto peakY = inner.getY() + dbToY (peakHoldDb, inner.getHeight());
            g.setColour (peakHoldDb > (float) getValue() && processingActive ? LF::warn : LF::ink.withAlpha (0.85f));
            g.fillRect (inner.getX(), peakY - 0.5f, inner.getWidth(), 1.0f);
        }

        // Hot cap when the input exceeds the top of the scale.
        if (displayedDb > (float) getMaximum())
        {
            g.setColour (LF::meterHot);
            g.fillRoundedRectangle (inner.withHeight (4.0f), 1.5f);
        }

        // Threshold line across the meter so the cut-off point is explicit.
        g.setColour (processingActive ? LF::warn.withAlpha (0.9f) : LF::inkFaint.withAlpha (0.5f));
        g.fillRect (meterArea.getX() - 1.0f, thresholdY - 0.5f, meterArea.getWidth() + 2.0f, 1.0f);
    }

    //==========================================================================
    // Labelled dB ticks to the right of the track.
    const auto range = getMaximum() - getMinimum();
    const auto step = range <= 12.0 ? 3.0 : range <= 24.0 ? 6.0 : range <= 100.0 ? 25.0 : 250.0;
    const auto isDb = range <= 100.0;
    g.setFont (LF::getMonoFont (7.5f));

    for (auto v = std::ceil (getMinimum() / step) * step; v <= getMaximum() + 1e-6; v += step)
    {
        const auto y = height - (float) valueToProportionOfLength (v) * height;
        const auto major = std::fmod (std::abs (v), step * 2.0) < 1e-6 || v == 0.0;

        g.setColour (v == 0.0 && isDb ? LF::inkFaint.withAlpha (0.8f) : LF::line.brighter (major ? 0.25f : 0.0f));
        g.fillRect (trackArea.getRight() + 3.0f, juce::jlimit (0.0f, height - 1.0f, y), major ? 6.0f : 3.0f, 1.0f);

        if (major)
        {
            g.setColour (LF::inkDim);
            g.drawText (juce::String ((int) v), (int) trackArea.getRight() + 11, (int) y - 5, 24, 10,
                        juce::Justification::centredLeft);
        }
    }

    // Track: recessed well with accent fill from the cap down.
    LF::paintRecessed (g, trackArea, 3.0f);

    const auto fill = trackArea.reduced (1.5f).withTop (handleY);
    juce::ColourGradient trackFill (LF::accent.withAlpha (isEnabled() ? 0.95f : 0.4f), 0.0f, handleY,
                                    LF::accent.withAlpha (isEnabled() ? 0.55f : 0.25f), 0.0f, height, false);
    g.setGradientFill (trackFill);
    g.fillRoundedRectangle (fill, 1.5f);

    //==========================================================================
    // Cap: raised block with grip lines and an accent indicator.
    const auto cap = juce::Rectangle<float> (capWidth, capHeight).withCentre ({ trackArea.getCentreX(), handleY });

    juce::Path capShape;
    capShape.addRoundedRectangle (cap, 4.0f);
    juce::DropShadow (juce::Colours::black.withAlpha (0.55f), 6, { 0, 2 }).drawForPath (g, capShape);

    LF::paintRaised (g, cap, 4.0f, hovered, isMouseButtonDown(), hasKeyboardFocus (true));

    for (int lineIndex = -1; lineIndex <= 1; ++lineIndex)
    {
        const auto y = cap.getCentreY() + (float) lineIndex * 5.0f;
        g.setColour (juce::Colours::black.withAlpha (0.55f));
        g.fillRect (cap.getX() + 5.0f, y, cap.getWidth() - 10.0f, 1.0f);
        g.setColour (juce::Colours::white.withAlpha (0.08f));
        g.fillRect (cap.getX() + 5.0f, y + 1.0f, cap.getWidth() - 10.0f, 1.0f);
    }

    g.setColour (isEnabled() ? LF::accent : LF::inkDim);
    g.fillRect (cap.getX() + 3.0f, cap.getCentreY() - 0.5f, cap.getWidth() - 6.0f, 1.0f);
}
