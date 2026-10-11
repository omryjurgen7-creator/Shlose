#pragma once
#include <JuceHeader.h>
#include <BinaryData.h>

namespace ShloseUI
{
    inline const juce::Colour skyTop    { 0xff58c6ff };
    inline const juce::Colour skyBottom { 0xff1d6ae6 };
    inline const juce::Colour navy      { 0xff102a66 };
    inline const juce::Colour white     { 0xffffffff };
    inline const juce::Colour yellow    { 0xffffd23f };
    inline const juce::Colour pink      { 0xffff6fb5 };
    inline const juce::Colour orange    { 0xffff9a3c };
    inline const juce::Colour green     { 0xff4fe3a1 };
    inline const juce::Colour purple    { 0xffa77bff };
    inline const juce::Colour cyan      { 0xff2fd6ff };
    inline const juce::Colour lime      { 0xffb6f23c };

    inline juce::Font font (float size)
    {
        static const bool hasRounded = juce::Font::findAllTypefaceNames().contains ("Arial Rounded MT Bold");
        return hasRounded ? juce::Font (juce::FontOptions ("Arial Rounded MT Bold", size, juce::Font::plain))
                          : juce::Font (juce::FontOptions (size, juce::Font::bold));
    }

    inline juce::String fmtHz (double f)
    {
        return f >= 1000.0 ? juce::String (f / 1000.0, 2) + " kHz" : juce::String ((int) f) + " Hz";
    }

    inline void drawShadowText (juce::Graphics& g, const juce::String& t, juce::Rectangle<int> r,
                                juce::Justification j, const juce::Font& f, juce::Colour c = white)
    {
        g.setFont (f);
        g.setColour (navy.withAlpha (0.55f));
        g.drawText (t, r.translated (0, 2), j, false);
        g.setColour (c);
        g.drawText (t, r, j, false);
    }
}

//==============================================================================
class ShloseLookAndFeel : public juce::LookAndFeel_V4
{
public:
    ShloseLookAndFeel()
    {
        using namespace ShloseUI;
        setColour (juce::ComboBox::textColourId, navy);
        setColour (juce::PopupMenu::backgroundColourId, white);
        setColour (juce::PopupMenu::textColourId, navy);
        setColour (juce::PopupMenu::highlightedBackgroundColourId, yellow);
        setColour (juce::PopupMenu::highlightedTextColourId, navy);
        setColour (juce::TextButton::textColourOffId, navy);
        setColour (juce::TextButton::textColourOnId, navy);
    }

    void drawRotarySlider (juce::Graphics& g, int x, int y, int w, int h, float pos,
                           float startAngle, float endAngle, juce::Slider& s) override
    {
        using namespace ShloseUI;
        const juce::Colour col = juce::Colour::fromString (s.getProperties().getWithDefault ("accent", yellow.toString()).toString());

        auto b = juce::Rectangle<float> ((float) x, (float) y, (float) w, (float) h).reduced (2.0f);
        const float r = juce::jmin (b.getWidth(), b.getHeight()) * 0.5f;
        const auto c = b.getCentre();
        const float trackW = juce::jmax (5.0f, r * 0.15f);
        const float arcR = r - trackW * 0.5f - 2.0f;
        const float angle = startAngle + pos * (endAngle - startAngle);

        juce::Path track;
        track.addCentredArc (c.x, c.y, arcR, arcR, 0.0f, startAngle, endAngle, true);
        g.setColour (navy);
        g.strokePath (track, juce::PathStrokeType (trackW + 5.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
        g.setColour (juce::Colour (0xff2d5fb8));
        g.strokePath (track, juce::PathStrokeType (trackW, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

        if (pos > 0.001f)
        {
            juce::Path val;
            val.addCentredArc (c.x, c.y, arcR, arcR, 0.0f, startAngle, angle, true);
            g.setColour (col);
            g.strokePath (val, juce::PathStrokeType (trackW, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
        }

        const float bodyR = r - trackW - 7.0f;
        g.setColour (navy.withAlpha (0.45f));
        g.fillEllipse (c.x - bodyR, c.y - bodyR + 4.0f, bodyR * 2.0f, bodyR * 2.0f);

        g.setGradientFill (juce::ColourGradient (col.brighter (0.35f), c.x, c.y - bodyR,
                                                 col.darker (0.15f), c.x, c.y + bodyR, false));
        g.fillEllipse (c.x - bodyR, c.y - bodyR, bodyR * 2.0f, bodyR * 2.0f);
        g.setColour (navy);
        g.drawEllipse (c.x - bodyR, c.y - bodyR, bodyR * 2.0f, bodyR * 2.0f, 3.0f);

        g.setColour (white.withAlpha (0.55f));
        g.fillEllipse (c.x - bodyR * 0.62f, c.y - bodyR * 0.68f, bodyR * 0.5f, bodyR * 0.28f);

        const float dx = std::sin (angle), dy = -std::cos (angle);
        juce::Path ptr;
        ptr.startNewSubPath (c.x + dx * bodyR * 0.2f, c.y + dy * bodyR * 0.2f);
        ptr.lineTo (c.x + dx * bodyR * 0.74f, c.y + dy * bodyR * 0.74f);
        const float lw = juce::jmax (4.0f, bodyR * 0.14f);
        g.setColour (navy);
        g.strokePath (ptr, juce::PathStrokeType (lw + 4.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
        g.setColour (white);
        g.strokePath (ptr, juce::PathStrokeType (lw, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    }

    void drawButtonBackground (juce::Graphics& g, juce::Button& b, const juce::Colour&, bool over, bool down) override
    {
        using namespace ShloseUI;
        auto r = b.getLocalBounds().toFloat().reduced (3.0f);
        if (down) r = r.translated (0.0f, 2.0f);
        g.setColour (navy.withAlpha (0.5f));
        g.fillRoundedRectangle (r.translated (0.0f, 3.0f), 14.0f);
        g.setColour (b.getToggleState() ? pink : (over ? white : white.withAlpha (0.88f)));
        g.fillRoundedRectangle (r, 14.0f);
        g.setColour (navy);
        g.drawRoundedRectangle (r, 14.0f, 3.0f);
    }

    juce::Font getTextButtonFont (juce::TextButton&, int) override { return ShloseUI::font (16.0f); }

    void drawComboBox (juce::Graphics& g, int width, int height, bool, int, int, int, int, juce::ComboBox&) override
    {
        using namespace ShloseUI;
        auto r = juce::Rectangle<float> (0.0f, 0.0f, (float) width, (float) height).reduced (3.0f);
        g.setColour (navy.withAlpha (0.5f));
        g.fillRoundedRectangle (r.translated (0.0f, 3.0f), 14.0f);
        g.setColour (white);
        g.fillRoundedRectangle (r, 14.0f);
        g.setColour (navy);
        g.drawRoundedRectangle (r, 14.0f, 3.0f);

        juce::Path arrow;
        const float ax = r.getRight() - 22.0f, ay = r.getCentreY();
        arrow.addTriangle (ax - 7.0f, ay - 4.0f, ax + 7.0f, ay - 4.0f, ax, ay + 6.0f);
        g.setColour (navy);
        g.fillPath (arrow);
    }

    juce::Font getComboBoxFont (juce::ComboBox&) override { return ShloseUI::font (16.0f); }
    juce::Font getPopupMenuFont() override { return ShloseUI::font (16.0f); }

    juce::Label* createSliderTextBox (juce::Slider& s) override
    {
        auto* l = LookAndFeel_V4::createSliderTextBox (s);
        l->setFont (ShloseUI::font (14.0f));
        l->setJustificationType (juce::Justification::centred);
        return l;
    }

    void drawLabel (juce::Graphics& g, juce::Label& l) override
    {
        using namespace ShloseUI;
        const auto bg = l.findColour (juce::Label::backgroundColourId);
        const bool hasBox = ! bg.isTransparent();
        if (hasBox)
        {
            auto b = l.getLocalBounds().toFloat().reduced (1.0f);
            g.setColour (bg);
            g.fillRoundedRectangle (b, 9.0f);
            g.setColour (l.findColour (juce::Label::outlineColourId));
            g.drawRoundedRectangle (b, 9.0f, 2.5f);
        }
        if (! l.isBeingEdited())
        {
            auto area = getLabelBorderSize (l).subtractedFrom (l.getLocalBounds());
            g.setFont (l.getFont());
            if (! hasBox)
            {
                g.setColour (navy.withAlpha (0.55f));
                g.drawText (l.getText(), area.translated (0, 2), l.getJustificationType(), true);
            }
            g.setColour (l.findColour (juce::Label::textColourId));
            g.drawText (l.getText(), area, l.getJustificationType(), true);
        }
    }
};

//==============================================================================
class Knob : public juce::Component
{
public:
    Knob (juce::AudioProcessorValueTreeState& apvts, const juce::String& paramId,
          const juce::String& title, juce::Colour accent)
        : attach (apvts, paramId, slider)
    {
        using namespace ShloseUI;
        slider.getProperties().set ("accent", accent.toString());
        slider.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
        slider.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 80, 22);
        slider.setRotaryParameters (juce::degreesToRadians (225.0f), juce::degreesToRadians (495.0f), true);
        slider.setColour (juce::Slider::textBoxTextColourId, navy);
        slider.setColour (juce::Slider::textBoxBackgroundColourId, white);
        slider.setColour (juce::Slider::textBoxOutlineColourId, navy);
        if (auto* prm = apvts.getParameter (paramId))
            slider.setDoubleClickReturnValue (true, prm->convertFrom0to1 (prm->getDefaultValue()));
        addAndMakeVisible (slider);

        label.setText (title, juce::dontSendNotification);
        label.setJustificationType (juce::Justification::centred);
        label.setFont (font (14.0f));
        label.setColour (juce::Label::textColourId, white);
        addAndMakeVisible (label);
    }

    void resized() override
    {
        auto b = getLocalBounds();
        label.setBounds (b.removeFromTop (20));
        slider.setBounds (b);
    }

private:
    juce::Slider slider;
    juce::Label label;
    juce::AudioProcessorValueTreeState::SliderAttachment attach;
};

//==============================================================================
class GRBar : public juce::Component
{
public:
    void setGR (float db)
    {
        level = db > level ? db : level + (db - level) * 0.2f;
        repaint();
    }

    void paint (juce::Graphics& g) override
    {
        using namespace ShloseUI;
        auto b = getLocalBounds();
        auto top = b.removeFromTop (24);
        drawShadowText (g, "GAIN REDUCTION", top, juce::Justification::centredLeft, font (13.0f));
        drawShadowText (g, level > 0.05f ? "-" + juce::String (level, 1) + " dB" : "0.0 dB",
                        top, juce::Justification::centredRight, font (15.0f), yellow);

        auto bar = b.toFloat().reduced (2.0f, 4.0f);
        g.setColour (navy.withAlpha (0.45f));
        g.fillRoundedRectangle (bar, 12.0f);

        const float frac = juce::jlimit (0.0f, 1.0f, level / 24.0f);
        if (frac > 0.003f)
        {
            auto fill = bar.withWidth (juce::jmax (bar.getHeight(), bar.getWidth() * frac));
            g.setGradientFill (juce::ColourGradient (green, bar.getX(), 0.0f, pink, bar.getRight(), 0.0f, false));
            g.fillRoundedRectangle (fill, 12.0f);
        }
        g.setColour (navy);
        g.drawRoundedRectangle (bar, 12.0f, 3.0f);
    }

private:
    float level = 0.0f;
};

//==============================================================================
class FaceComponent : public juce::Component
{
public:
    FaceComponent()
    {
        img = juce::ImageCache::getFromMemory (BinaryData::face_png, BinaryData::face_pngSize);
        setInterceptsMouseClicks (false, false);
    }

    void setGR (float db) { gr = db; repaint(); }

    void paint (juce::Graphics& g) override
    {
        using namespace ShloseUI;
        const double t = juce::Time::getMillisecondCounterHiRes() * 0.001;
        const float bob   = (float) std::sin (t * 1.7) * 9.0f;
        const float k     = juce::jlimit (0.0f, 1.0f, gr / 18.0f);
        const float angle = (float) std::sin (t * 1.1) * 0.045f + (float) std::sin (t * 45.0) * 0.035f * k;
        const float scale = 1.0f + 0.05f * k;

        auto area = getLocalBounds().toFloat();

        // floating shadow
        const float shW = 150.0f + bob * 2.5f;
        g.setColour (navy.withAlpha (0.28f));
        g.fillEllipse (area.getCentreX() - shW * 0.5f, area.getBottom() - 30.0f, shW, 20.0f);

        if (! img.isValid()) return;

        float iw = area.getWidth() - 24.0f;
        float ih = iw * (float) img.getHeight() / (float) img.getWidth();
        if (ih > area.getHeight() - 44.0f)
        {
            ih = area.getHeight() - 44.0f;
            iw = ih * (float) img.getWidth() / (float) img.getHeight();
        }
        const float cx = area.getCentreX();
        const float cy = area.getY() + 8.0f + ih * 0.5f + 12.0f + bob;
        const juce::Rectangle<float> dest (cx - iw * 0.5f, cy - ih * 0.5f, iw, ih);

        {
            juce::Graphics::ScopedSaveState ss (g);
            g.addTransform (juce::AffineTransform::scale (scale, scale, cx, cy)
                                .followedBy (juce::AffineTransform::rotation (angle, cx, cy)));
            g.setImageResamplingQuality (juce::Graphics::highResamplingQuality);
            g.drawImage (img, dest, juce::RectanglePlacement::stretchToFit);
        }

        // speech bubble when an S is being caught
        if (gr > 2.0f)
        {
            const float a = juce::jlimit (0.0f, 1.0f, (gr - 2.0f) / 3.0f);
            auto bub = juce::Rectangle<float> (area.getRight() - 112.0f, area.getY() + 6.0f + bob * 0.5f, 100.0f, 42.0f);
            g.setColour (navy.withAlpha (0.5f * a));
            g.fillRoundedRectangle (bub.translated (0.0f, 3.0f), 18.0f);
            g.setColour (white.withAlpha (a));
            g.fillRoundedRectangle (bub, 18.0f);
            g.setColour (navy.withAlpha (a));
            g.drawRoundedRectangle (bub, 18.0f, 3.0f);
            g.setFont (font (22.0f));
            g.drawText ("SHH!", bub.toNearestInt(), juce::Justification::centred, false);
        }
    }

private:
    juce::Image img;
    float gr = 0.0f;
};
