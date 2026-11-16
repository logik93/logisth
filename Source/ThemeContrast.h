#pragma once
#include "ThemeSettings.h"
#include <algorithm>
namespace logisth {
inline double relativeLuminance(juce::Colour c) {
    auto linear = [](double x) {
        return x <= .04045 ? x / 12.92 : std::pow((x + .055) / 1.055, 2.4);
    };
    return .2126 * linear(c.getFloatRed()) + .7152 * linear(c.getFloatGreen()) +
           .0722 * linear(c.getFloatBlue());
}
inline double colourContrast(juce::Colour a, juce::Colour b) {
    const auto x = relativeLuminance(a), y = relativeLuminance(b);
    return (std::max(x, y) + .05) / (std::min(x, y) + .05);
}
inline juce::Colour readableColour(juce::Colour colour, juce::Colour a, juce::Colour b,
                                   double minimum) {
    auto score = [&](juce::Colour c) {
        return std::min(colourContrast(c, a), colourContrast(c, b));
    };
    if (score(colour) >= minimum)
        return colour;
    const auto target = score(juce::Colours::white) >= score(juce::Colours::black)
                            ? juce::Colours::white
                            : juce::Colours::black;
    float low = 0, high = 1;
    for (int i = 0; i < 12; ++i) {
        const float mid = (low + high) * .5f;
        if (score(colour.interpolatedWith(target, mid)) >= minimum)
            high = mid;
        else
            low = mid;
    }
    return colour.interpolatedWith(target, high);
}
struct KnobPaint {
    juce::Colour surface, top, bottom, active, track, pointer, outline;
};
inline KnobPaint knobPaint(const ThemeDefinition &theme, bool screenContext) {
    const auto &c = theme.colours;
    KnobPaint result;
    result.surface = screenContext ? c[7] : c[1];
    const bool dark = theme.dark || screenContext;
    result.top = dark ? (screenContext ? c[7] : c[1]).brighter(.15f) : juce::Colours::white;
    result.bottom = dark ? (screenContext ? c[7].darker(.1f) : c[0]) : juce::Colour(0xffdedfd7);
    if (theme.knobStyle == 1)
        result.top = result.bottom = c[10].interpolatedWith(result.surface, .65f);
    if (theme.knobStyle == 2)
        result.top = result.bottom = result.surface;
    result.active = readableColour(c[10], result.surface, result.surface, 3);
    result.track = readableColour(c[11], result.surface, result.surface, 3);
    result.pointer = readableColour(dark ? c[9] : c[3], result.top, result.bottom, 4.5);
    result.outline = readableColour(c[8], result.surface, result.top, 1.5);
    return result;
}
} // namespace logisth
