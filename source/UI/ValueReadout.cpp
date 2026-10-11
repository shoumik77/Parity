#include "ValueReadout.h"
#include "ParityLookAndFeel.h"

ValueReadout::ValueReadout (juce::Slider& sliderToControl, juce::String unitText,
                            juce::String captionText, int numDecimals)
    : slider (sliderToControl), unit (std::move (unitText)), caption (std::move (captionText)),
      decimals (numDecimals)
{
    slider.addListener (this);
    setMouseCursor (juce::MouseCursor::UpDownResizeCursor);

    editor.setEditable (false, false, true);
    editor.setJustificationType (juce::Justification::centred);
    editor.setFont (ParityLookAndFeel::getMonoFont (14.0f, true));
    editor.setColour (juce::Label::backgroundColourId, juce::Colours::transparentBlack);
    editor.setColour (juce::Label::outlineWhenEditingColourId, juce::Colours::transparentBlack);
    editor.addListener (this);
    addChildComponent (editor);
}

ValueReadout::~ValueReadout()
{
    slider.removeListener (this);
}

void ValueReadout::setSectionActive (bool shouldBeActive)
{
    if (sectionActive != shouldBeActive)
    {
        sectionActive = shouldBeActive;
        repaint();
    }
}

juce::String ValueReadout::formatValue() const
{
    const auto v = slider.getValue();

    if (v >= 100.0)
        return juce::String ((int) std::round (v));

    auto text = juce::String (v, decimals);

    // Explicit sign for bipolar ranges so "+3.0" and "-3.0" line up.
    if (slider.getMinimum() < 0.0 && slider.getMaximum() > 0.0 && v > 0.0)
        text = "+" + text;

    return text;
}

juce::Rectangle<float> ValueReadout::chipBounds() const
{
    return getLocalBounds().toFloat().withTrimmedBottom (16.0f);
}

void ValueReadout::resized()
{
    editor.setBounds (chipBounds().toNearestInt());
}

//==============================================================================
void ValueReadout::paint (juce::Graphics& g)
{
    using LF = ParityLookAndFeel;

    const auto chip = chipBounds();
    LF::paintRecessed (g, chip, 4.0f, hovered || dragging ? LF::well.brighter (0.03f) : LF::well);

    if (hovered || dragging)
    {
        g.setColour (LF::accent.withAlpha (dragging ? 0.9f : 0.45f));
        g.drawRoundedRectangle (chip.reduced (0.5f), 4.0f, 1.0f);
    }

    const auto inkColour = sectionActive ? LF::ink : LF::inkDim;

    if (! editor.isVisible())
    {
        const auto valueFont = LF::getMonoFont (14.0f, true);
        const auto unitFont = LF::getFont (9.0f);
        const auto value = formatValue();
        const auto valueWidth = (float) juce::GlyphArrangement::getStringWidthInt (valueFont, value);
        const auto unitWidth = (float) juce::GlyphArrangement::getStringWidthInt (unitFont, unit);
        const auto total = valueWidth + 3.0f + unitWidth;

        auto text = chip.withSizeKeepingCentre (total, chip.getHeight());

        g.setFont (valueFont);
        g.setColour (inkColour);
        g.drawText (value, text.removeFromLeft (valueWidth), juce::Justification::centred);

        text.removeFromLeft (3.0f);
        g.setFont (unitFont);
        g.setColour (sectionActive ? LF::inkFaint : LF::inkMuted);
        g.drawText (unit, text.withTrimmedTop (3.0f), juce::Justification::centredLeft);
    }

    g.setFont (LF::getLabelFont (9.0f));
    g.setColour (sectionActive ? LF::inkFaint : LF::inkMuted);
    g.drawText (caption, getLocalBounds().toFloat().removeFromBottom (14.0f), juce::Justification::centred);
}

//==============================================================================
void ValueReadout::mouseEnter (const juce::MouseEvent&)  { hovered = true;  repaint(); }
void ValueReadout::mouseExit  (const juce::MouseEvent&)  { hovered = false; repaint(); }

void ValueReadout::mouseDown (const juce::MouseEvent& e)
{
    dragging = false;
    dragStart = e.position;
    dragStartValue = slider.getValue();
}

void ValueReadout::mouseDrag (const juce::MouseEvent& e)
{
    const auto dy = dragStart.y - e.position.y;

    if (! dragging && std::abs (dy) < 3.0f)
        return;

    if (! dragging)
    {
        dragging = true;
        slider.startedDragging();
    }

    // Full range over ~200 px; shift for 8x finer control.
    const auto range = slider.getMaximum() - slider.getMinimum();
    const auto perPixel = range / (e.mods.isShiftDown() ? 1600.0 : 200.0);
    slider.setValue (dragStartValue + dy * perPixel, juce::sendNotificationSync);
    repaint();
}

void ValueReadout::mouseUp (const juce::MouseEvent& e)
{
    if (dragging)
    {
        dragging = false;
        slider.stoppedDragging();
        repaint();
        return;
    }

    if (e.mouseWasClicked() && e.getNumberOfClicks() == 1)
    {
        editor.setText (juce::String (slider.getValue(), decimals), juce::dontSendNotification);
        editor.setVisible (true);
        editor.showEditor();
        repaint();
    }
}

void ValueReadout::mouseDoubleClick (const juce::MouseEvent&)
{
    if (slider.isDoubleClickReturnEnabled())
    {
        editor.hideEditor (true);
        slider.setValue (slider.getDoubleClickReturnValue(), juce::sendNotificationSync);
    }
}

void ValueReadout::mouseWheelMove (const juce::MouseEvent& e, const juce::MouseWheelDetails& wheel)
{
    slider.mouseWheelMove (e.getEventRelativeTo (&slider), wheel);
}

//==============================================================================
void ValueReadout::sliderValueChanged (juce::Slider*)
{
    repaint();
}

void ValueReadout::labelTextChanged (juce::Label*)
{
    // Accept "−3", "-3dB", "+1.5 dB", "250ms"...
    auto text = editor.getText().replace (juce::String::fromUTF8 ("\xe2\x88\x92"), "-").trim();
    auto digits = text.retainCharacters ("0123456789.+-");

    if (digits.isNotEmpty())
        slider.setValue (digits.getDoubleValue(), juce::sendNotificationSync);
}

void ValueReadout::editorHidden (juce::Label*, juce::TextEditor&)
{
    editor.setVisible (false);
    repaint();
}
