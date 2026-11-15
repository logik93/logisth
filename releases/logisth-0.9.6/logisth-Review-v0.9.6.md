# logisth v0.9.6 implementation and validation

## Modulation and compatible control ranges

A+C and B+C now join every applicable shared destination family. Each part
receives the sum of its individual, paired and all-part routes. Volume routes
also activate C when its original player levels were zero. Grouped popup menus
retain the existing numeric IDs, show readable function/scope labels and expose
all 115 destinations. An integration test selects and reads back every leaf.

The 954 previous parameter definitions retain their IDs, order, ranges, defaults
and choice counts. Wide detune, unison blend, paired-choice companions, fixed
velocity and per-band EQ settings are appended, for 1053 parameters. A frozen
contract verifies the older ranges and ordering. Main DETUNE combines three
parameters into 0–400 ct, with 100 ct at the knob midpoint. The editor initializes
all attachments before its initial value callback, including preset reopening.

## Unison and MIDI

Blend 100% retains the previous unison result. Odd unison counts keep their
existing center as other voices fade. Even counts use a reserved additional
center lane, prepared outside processing for spectral sources. Power compensation
and a short smoothing ramp avoid large level changes. The extra lane does not
advance the old random sequence; original A/B fixtures remain byte-identical.
Tests measure the actual ±400 ct frequencies in all six players, compare zero
blend with a single tuned voice across counts 2–8 and exercise all four engines.

Optional fixed velocity defaults off. When enabled, the processor and generated
arp MIDI use velocity 127. Note-off/controller handling is retained; arp rests
still remain rests. The switch is stored with patch/DAW state. Tests cover the
engine's velocity response, Settings binding, presets and actual generated MIDI.

## EQ and interface

Graphic EQ has sixteen slots, initially the previous ten Bell bands plus six
disabled slots. Each slot has enable, frequency, Q, gain and one of eight filter
shapes. Gain applies to Bell/shelves. High/low-pass use second-order sections;
notch, band-pass and all-pass are also available. Legacy shelves are retained.

Band enable, gain, frequency and Q changes are smoothed. Filter-type changes
crossfade, and fast requests settle to the last type. Tests compare calculated
responses with actual audio at 44.1/48/96 kHz, exercise all sixteen slots and
switch types/enables live. No new processing-time allocation is introduced.

The graph reserves axis margins; a selected-band inspector replaces crowded
faders. The old EQ footer that entered the value area is removed. Numeric
frequency display rounds correctly near 1 kHz. All 1053 controls are checked
for reachability/overlap. About is a scrollable, themed read-only text area and
contains every requested project credit exactly.

## Validation and limits

Native publication requires 17 Linux or 15 Windows/macOS CTest suites and a
separate Windows AddressSanitizer pass. Existing upstream identity/DSP conformance,
small audio-stack, source replacement, real plug-in load, preset, calibration,
factory-level and theme/font checks remain active. Spectrum tests establish the
implemented behavior; they do not claim equivalence to proprietary binaries.

Spectral center lanes count toward the separate 512 MiB bank estimate. More
players/bands can use additional CPU and RAM. Current CI tests macOS runtime on
Intel and verifies both Universal architectures. Native receipts record the
platform results; public assets and SHA256SUMS are checked after publication.
