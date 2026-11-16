# Rack, polyphony and skin validation — v0.9.7

## Signal routing and live changes

Eight movable audio stages form a validated permutation: Distortion, Phaser,
Chorus, Equalizer, Reverb, Delay, Compressor and Clipper. Main and the output
Limiter stay fixed after them. Chords/Arp are separate MIDI generators and
Sample/Morph are tools. Drag, Alt+Up/Down and the context menu call the same
processor API. Order is optional preset/project metadata; older states use the
original order. Invalid permutations and malformed decimal attributes reject
the state before publishing assets or replacing the patch.

Rack CTest round-trips all 40320 permutations. It compares the original default
path against frozen v0.9.6.2 source at 44.1/48/96 kHz, with all seven original
effects enabled and modulated input. Maximum permitted absolute error is 2e-5.
Separate EQ/distortion and EQ/clipper fixtures establish noncommuting audio
behavior rather than testing only an onscreen list.

Live reordering uses one state bank: 5 ms fade down, change only at zero, 5 ms
fade up. Audio work splits at the transition boundary. Tests bound individual
gain steps and completion time; callback allocation guards cover actual
32-note processing with reordered dynamics and the fixed final ceiling.
This transition is an intentional short level dip, not parallel-bank morphing.

Idle timing now sums serial first-response delays. A regression combines a
0.9-second sparse IR response and a 1.5-second delay, in both orders, and checks
that their late combined response survives the otherwise silent interval.
Disabled processing still bypasses and eligible quiet processing sleeps.

## Polyphony and host controls

Classic, Wavetable, Sampler and Granular render 32 held notes and retain bounded
voice stealing. Spectral preparation/readiness and both playback lanes address
voice 32. Default polyphony remains 16, and the inner renderer visits only the
configured range. The existing spectral memory budget still applies.

The complete 1070-entry prior host contract is frozen. One appended integer,
polyphonyextra 0–16, extends the old 1–16 polyphony control without changing its
normalized range, ID, position or default. The editor exposes their sum as POLY
1–32. Tests cover typed/slider changes, host changes and clearing the extension.

Review also reproduced a one-update-late composite display. Parameter attachments
could run before the APVTS raw adapter had received the same event. Combined
POLY/detune values and expanded routing menus now cache incoming attachment
values. Regression checks update each field from the host once and inspect the
visible result immediately, without a second update or polling workaround.

## Ten-theme graphics review

Light knobs inside the screen previously combined a pale face with a pale
screen needle. Their face now follows the actual screen surface. Palette/style
changes prepare and cache readable colors; painting does not run a contrast
search or write modified JSON. Supplied palettes and fonts are byte-identical.
Primary screen plot accents receive the same context-aware contrast treatment.

The fixture covers ten palettes × three knob styles × normal/screen context ×
regular/trim size: 120 cases. Enabled needle colors target at least 4.5:1 against
both gradient endpoints; active and passive arcs target 3:1 against the surface.
The selected-angle raster probe requires a visible needle-pixel contrast over
3:1. It checks rendered pixels as well as color calculations. This is a focused
component fixture, not an accessibility certification for every interface pixel.

[theme-contrast.csv](rack/theme-contrast.csv) records the per-case measurements.
[Knob contact sheet](images/logisth-knob-contrast.png),
[Light compressor](images/logisth-light-compressor.png) and
[reordered rack](images/logisth-rack-reordered.png) provide visual evidence.
All 1071 controls are checked for reachability, minimum size, hit targets and
overlap in the mode/page matrix. New editors open Sample; a drag keeps its active
editor page, and generators/output remain outside the draggable region.

## Reproduction and limits

Build Rack_tests and processor_tests with the project CMake configuration, then:

```text
ctest --test-dir build --output-on-failure
build/processor_tests --features097-report /absolute/report-folder
python -m unittest discover -s scripts/tests
python scripts/verify_theme_assets.py
```

Native releases require 20 Linux / 18 Windows and macOS suites and a separate
Windows AddressSanitizer run. macOS tests execute on Intel; Universal binaries
are checked for Intel and Apple Silicon slices. These checks do not replace
auditioning and project/automation testing in each production DAW.

Existing practical limits remain: no global oversampling, discrete WT frame
selection, bounded grain/spectral/IR memory, a sample-peak rather than true-peak
limiter, and host-dependent MIDI output routing. These are documented behaviors,
not additional synthesis algorithms introduced by this review.
