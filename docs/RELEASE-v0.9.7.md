# logisth v0.9.7

The audio rack can now be reordered by dragging, Alt+Up/Down or its context menu.
All eight movable effects follow that order in actual processing, including
Clipper. Presets and projects remember it; old files retain the original order.
Live changes use a short fade with no duplicate DSP bank. Main and the final
Limiter remain fixed at the output.

Chords and Arp have their own GENERATORS row, separate from AUDIO FX and the
Sample/Morph tools. New editors open Sample first. Polyphony reaches 32 notes
across all four engines and prepared spectral playback, while retaining 16 as
the default. All 1070 prior host definitions remain intact; one additive control
extends polyphony without changing old automation ranges.

Light screen knobs have the correct surface and visible needles. All ten themes
and three knob styles receive cached contrast-aware rendering, including primary
screen plots. The review also fixes composite controls that could display the
previous host update and protects combined IR/Delay first echoes from premature
idle sleep. Supplied theme JSON and fonts remain unchanged.

Validation includes all 40320 order permutations, frozen default FX parity,
nonlinear routing, actual pointer/keyboard interactions, state compatibility,
32 voices/streams, allocation/ceiling guards, 120 theme renders and full layout.
Native gates require 20 Linux / 18 Windows and macOS suites plus Windows ASAN.

Windows/Linux publish first; macOS Universal is appended to the same release.
VST3, CLAP, Standalone, updated English user and technical manuals, original MIT
license, upstream notices, preset bank, demos and measurement bundles included.
