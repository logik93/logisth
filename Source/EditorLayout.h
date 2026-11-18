#pragma once
namespace logisth::EditorLayout {
inline constexpr int width = 1240, expandedHeight = 860, modulationHeight = 220;
inline constexpr int height(bool collapsed) {
    return expandedHeight - (collapsed ? modulationHeight : 0);
}
} // namespace logisth::EditorLayout
