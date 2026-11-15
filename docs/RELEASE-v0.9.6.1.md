# logisth v0.9.6.1

Stereo now fully separates unison groups at 100%. In the eight-voice detuned-saw
measurement, L/R correlation changes from 0.651 to 0.003 and Side/Mid from
−6.74 to −0.03 dB, with essentially unchanged RMS. Zero remains exact mono;
single/identical-phase voices remain mono. Full Pan moves the whole oscillator.

MORPH targets group A/B/C together by function, with local cutoff, resonance and
mix for each part. Each oscillator has small Copy/Paste buttons including its
controls, envelope, embedded sample and wavetable configuration. The private
clipboard also works between instances and preserves FM/AM source meanings.

All five algorithmic reverbs have calibrated return gains, equal-power Dry/Wet
and a common ±24 dB Wet Gain. Their default full-wet pink-noise level is near
−1 dB relative to input. Output gain sits outside feedback. Reverb Off clears
old tails rather than resuming frozen audio.

New Convolution uses JUCE zero-latency partitioned processing. Load mono/stereo
WAV, AIFF or FLAC IRs, with optional resampled-energy normalization and the
included factory chamber. Ready IRs crossfade; import/preparation/retirement stay
off the audio thread. IR audio is embedded in presets and DAW state for portable
recall. A 32 MiB decoded/resampled budget bounds memory.

This version intentionally changes stereo/reverb sound and extends reverb/macro
choice ranges. The new 1055-parameter baseline is frozen for subsequent versions.

Validation: 18 Linux / 16 Windows and macOS suites plus Windows AddressSanitizer.
Convolution tests compare exact stereo FIR output at 44.1/48/96 kHz, arbitrary
block sizes, zero latency, resampled normalization, portable preset/project PCM,
invalid imports, live replacement and allocation-free processing. Clipboard tests
cover all 36 source/destination pairs; every macro leaf and layout is exercised.
The 48 authored presets are auditioned in 96 default/maximum-macro profiles.

Windows/Linux publish first; macOS Universal appends to this same release.
VST3, CLAP, Standalone, English manual, measurement bundles and notices included.
Windows is unsigned; macOS is ad-hoc signed. VST2 requires a licensed SDK.
