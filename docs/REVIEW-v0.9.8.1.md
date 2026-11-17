# logisth v0.9.8.1 — engineering review

The previous optional text renderer cached the complete interface then composited
it with nearest-neighbour resampling. Fractional UI/DPI transforms could make text
uneven and retain pixels from transparent repaints. The wrapper now intercepts
glyphs during the normal paint; vectors draw directly. Uniform scaling avoids
separate X/Y rounding stretching glyphs. Native/grayscale/2x/4x, Off and edge weight
are persisted. The renderer supersamples only glyph masks and averages coverage.

The appearance regression matrix checks actual glyph pixels and exact vector
identity across six UI scales, two display scales and every quality/Off setting.
Coverage-weight, transparent repaint, folder selection and portable PNG/JPEG/grid
fixtures extend it. Context menus use the owning look-and-feel on every theme.
Distortion has a dedicated title row; inline-header views avoid redundant titles.
See [APPEARANCE_REVIEW.md](APPEARANCE_REVIEW.md) for evidence and limits.

Audio algorithms, the 1084-parameter contract and existing real-WAV convolution,
gain, factory-bank, rack/contrast and memory regressions remain active.
