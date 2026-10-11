#pragma once
#include <JuceHeader.h>

namespace ShloseUI
{
    inline const juce::Colour bg     { 0xff101217 };
    inline const juce::Colour panel  { 0xff181b22 };
    inline const juce::Colour edge   { 0xff2a2f39 };
    inline const juce::Colour accent { 0xff35e0c0 };
    inline const juce::Colour warm   { 0xffffb454 };
    inline const juce::Colour hot    { 0xffff5a6e };
    inline const juce::Colour text   { 0xffe6eaf0 };
    inline const juce::Colour dim    { 0xff7c8594 };

    inline juce::Font font (float size, bool bold = false)
    {
        return juce::Font (juce::FontOptions (size, bold ? juce::Font::bold : juce::Font::plain));
    }
}

class ShloseLookAndFeel : public juce::LookAndFeel_V4
{
public:
    ShloseLookAndFeel()
    {
        using namespace ShloseUI;
        setColour (juce::Slider::textBoxTextColourId, text);
        setColour (juce::Slider::textBoxBackgroundColourId, juce::Colours::transparentBlack);
        setColour (juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
        setColour (juce::Slider::textBoxHighlightColourId, accent.withAlpha (0.3f));
        setColour (juce::ComboBox::backgroundColourId, panel);
        setColour (juce::ComboBox::textColourId, text);
        setColour (juce::ComboBox::outlineColourId, edge);
        setColour (juce::ComboBox::arrowColourId, accent);
        setColour (juce::PopupMenu::backgroundColourId, panel);
        setColour (juce::PopupMenu::textColourId, text);
        setColour (juce::PopupMenu::highlightedBackgroundColourId, accent.withAlpha (0.25f));
        setColour (juce::PopupMenu::highlightedTextColourId, text);
        setColour (juce::TextButton::buttonColourId, panel);
        setColour (juce::TextButton::buttonOnColourId, accent);
        setColour (juce::TextButton::textColourOffId, dim);
        setColour (juce::TextButton::textColourOnId, bg);
    }

    void drawRotarySlider (juce::Graphics& g, int x, int y, int w, int h, float pos,
                           float startAngle, float endAngle, juce::Slider&) override
    {
        using namespace ShloseUI;
        auto b = juce::Rectangle<float> ((float) x, (float) y, (float) w, (float) h).reduced (4.0f);
        const float r = juce::jmin (b.getWidth(), b.getHeight()) * 0.5f;
        const auto c = b.getCentre();
        const float lw = juce::jmax (3.0f, r * 0.13f);
        const float arcR = r - lw * 0.5f;
        const float angle = startAngle + pos * (endAngle - startAngle);

        juce::Path track;
        track.addCentredArc (c.x, c.y, arcR, arcR, 0.0f, startAngle, endAngle, true);
        g.setColour (edge);
        g.strokePath (track, juce::PathStrokeType (lw, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

        juce::Path val;
        val.addCentredArc (c.x, c.y, arcR, arcR, 0.0f, startAngle, angle, true);
        g.setColour (accent);
        g.strokePath (val, juce::PathStrokeType (lw, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

        const float bodyR = r - lw * 1.8f;
        juce::ColourGradient grad (juce::Colour (0xff2b303a), c.x, c.y - bodyR,
                                   juce::Colour (0xff171a20), c.x, c.y + bodyR, false);
        g.setGradientFill (grad);
        g.fillEllipse (c.x - bodyR, c.y - bodyR, bodyR * 2.0f, bodyR * 2.0f);
        g.setColour (edge.brighter (0.2f));
        g.drawEllipse (c.x - bodyR, c.y - bodyR, bodyR * 2.0f, bodyR * 2.0f, 1.0f);

        juce::Path ptr;
        ptr.addRoundedRectangle (-1.8f, -bodyR + 3.0f, 3.6f, bodyR * 0.42f, 1.8f);
        g.setColour (text);
        g.fillPath (ptr, juce::AffineTransform::rotation (angle).translated (c.x, c.y));
    }

    juce::Font getComboBoxFont (juce::ComboBox&) override { return ShloseUI::font (14.0f, true); }
    juce::Font getTextButtonFont (juce::TextButton&, int) override { return ShloseUI::font (14.0f, true); }
};

class Knob : public juce::Component
{
public:
    Knob (juce::AudioProcessorValueTreeState& apvts, const juce::String& paramId, const juce::String& title)
        : attach (apvts, paramId, slider)
    {
        slider.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
        slider.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 90, 20);
        slider.setRotaryParameters (juce::degreesToRadians (225.0f), juce::degreesToRadians (495.0f), true);
        slider.setDoubleClickReturnValue (true, apvts.getParameter (paramId)->convertFrom0to1 (
                                                    apvts.getParameter (paramId)->getDefaultValue()));
        addAndMakeVisible (slider);

        label.setText (title, juce::dontSendNotification);
        label.setJustificationType (juce::Justification::centred);
        label.setFont (ShloseUI::font (12.0f, true));
        label.setColour (juce::Label::textColourId, ShloseUI::dim);
        addAndMakeVisible (label);
    }

    void resized() override
    {
        auto b = getLocalBounds();
        label.setBounds (b.removeFromTop (18));
        slider.setBounds (b);
    }

private:
    juce::Slider slider;
    juce::Label label;
    juce::AudioProcessorValueTreeState::SliderAttachment attach;
};

class GRMeter : public juce::Component
{
public:
    void setGR (float db)
    {
        level = db > level ? db : level + (db - level) * 0.22f;
        repaint();
    }

    void paint (juce::Graphics& g) override
    {
        using namespace ShloseUI;
        auto b = getLocalBounds().toFloat();
        g.setColour (panel);
        g.fillRoundedRectangle (b, 10.0f);
        g.setColour (edge);
        g.drawRoundedRectangle (b.reduced (0.5f), 10.0f, 1.0f);

        auto inner = b.reduced (14.0f, 12.0f);
        g.setColour (dim);
        g.setFont (font (12.0f, true));
        g.drawText ("GAIN REDUCTION", inner.removeFromTop (16).toNearestInt(), juce::Justification::centred);
        auto valueArea = inner.removeFromBottom (24.0f);
        inner.removeFromTop (6.0f);
        inner.removeFromBottom (6.0f);

        auto bar = inner.withTrimmedLeft (inner.getWidth() * 0.45f).withTrimmedRight (2.0f);
        g.setColour (juce::Colour (0xff0c0e12));
        g.fillRoundedRectangle (bar, 4.0f);

        const float frac = juce::jlimit (0.0f, 1.0f, level / maxDb);
        auto fill = bar.withHeight (bar.getHeight() * frac);
        juce::ColourGradient grad (accent, bar.getX(), bar.getY(), hot, bar.getX(), bar.getBottom(), false);
        grad.addColour (0.5, warm);
        g.setGradientFill (grad);
        g.fillRoundedRectangle (fill, 4.0f);

        g.setFont (font (11.0f));
        for (int db = 0; db <= 24; db += 6)
        {
            const float yy = bar.getY() + bar.getHeight() * ((float) db / maxDb);
            g.setColour (edge.brighter (0.3f));
            g.drawHorizontalLine ((int) yy, inner.getX() + inner.getWidth() * 0.30f, bar.getX() - 2.0f);
            g.setColour (dim);
            g.drawText (db == 0 ? "0" : "-" + juce::String (db),
                        juce::Rectangle<float> (inner.getX() - 4.0f, yy - 7.0f, inner.getWidth() * 0.28f + 4.0f, 14.0f).toNearestInt(),
                        juce::Justification::centredRight);
        }

        g.setColour (level > 0.05f ? accent : dim);
        g.setFont (font (16.0f, true));
        g.drawText (level > 0.05f ? "-" + juce::String (level, 1) + " dB" : "0.0 dB",
                    valueArea.toNearestInt(), juce::Justification::centred);
    }

private:
    float level = 0.0f;
    const float maxDb = 24.0f;
};
