# logisth v0.9.5 implementation and validation

## Three signal parts

Part C has two source players, a filter, amplitude envelope and mixer level.
The existing global Filter Control and effects serve all three parts. Filter
inputs retain A/B/A+B/None and add C/A+C/B+C/A+B+C. None remains disconnected.
C1/C2 support the same four engines, source envelopes and modulation as A/B.
C is also included in sample import/export, spectral preparation, playback
telemetry, per-player previews, Solo and the mixer oscilloscope.

A+B modulation continues to affect the original four players. Explicit C and
A+B+C destinations add the new part. Muted C players can supply FM/AM signals.
New C phase seeds do not advance the random sequence used by older A/B sounds.
Silent C does not extend older voices or add filter output.

## Host and sound compatibility

The first 736 parameters retain their complete v0.9.4.1 definitions. Extending
existing normalized ranges or choice counts would reinterpret old host
automation, so 218 additions follow them instead. Main detune controls combine
the original 0–100 ct parameter with a 0–100 ct additive parameter. C choices use
appended companion parameters; the GUI presents one combined control. The manual
explains how to automate those additions directly in a host.

A frozen JSON contract checks all original specification fields and host indices.
The test suite also builds the original rendering headers and the current engine
separately, comparing 20 A/B audio and voice-lifetime fixtures at three sample
rates. Local Linux renders are byte-identical. Cross-platform comparison allows
2e-6 floating-point error. These are regression fixtures, not an exhaustive claim
about every host or possible patch. Older presets default to silent C, tail loops
off, zero additive detune and original routing choices.

## Sampler tail playback

The complete main selection plays once before the virtual timeline repeats the
independent tail interval. Playback and interpolation map that timeline back to
the source, including reverse and streaming stretch. Crossfades overlap the
loop head and are bounded to a quarter of its length. Tail changes use the
existing player transition, while disabling tails resumes from the displayed
source position. Alt-drag and lower handles edit the tail independently of trim.

Generated audio fixtures cover the first full pass, tail-only repeats, reverse,
stereo, four interpolation modes, all eight playback algorithms including actual
Signalsmith processing, small regions, clipping and seam crossfades.

## UI, presets and resource limits

The existing canvas holds A/B/C editing tabs, A/B/C/Main faders and A/B/C/L+R
scope buttons. C destinations are grouped in a compact submenu. Engine changes
refresh applicable pages immediately. Layout validation visits all 954 controls.
Integration tests exercise six detune controls, extended menus, mouse tail edits,
all-parameter preset round-trips and embedded independent C stereo samples after
their original files are deleted. Real spectral output reaches scope C.

The 512 MiB aggregate decoded-PCM guard and separate 512 MiB spectral-bank
estimate now cover six sources. More active players can consume more CPU/RAM;
configuration remains outside audio processing. Existing real-time allocation,
small-stack, source-replacement and upstream DSP conformance checks remain active.

The native release requires 15 Linux or 13 Windows/macOS CTest suites without
failures or skips, plus Windows AddressSanitizer before publication. Local Linux
passed 15/15. Platform receipts record native results; macOS Universal is added
only after the initial Windows/Linux publication. Current checks do not replace
hands-on testing of every DAW or arm64 runtime execution.
