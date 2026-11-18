#pragma once
#include "EqSpectrum.h"
namespace logisth_ui {
inline void drawStereoSpectrum(juce::Graphics &g, juce::Rectangle<float> area,
                               const LogisthColours &theme, const logisth::EqSpectrum &spectrum,
                               int analysisMode, int analysisTilt, double rate) {
    rate = std::max(8000., rate);
    for (int stream = 0; stream < 2; ++stream) {
        if (!(analysisMode & (1 << stream)))
            continue;
        juce::Path path;
        bool started = false;
        for (int i = 0; i < 384; ++i) {
            const double hz = 20 * std::pow(1000., i / 383.);
            if (hz >= rate * .5)
                break;
            const double bin = hz * logisth::EqSpectrum::size / rate;
            const auto at = std::min(size_t(bin), logisth::EqSpectrum::bins - 2);
            const float fraction = float(bin - at);
            const auto &values = spectrum.db[size_t(stream)];
            const auto level = values[at] + fraction * (values[at + 1] - values[at]) +
                               logisth::EqSpectrum::tiltDb(hz, analysisTilt);
            const auto yy = area.getBottom() - float((level + 108) / 120) * area.getHeight();
            if (!started) {
                path.startNewSubPath(
                    (area.getX() + float(std::log(hz / 20) / std::log(1000.)) * area.getWidth()),
                    yy);
                started = true;
            } else
                path.lineTo(
                    (area.getX() + float(std::log(hz / 20) / std::log(1000.)) * area.getWidth()),
                    yy);
        }
        g.setColour((stream == 0 ? theme.mint : theme.screenAccent).withAlpha(.65f));
        g.strokePath(path, juce::PathStrokeType(1.2f));
    }
}
} // namespace logisth_ui
