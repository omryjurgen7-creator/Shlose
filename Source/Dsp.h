#pragma once
#include <JuceHeader.h>
#include <complex>

// RBJ band-pass (constant 0 dB peak gain). No allocations -> safe on audio thread.
struct Biquad
{
    double b0 = 1, b1 = 0, b2 = 0, a1 = 0, a2 = 0, z1 = 0, z2 = 0;

    void setBandpass (double sr, double f, double q)
    {
        f = juce::jlimit (20.0, sr * 0.45, f);
        const double w0 = juce::MathConstants<double>::twoPi * f / sr;
        const double alpha = std::sin (w0) / (2.0 * q);
        const double c = std::cos (w0);
        const double a0 = 1.0 + alpha;
        b0 = alpha / a0;
        b1 = 0.0;
        b2 = -alpha / a0;
        a1 = -2.0 * c / a0;
        a2 = (1.0 - alpha) / a0;
    }

    inline double process (double x)
    {
        const double y = b0 * x + z1;
        z1 = b1 * x - a1 * y + z2;
        z2 = b2 * x - a2 * y;
        return y;
    }

    void reset() { z1 = z2 = 0.0; }

    // complex frequency response at f Hz (used by the EQ display)
    std::complex<double> response (double sr, double f) const
    {
        const double w = juce::MathConstants<double>::twoPi * f / sr;
        const std::complex<double> z1c = std::polar (1.0, -w), z2c = std::polar (1.0, -2.0 * w);
        return (b0 + b1 * z1c + b2 * z2c) / (1.0 + a1 * z1c + a2 * z2c);
    }
};
