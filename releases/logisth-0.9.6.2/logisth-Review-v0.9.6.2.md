# logisth v0.9.6.2 implementation and validation

## Dynamics and signal order

Modern compression uses a continuous quadratic soft knee in the dB domain and
attack/release smoothing of the desired reduction. A linked maximum L/R detector
preserves stereo balance. Legacy retains the earlier detector/curve. Missing
compmode in an old preset/state selects Legacy, while new patches start Modern.
Makeup and parallel Mix are independent. Existing parameter definitions remain
frozen; all fifteen new controls are appended for 1070 total.

Clipper's Hard/Soft curve and analytic integral are shared by its processing and
plot. Optional first-order ADAA reduces aliasing with no oversampling allocation
or added host latency. The wet path has approximately half-sample phase lag.
Direct remains available. Dry/wet blending can exceed the clipper ceiling.

The zero-added-latency sample-peak limiter is after Main. It uses an instantaneous
linked gain constraint, exponential release and a final per-sample ceiling bound,
including during automation. It is not an intersample true-peak limiter.

Meter values come from actual processing. A fixed atomic ring captures only the
visible dynamics page; graphs read it on the UI thread. Curve plots incorporate
Drive/Makeup/Mix. Knobs, plots, waveforms, activity and preset controls stay in the
central rack. Chords is first; effects follow their actual order; Sample/Morph
remain tools. UI tests check all 1070 fields, bindings and ordered tab positions.

## CPU and IR display

Disabled FX bypass directly. Empty/tail-complete enabled FX sleep after a quiet
window plus a conservative first-echo/IR-duration horizon, avoiding premature
silence before delayed content. Selected/fading reverb models alone receive
configuration. Idle compressor recovery and free phaser/chorus phase advance
analytically. FM/AM work follows a per-render dependency closure that includes
possible modulation and smoothed depths. Muted carriers with inactive depths do
not force unused players to run; transitive active modulation remains audible.

MIDI-only avoids audio rendering and note voice creation while preserving the
MIDI/chord/arp pipeline and controller updates. Switching modes clears audio
voices and effect state. DSP prepare also establishes rate/block details for
consistent direct use and tests.

IR extrema are built during factory generation/import/state decoding, outside
audio, and retained with immutable PCM. The waveform paints cached stereo bins
and accepts WAV/AIFF/FLAC drag/drop using the existing import worker. Invalid
imports retain the previous response. Preset portability remains intact.

## Verification

Tests cover static knee/ratio response, stereo-linked settling/release, clipping
bounds for both curves/qualities, alias reduction relative to the fundamental,
exact limiter ceilings under automation, direct bypass, sleeping/waking FX,
long first echoes, transitive FM/AM dependencies, old-preset Legacy selection,
all new controls' preset round-trip, actual post-Main ceiling, allocation-free
callbacks, graph capture, IR waveform/drop, ordered tabs and MIDI-only CPU.

Required native gates: 19 Linux / 17 Windows/macOS suites and a separate Windows
AddressSanitizer run. Public downloads are checked against native fingerprints,
architectures, version metadata and checksums. Runtime tests on macOS use Intel;
both Universal architecture slices are inspected. CPU figures are fixture-specific,
not a guarantee for every host/preset. See DYNAMICS_CPU.md and its raw CSVs.
