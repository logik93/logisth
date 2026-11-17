# logisth v0.9.8 — GUI and DSP review

This report describes v0.9.8. Earlier published tags and assets are unchanged. The implementation retains all 1071 prior host definitions and
appends 13 distortion controls, for 1084 in total.

## Chorus and distortion

Chorus uses the same 4×2 grid in Sync/free modes. RATE and DUAL occupy the far-right
column; FEEDBACK follows DRY/WET on the lower row. Hz/division switching keeps
RATE in one cell, and its selector aligns with the knob face centers.

The distortion group owns four independent serial slots. New instances and Init
start at 1/2/3/4. Dragging, keyboard movement and the context menu change real DSP
order. Each slot has an individual switch; the original group switch bypasses
all four. Stage 1 keeps the previous Type/Drive/Mix and modulation target; added
slots default off. Presets/projects retain all settings and optional order.
Malformed/duplicate orders are rejected transactionally; old files use the
original order. Each stage has independent sample-hold and smoothing state.
Live changes fade down for 5 ms, switch at zero, and fade up for 5 ms.
Bypassed/zero-Mix stages are skipped; four bypassed slots add no distortion work.

Rack tests cover all 24 inner permutations, noncommuting nonlinear audio,
independent bypass and all-disabled work. Existing tests retain 40320 outer-rack
permutations and frozen default FX parity at 44.1/48/96 kHz. Processor tests use
actual pointer clicks/drags, keyboard moves, portable saves and invalid/old states.

## Equalizer spectra

Four atomic full-rate lanes capture pre-L/pre-R/post-L/post-R at the actual EQ
position. FFT work runs only on the GUI thread, with 2048 points and a periodic
Hann window. One-sided amplitudes use window-sum normalization; the display uses
mean stereo power to preserve visibility of opposite-polarity channels. A
bin-centered full-scale sine measures 0 dBFS and a doubled post signal +6.0206 dB.
The response curve remains distinct from the spectra.

Off/Pre/Post/Both and five display tilts (0/1.5/3/4.5/6 dB/oct) are application
preferences. Tilt is anchored at 1 kHz and changes no audio coefficients. Off,
hidden EQ pages and closed editors stop capture/FFT work. The tests check signal
calibration, antiphase, pre/post tap identity, bypass and each tilt value.

## Convolution stereo checks

The review imports real WAV fixtures: mono/asymmetric-stereo IRs and independent,
left-only, right-only and antiphase sources at 44.1/48/96 kHz. Normalization on/off
produces 48 cases. The complete reverb stage, at Width=1 and Predelay=0, is compared
sample by sample with independent direct L/R FIR convolution. Two more tests
render the actual sampler through the mixer, convolution and Main.

Maximum recorded absolute sample error: **1.036e-7** (threshold 2e-5). C++ allocation
guards pass during processing. No lane swap, unwanted mono fold or cross-lane
leak appeared in these fixtures. The convolution DSP was retained. A stereo IR
is diagonal (L*IR-L, R*IR-R); a mono IR filters both lanes independently. This is
not a four-channel cross-convolution matrix, an acoustic quality comparison, or
a claim about every imported IR/preset. Width=0 deliberately collapses the wet
side component. Normalization applies one shared energy gain, preserving balance.

[Raw stereo measurements](gui-review/stereo-convolution.csv) record all cases.

`processor_tests --review-report /absolute/folder` exports the input WAVs, two wet
examples, stereo-convolution.csv and GUI captures. The fixtures are original
mathematical signals, not third-party recordings.

## Text, settings, file dialogs and branding

Settings separates Display, Text, Colours, Skin, MIDI, Files and About. Font size
80–125%, theme/regular/bold/italic/bold-italic style and font AA persist globally.
Base sizes are stable through repeated changes. AA Off thresholds only shaped
glyph coverage at physical resolution; other vector curves remain antialiased.
Editable text, menus and tooltips use this path; a dirty-region cache limits GUI
work. Synthetic italic supplies a slant when no italic face is available. The
scale of this optional text cache is bounded to 0.5–4×. Native file-dialog fonts
are controlled by the operating system.

Tests compare actual rendered text pixels: AA-on has intermediate coverage;
AA-off has 0/255 glyph coverage, while a neighboring vector curve remains smooth.
Pointer/size/style tests check existing editable text, settings bounds, preference
reload and restoration without cumulative scaling.

Preset open/save, samples, IRs and library choices have independent remembered
folders in preferences.json, outside sound presets. Only successful operations
replace them. Deleted folders fall back to an existing ancestor. Tests perform
actual save/load/import operations, failed loads and preference reload.

The owner-provided SVG is embedded unchanged. Its smaller standard-header mark
has no adjacent wordmark/description; the lower-right footer reads
`wavelogik / logisth / polyphonic instrument`. AP source artwork is unchanged.

| Asset | SHA-256 |
| --- | --- |
| logisth SVG | 2c39ac163acb633eeeff3759c339b07e7b4315f4423685e4e9b5b30ae5b5cddb |
| Existing AP bitmap | 973510be7c5ca8f47b563fea52d5476bcd02f09a89b0f6b92b5614c5e9552e8f |

Current installation instructions recommend Windows system VST3/CLAP directories
under `C:\Program Files\Common Files`, retain valid per-user alternatives, and
list macOS/Linux system and user locations. Copy complete bundles.
