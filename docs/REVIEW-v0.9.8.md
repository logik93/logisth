# logisth v0.9.8 — engineering review

The Chorus rate selector previously moved when Sync changed, and its vertical
placement differed from rotary controls. Stable identity-based grid placement
keeps RATE and DUAL in the right column, with FEEDBACK after DRY/WET.

The original distortion model remains slot 1. Three independent stages, per-slot
bypass and validated saved order extend it while preserving the earlier 1071
host definitions. Tests cover all 24 inner permutations, actual noncommuting
audio, pointer/keyboard movement, old/new state and all-disabled work. Outer-rack
coverage and frozen first-stage/default FX comparisons remain active.

The EQ analyzer captures stereo before/after the actual EQ stage. Four atomic
lanes feed a GUI-only windowed FFT; stereo power avoids antiphase cancellation.
Calibration, display tilts, actual tap identity and Off behavior are tested.
Convolution imports actual WAVs for 48 rate/IR/channel/polarity/normalization cases
and compares complete wet processing to direct FIR. Two actual sampler renders
cover the mixer/Main integration. Maximum absolute error is about 1.04e-7;
no C++ audio allocations occur in the measured path.

Application typography/file/analyzer preferences are separate from sound patches.
Actual glyph-pixel tests distinguish one-bit text coverage from smooth vector
curves. Bounds/click/recreation checks cover all seven Settings categories,
manual font sizes, style, AA and folder persistence/failure recovery. The exact
owner-provided SVG is embedded; the existing AP source hash is preserved.

[GUI/DSP evidence](GUI_DSP_REVIEW.md) documents fixtures and limits. Existing
resource bounds, platform signing and host-coverage limitations still apply.
