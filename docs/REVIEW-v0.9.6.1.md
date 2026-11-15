# logisth v0.9.6.1 implementation and validation

## Stereo and reverb corrections

The previous full-width pan distribution left inner unison voices in both
channels. The revised mapping fully separates left/right groups at 100%, with
intermediate Side/Mid amplitude approximately proportional to the control.
At eight saw voices, 20 ct, 48 kHz: L/R correlation 0.650708 → 0.003103,
Side/Mid −6.74477 → −0.02695 dB, stereo RMS 0.0576167 → 0.0575315. Zero width
is exact mono. Single/identical-phase voices remain mono; no inverted-phase
widener is added. Full Pan now moves the complete oscillator to one side.
Tests cover all six players, counts 2/4/8, coherent cases, levels and full Pan.

Five algorithmic returns receive fixed output compensation outside feedback.
Default full-wet pink noise is close to −1 dB relative to input at 44.1/48/96 kHz.
Dry/Wet is equal-power, a common Wet Gain adds ±24 dB, and zero Predelay no
longer introduces an accidental sample. Reverb Off clears model/IR tails.
Main sine calibration remains unity. Factory Main trims are remeasured after
these sound changes; each audition starts in a fresh processor so random phase
history does not change the reference. Both default/maximum-macro profiles must
retain at least 3 dB fixture headroom. There is no hidden output limiter.

## Convolution and portable state

JUCE dsp::Convolution uses zero-latency non-uniform partitioning with a 256-sample
head. The adapter prepares, resamples and probes the actual IR outside audio.
Optional normalization sets the maximum channel's L2 energy to one after
resampling; stereo balance remains intact. It is not perceptual loudness matching.
Mono/stereo WAV, AIFF and FLAC are decoded; invalid, silent and oversized IRs
are rejected. The decoded/resampled stereo IR budget is 32 MiB, not a fixed
length or a cap on the whole process. Four-channel true-stereo IRs are unsupported.

Prepared engines are published through a stable processor-owned adapter. Old/new
engines crossfade for 100 ms; lifetime markers and the audio-reader guard permit
retirement outside processing. Imports validate/prepare before publication.
A newer import or restored patch supersedes queued older requests. A sample-rate
change invalidates preparation. Immutable IR PCM, original rate/channel count
and a bounded name are gzip-embedded in presets and DAW state. Malformed embedded
IR data is rejected before state application. No external path is required.
Factory/chamber is original synthetic material, not an externally licensed IR.

Tests compare exact stereo output with direct FIR at 44.1/48/96 kHz and block
sizes 1/17/256/511, establish zero added latency, validate resampled normalization,
reset, removed-source-file recall, DAW state, invalid imports, live replacement
and C++ allocation guards in prepared processing and the instrument callback.

## Oscillator clipboard and menus

Each of six oscillator panels has small Copy/Paste buttons in its lower-left.
The private in-process clipboard copies every oscillator control and immutable
source media, including the optional envelope, detune extensions, blend, loop
points, wavetable slicing and semantic C FM/AM source choices. Global routes,
part filters and part amplitude envelopes are separate. Memory validation
precedes paste parameter changes. Tests cover all 36 source/destination pairs,
source-file-independent media, cross-instance paste and all six panel buttons.

MORPH menus group A/B/C by function rather than isolating C. Local A/B cutoff,
resonance and mix targets join their C equivalents. Tests select all 200 first-route
macro leaves and verify symmetric host-range scaling. Convolution controls
replace irrelevant model controls inside the existing rack, and the layout
validator checks bounds and overlaps.

## Compatibility and delivery

v0.9.6.1 intentionally changes stereo/reverb sound and extends the reverb-type
and macro-companion choice ranges. The complete 1055-parameter contract is
frozen for subsequent versions. Historical mono A/B capture checks remain
byte-identical; stereo changes are covered by the new measurements instead.

Release gates: 18 Linux suites, 16 Windows/macOS suites, a separate Windows
AddressSanitizer run, nine publisher/version tests, exact theme/font identities
and pinned upstream source checks. Public asset checks validate checksums,
source fingerprints, native metadata, architecture slices, manuals, themes and
measurement bundles. macOS runtime tests run on Intel; Universal slices are
verified. These checks do not replace auditioning in the user's particular DAW.

With Convolution enabled, the tail reported to the host includes the IR duration
plus the existing conservative 60-second instrument/effects release allowance.
A long-IR regression checks that responses over one minute are retained.
