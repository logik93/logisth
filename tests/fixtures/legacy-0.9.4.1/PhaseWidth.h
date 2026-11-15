#pragma once
#include "Modulation.h"
namespace logisth {
// Continuous phase distortion: identity at 50%, with fixed cycle endpoints.
inline double phaseWidth(double phase, double width) {
    width = std::clamp(width, .01, .99);
    return phase < width ? .5 * phase / width : .5 + .5 * (phase - width) / (1 - width);
}
inline double warpSamplePosition(double position, double window, double width) {
    if (std::abs(width - .5) < 1e-7 || window < 2)
        return position;
    double cycle = std::floor(position / window), phase = position / window - cycle;
    return (cycle + phaseWidth(phase, width)) * window;
}
} // namespace logisth
