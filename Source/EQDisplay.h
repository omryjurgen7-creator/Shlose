#pragma once
#include <JuceHeader.h>
#include "PluginProcessor.h"
#include "UI.h"

// Pro-Q style analyser: live spectrum, dynamic curve, draggable band node.
class EQDisplay : public juce::Component
{
public:
    static constexpr int fftOrder = 12;
    static constexpr int fftSize  = 1 << fftOrder;
    static constexpr int half     = fftSize / 2;
    using Spec = std::array<float, (size_t) half>;

    explicit EQDisplay (ShloseAudioProcessor& p)
        : proc (p),
          fft (fftOrder),
          window ((size_t) fftSize, juce::dsp::WindowingFunction<float>::hann, false)
    {
        specIn.fill (floorDb);
        specOut.fill (floorDb);

        auto& a = proc.apvts;
        pFreq  = a.getParameter ("freq");
        pRange = a.getParameter ("range");
        pQ     = a.getParameter ("q");
        rFreq  = a.getRawParameterValue ("freq");
        rQ     = a.getRawParameterValue ("q");
        rRange = a.getRawParameterValue ("range");
        rMode  = a.getRawParameterValue ("mode");
        setMouseCursor (juce::MouseCursor::PointingHandCursor);
    }

    // called ~30x per second by the editor
    void update()
    {
        const int pos = proc.ringPos.load();
        const bool fresh = pos != lastPos;
        lastPos = pos;
        analyse (proc.inRing, specIn, fresh);
        analyse (proc.outRing, specOut, fresh);
        findPeak();
        repaint();
    }

    void resized() override
    {
        plot = getLocalBounds().toFloat().withTrimmedTop (52.0f).withTrimmedBottom (28.0f)
                                         .withTrimmedLeft (14.0f).withTrimmedRight (46.0f);
    }

    //==========================================================================
    void paint (juce::Graphics& g) override
    {
        using namespace ShloseUI;
        const auto full = getLocalBounds().toFloat().reduced (2.0f);
        g.setColour (navy.withAlpha (0.5f));
        g.fillRoundedRectangle (full.translated (0.0f, 4.0f), 24.0f);
        g.setColour (juce::Colour (0xff0e2a66));
        g.fillRoundedRectangle (full, 24.0f);

        const double sr = proc.getSampleRate() > 0.0 ? proc.getSampleRate() : 44100.0;
        const float fc = rFreq->load(), q = rQ->load(), range = rRange->load();
        const bool wide = rMode->load() > 0.5f;
        const float gr = proc.grMeter.load();
        const double gLive = std::pow (10.0, -(double) gr / 20.0);
        const double gMax  = std::pow (10.0, -(double) range / 20.0);

        Biquad bq;
        bq.setBandpass (sr, fc, q);

        auto respDb = [&] (double f, double gain) -> double
        {
            if (wide) return 20.0 * std::log10 (gain);
            const std::complex<double> h = 1.0 - (1.0 - gain) * bq.response (sr, f);
            return 20.0 * std::log10 (juce::jmax (std::abs (h), 1.0e-4));
        };

        {
            juce::Graphics::ScopedSaveState ss (g);
            g.reduceClipRegion (plot.toNearestInt());

            drawGrid (g);

            drawSpectrum (g, specIn, true, cyan.withAlpha (0.9f), 1.5f);
            drawSpectrum (g, specOut, false, white, 2.0f);

            // detection band (faint)
            const double bw = 1.0 / (double) q;
            const float x0 = (float) xFromFreq (fc * std::pow (2.0, -bw * 0.5));
            const float x1 = (float) xFromFreq (fc * std::pow (2.0, bw * 0.5));
            g.setColour (yellow.withAlpha (0.08f));
            g.fillRect (juce::Rectangle<float> (x0, plot.getY(), x1 - x0, plot.getHeight()));

            auto buildCurve = [&] (double gain, juce::Path& line)
            {
                bool first = true;
                for (float x = plot.getX(); x <= plot.getRight(); x += 2.0f)
                {
                    const float y = yFromDb (respDb (freqFromX (x), gain));
                    if (first) { line.startNewSubPath (x, y); first = false; }
                    else line.lineTo (x, y);
                }
            };

            // ghost = maximum reduction
            juce::Path ghost, dashed;
            buildCurve (gMax, ghost);
            const float dashes[] = { 8.0f, 6.0f };
            juce::PathStrokeType (2.0f).createDashedStroke (dashed, ghost, dashes, 2);
            g.setColour (white.withAlpha (0.75f));
            g.fillPath (dashed);

            // live curve
            juce::Path live;
            buildCurve (gLive, live);
            juce::Path fill (live);
            fill.lineTo (plot.getRight(), yFromDb (0.0));
            fill.lineTo (plot.getX(), yFromDb (0.0));
            fill.closeSubPath();
            g.setGradientFill (juce::ColourGradient (pink.withAlpha (0.8f), 0.0f, yFromDb (0.0),
                                                     pink.withAlpha (0.12f), 0.0f, yFromDb (-30.0), false));
            g.fillPath (fill);
            g.setColour (navy);
            g.strokePath (live, juce::PathStrokeType (7.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
            g.setColour (yellow);
            g.strokePath (live, juce::PathStrokeType (3.5f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

            // peak marker (where the S really is)
            if (gr > 0.3f && peakValid)
            {
                const float px = (float) xFromFreq (peakFreq);
                juce::Path pl, pd;
                pl.startNewSubPath (px, plot.getY());
                pl.lineTo (px, plot.getBottom());
                const float d2[] = { 4.0f, 5.0f };
                juce::PathStrokeType (1.8f).createDashedStroke (pd, pl, d2, 2);
                g.setColour (pink);
                g.fillPath (pd);
                juce::Path tri;
                tri.addTriangle (px - 7.0f, plot.getY(), px + 7.0f, plot.getY(), px, plot.getY() + 11.0f);
                g.fillPath (tri);
            }

            // hover crosshair
            if (hovering && plot.contains (hover))
            {
                g.setColour (white.withAlpha (0.35f));
                g.drawLine (hover.x, plot.getY(), hover.x, plot.getBottom(), 1.2f);
                const double hf = freqFromX (hover.x);
                const juce::String txt = fmtHz (hf) + "   " + juce::String (respDb (hf, gLive), 1) + " dB";
                auto bub = juce::Rectangle<float> (juce::jmin (hover.x + 10.0f, plot.getRight() - 150.0f),
                                                   plot.getBottom() - 34.0f, 140.0f, 24.0f);
                g.setColour (white.withAlpha (0.92f));
                g.fillRoundedRectangle (bub, 10.0f);
                g.setColour (navy);
                g.setFont (font (13.0f));
                g.drawText (txt, bub.toNearestInt(), juce::Justification::centred, false);
            }
        }

        drawAxisLabels (g);

        // node (max reduction) + live dot
        const float nx = (float) xFromFreq (fc), ny = yFromDb (-(double) range);
        const float nr = dragging ? 14.0f : 12.0f;
        g.setColour (navy.withAlpha (0.5f));
        g.fillEllipse (nx - nr, ny - nr + 3.0f, nr * 2.0f, nr * 2.0f);
        g.setColour (yellow);
        g.fillEllipse (nx - nr, ny - nr, nr * 2.0f, nr * 2.0f);
        g.setColour (navy);
        g.drawEllipse (nx - nr, ny - nr, nr * 2.0f, nr * 2.0f, 3.0f);
        g.setColour (white.withAlpha (0.8f));
        g.fillEllipse (nx - nr * 0.5f, ny - nr * 0.6f, nr * 0.5f, nr * 0.3f);

        if (gr > 0.05f)
        {
            const float ly = yFromDb (-(double) gr);
            g.setColour (pink);
            g.fillEllipse (nx - 6.0f, ly - 6.0f, 12.0f, 12.0f);
            g.setColour (white);
            g.drawEllipse (nx - 6.0f, ly - 6.0f, 12.0f, 12.0f, 2.5f);
        }

        // value bubble next to node
        {
            auto bub = juce::Rectangle<float> (nx + 20.0f, ny - 22.0f, 96.0f, 42.0f);
            if (bub.getRight() > plot.getRight() + 40.0f) bub.setX (nx - 20.0f - bub.getWidth());
            bub.setY (juce::jlimit (plot.getY() + 4.0f, plot.getBottom() - bub.getHeight() - 4.0f, bub.getY()));
            g.setColour (navy.withAlpha (0.45f));
            g.fillRoundedRectangle (bub.translated (0.0f, 3.0f), 14.0f);
            g.setColour (white);
            g.fillRoundedRectangle (bub, 14.0f);
            g.setColour (navy);
            g.drawRoundedRectangle (bub, 14.0f, 2.5f);
            g.setFont (font (14.0f));
            g.drawText (fmtHz (fc), bub.withTrimmedBottom (20.0f).toNearestInt(), juce::Justification::centred, false);
            g.setColour (gr > 0.05f ? juce::Colour (0xffe0338a) : juce::Colour (0xff6a7aa0));
            g.drawText (gr > 0.05f ? "-" + juce::String (gr, 1) + " dB" : "idle",
                        bub.withTrimmedTop (20.0f).toNearestInt(), juce::Justification::centred, false);
        }

        // readout chips
        const float cw = ((float) getWidth() - 24.0f - 12.0f) / 3.0f;
        drawChip (g, { 12.0f, 10.0f, cw, 32.0f }, yellow, "CENTER", fmtHz (fc));
        drawChip (g, { 18.0f + cw, 10.0f, cw, 32.0f }, pink, "NOW",
                  gr > 0.05f ? "-" + juce::String (gr, 1) + " dB" : "0.0 dB");
        drawChip (g, { 24.0f + cw * 2.0f, 10.0f, cw, 32.0f }, cyan, "S-PEAK",
                  peakValid ? fmtHz (peakFreq) : juce::String ("--"));
    }

    //==========================================================================
    void mouseDown (const juce::MouseEvent& e) override
    {
        if (! plot.expanded (10.0f).contains (e.position)) return;
        dragging = true;
        pFreq->beginChangeGesture();
        pRange->beginChangeGesture();
        applyDrag (e.position);
    }

    void mouseDrag (const juce::MouseEvent& e) override
    {
        hover = e.position; hovering = true;
        if (dragging) applyDrag (e.position);
    }

    void mouseUp (const juce::MouseEvent&) override
    {
        if (! dragging) return;
        pFreq->endChangeGesture();
        pRange->endChangeGesture();
        dragging = false;
    }

    void mouseMove (const juce::MouseEvent& e) override { hover = e.position; hovering = true; }
    void mouseExit (const juce::MouseEvent&) override   { hovering = false; }

    void mouseWheelMove (const juce::MouseEvent&, const juce::MouseWheelDetails& w) override
    {
        float q = pQ->convertFrom0to1 (pQ->getValue());
        q *= std::exp (w.deltaY * 1.2f);
        pQ->beginChangeGesture();
        pQ->setValueNotifyingHost (pQ->convertTo0to1 (juce::jlimit (0.5f, 4.0f, q)));
        pQ->endChangeGesture();
    }

private:
    static constexpr float floorDb = -140.0f;
    static constexpr double dbTop = 6.0, dbBottom = -30.0;
    static constexpr float specLo = -95.0f, specHi = -30.0f;

    double xFromFreq (double f) const { return plot.getX() + plot.getWidth() * std::log (f / 20.0) / std::log (1000.0); }
    double freqFromX (double x) const { return 20.0 * std::pow (1000.0, (x - plot.getX()) / plot.getWidth()); }
    float  yFromDb (double db) const  { return plot.getY() + plot.getHeight() * (float) ((dbTop - db) / (dbTop - dbBottom)); }

    void applyDrag (juce::Point<float> pos)
    {
        const double f = juce::jlimit (2000.0, 16000.0, freqFromX (pos.x));
        const double db = dbTop - (double) (pos.y - plot.getY()) / (double) plot.getHeight() * (dbTop - dbBottom);
        const double r = juce::jlimit (0.0, 24.0, -db);
        pFreq->setValueNotifyingHost (pFreq->convertTo0to1 ((float) f));
        pRange->setValueNotifyingHost (pRange->convertTo0to1 ((float) r));
    }

    void analyse (const std::array<float, 8192>& ring, Spec& spec, bool fresh)
    {
        if (fresh)
        {
            const int pos = proc.ringPos.load();
            for (int i = 0; i < fftSize; ++i)
                fftData[(size_t) i] = ring[(size_t) ((pos - fftSize + i) & 8191)];
            window.multiplyWithWindowingTable (fftData.data(), (size_t) fftSize);
            std::fill (fftData.begin() + fftSize, fftData.end(), 0.0f);
            fft.performFrequencyOnlyForwardTransform (fftData.data());
        }

        for (int k = 0; k < half; ++k)
        {
            float db = floorDb;
            if (fresh)
            {
                const float amp = fftData[(size_t) k] * 4.0f / (float) fftSize;
                db = juce::Decibels::gainToDecibels (amp, floorDb);
            }
            float& s = spec[(size_t) k];
            s += (db - s) * (db > s ? 0.55f : 0.10f);
        }
    }

    void findPeak()
    {
        const double sr = proc.getSampleRate() > 0.0 ? proc.getSampleRate() : 44100.0;
        const double binHz = sr / fftSize;
        const double fc = rFreq->load(), bw = 1.0 / (double) rQ->load();
        const double lo = fc * std::pow (2.0, -bw * 0.5), hi = fc * std::pow (2.0, bw * 0.5);
        const int b0 = juce::jmax (1, (int) (lo / binHz));
        const int b1 = juce::jmin (half - 2, (int) std::ceil (hi / binHz));
        float best = -1000.0f;
        int bi = b0;
        for (int b = b0; b <= b1; ++b)
            if (specIn[(size_t) b] > best) { best = specIn[(size_t) b]; bi = b; }
        peakValid = best > -90.0f;
        peakFreq = bi * binHz;
    }

    float sampleSpec (const Spec& s, double f0, double f1) const
    {
        const double sr = proc.getSampleRate() > 0.0 ? proc.getSampleRate() : 44100.0;
        const double binHz = sr / fftSize;
        const double b0 = f0 / binHz, b1 = f1 / binHz;
        float v;
        if (b1 - b0 <= 1.0)
        {
            const double b = 0.5 * (b0 + b1);
            const int i = juce::jlimit (0, half - 2, (int) b);
            const float fr = (float) (b - i);
            v = s[(size_t) i] * (1.0f - fr) + s[(size_t) i + 1] * fr;
        }
        else
        {
            v = floorDb;
            for (int i = juce::jmax (0, (int) b0); i <= juce::jmin (half - 1, (int) b1); ++i)
                v = juce::jmax (v, s[(size_t) i]);
        }
        return v + 4.5f * (float) std::log2 (0.5 * (f0 + f1) / 1000.0);   // pink-noise tilt
    }

    void drawSpectrum (juce::Graphics& g, const Spec& s, bool fill, juce::Colour line, float thick)
    {
        juce::Path p;
        bool first = true;
        for (float x = plot.getX(); x <= plot.getRight(); x += 2.0f)
        {
            const float db = sampleSpec (s, freqFromX (x), freqFromX (x + 2.0f));
            const float y = plot.getBottom() - plot.getHeight() * juce::jlimit (0.0f, 1.0f, (db - specLo) / (specHi - specLo));
            if (first) { p.startNewSubPath (x, y); first = false; }
            else p.lineTo (x, y);
        }
        if (fill)
        {
            juce::Path f (p);
            f.lineTo (plot.getRight(), plot.getBottom());
            f.lineTo (plot.getX(), plot.getBottom());
            f.closeSubPath();
            g.setGradientFill (juce::ColourGradient (ShloseUI::cyan.withAlpha (0.55f), 0.0f, plot.getY(),
                                                     ShloseUI::cyan.withAlpha (0.04f), 0.0f, plot.getBottom(), false));
            g.fillPath (f);
        }
        g.setColour (line);
        g.strokePath (p, juce::PathStrokeType (thick));
    }

    void drawGrid (juce::Graphics& g)
    {
        for (double decade : { 10.0, 100.0, 1000.0, 10000.0 })
            for (int m = 1; m <= 9; ++m)
            {
                const double f = m * decade;
                if (f < 20.0 || f > 20000.0) continue;
                const float x = (float) xFromFreq (f);
                g.setColour (ShloseUI::white.withAlpha (m == 1 ? 0.16f : 0.06f));
                g.drawLine (x, plot.getY(), x, plot.getBottom(), 1.0f);
            }
        for (int db = -30; db <= 6; db += 6)
        {
            const float y = yFromDb (db);
            g.setColour (ShloseUI::white.withAlpha (db == 0 ? 0.55f : 0.10f));
            g.drawLine (plot.getX(), y, plot.getRight(), y, db == 0 ? 1.8f : 1.0f);
        }
    }

    void drawAxisLabels (juce::Graphics& g)
    {
        g.setFont (ShloseUI::font (12.0f));
        g.setColour (ShloseUI::white.withAlpha (0.8f));
        const std::pair<double, const char*> fl[] = { { 50, "50" }, { 100, "100" }, { 200, "200" }, { 500, "500" },
                                                      { 1000, "1k" }, { 2000, "2k" }, { 5000, "5k" },
                                                      { 10000, "10k" }, { 20000, "20k" } };
        for (auto& f : fl)
        {
            const int x = (int) xFromFreq (f.first);
            g.drawText (f.second, x - 20, (int) plot.getBottom() + 5, 40, 16, juce::Justification::centred, false);
        }
        for (int db = -30; db <= 6; db += 6)
            g.drawText (db > 0 ? "+" + juce::String (db) : juce::String (db),
                        (int) plot.getRight() + 6, (int) yFromDb (db) - 8, 38, 16, juce::Justification::centredLeft, false);
    }

    void drawChip (juce::Graphics& g, juce::Rectangle<float> r, juce::Colour c,
                   const juce::String& label, const juce::String& value)
    {
        using namespace ShloseUI;
        g.setColour (c);
        g.fillRoundedRectangle (r, 14.0f);
        g.setColour (navy);
        g.drawRoundedRectangle (r.reduced (1.0f), 14.0f, 2.5f);
        g.setFont (font (11.0f));
        g.setColour (navy.withAlpha (0.7f));
        g.drawText (label, r.withTrimmedLeft (11.0f).toNearestInt(), juce::Justification::centredLeft, false);
        g.setFont (font (15.0f));
        g.setColour (navy);
        g.drawText (value, r.withTrimmedRight (11.0f).toNearestInt(), juce::Justification::centredRight, false);
    }

    ShloseAudioProcessor& proc;
    juce::dsp::FFT fft;
    juce::dsp::WindowingFunction<float> window;
    std::array<float, (size_t) fftSize * 2> fftData {};
    Spec specIn {}, specOut {};
    int lastPos = -1;

    juce::RangedAudioParameter *pFreq = nullptr, *pRange = nullptr, *pQ = nullptr;
    std::atomic<float> *rFreq = nullptr, *rQ = nullptr, *rRange = nullptr, *rMode = nullptr;

    juce::Rectangle<float> plot;
    juce::Point<float> hover;
    bool hovering = false, dragging = false;
    double peakFreq = 0.0;
    bool peakValid = false;
};
