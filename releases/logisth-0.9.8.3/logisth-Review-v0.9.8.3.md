# logisth v0.9.8.3 — engineering review

The weak background gradient was mostly hidden by opaque panels. The corrected
composition applies palette-aware depth inside shells and the rack; the SKIN
page exposes persistent gradient/grid amount controls. Held slider tests check
that refreshes do not hide controls or interrupt mouse drags. Ten-palette pixel
checks, codec defaults/validation, draft persistence and theme file reload cover
the visual changes. ABOUT includes mixedbytrzy's requested contribution text.

Distortion's optional parallel mode reuses existing four module identities and
adds phase-aligned LR24 splits with TPT state, smoothed cutoffs and causal fades.
Legacy stays unchanged. Five appended controls preserve the previous parameter
layout. Graphic gestures, numeric cutoffs, active-band reordering, independent
spectrum preferences and host state are tested. Inactive slots cannot trigger
DSP; capture and FFT stop off/hidden/closed. See MULTIBAND_REVIEW.md and its raw
CSV/PNG release bundle for the numerical method, measured values and limits.

Existing stereo-convolution, gain/factory, frozen DSP, typography, palette,
compact layout, public-release and native memory checks remain required.
