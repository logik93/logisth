#pragma once
#include <algorithm>
#include <cmath>
namespace logisth {
// A full spread sends non-central unison voices exclusively to either side.
// This maps width to the Side/Mid amplitude ratio for independent voice groups.
inline float unisonPan(float spread, float width, float center) {
    width = std::clamp(width, 0.f, 1.f);
    center = std::clamp(center, -1.f, 1.f);
    const float side = spread < 0 ? -1.f : spread > 0 ? 1.f : 0.f;
    const float separation = 2 * width / (1 + width * width);
    return center + side * separation * (1 - std::abs(center));
}
} // namespace logisth
