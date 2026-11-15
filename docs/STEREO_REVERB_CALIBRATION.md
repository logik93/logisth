# Stereo and reverb calibration — v0.9.6.1

Measurements compare the v0.9.6 signal path with v0.9.6.1 using the same fixture.
They measure logisth itself, not another manufacturer's binary. Raw CSV files
are in docs/stereo-reverb and the downloadable stereo/reverb calibration bundle.

## Stereo width and phase

Fixture: A4 (440 Hz), classic saw, eight unison voices, 20 ct detune, retrigger
on, flat full-sustain envelope, filter bypassed, centered Pan, Blend 100%, five
seconds at 48 kHz. Exclude the first second. Mid=(L+R)/sqrt(2),
Side=(L−R)/sqrt(2); correlation is sum(L*R)/sqrt(sum(L²)*sum(R²)).

| Setting | L/R correlation | Side/Mid dB | Stereo RMS |
| --- | --- | --- | --- |
| v0.9.6, Stereo 100% | 0.650708 | −6.74477 | 0.0576167 |
| v0.9.6.1, Stereo 0% | 1.000000 | effectively zero Side | — |
| v0.9.6.1, Stereo 50% | 0.601987 | −6.04755 | — |
| v0.9.6.1, Stereo 100% | 0.003103 | −0.02695 | 0.0575315 |

Previously the inner voices crossfed both channels even at full width. Left and
right unison groups now reach their respective channels at 100%. The width law
makes intermediate Side/Mid amplitude approximately linear for independent
groups. Total RMS changes by only −0.013 dB in this fixture. Full Pan moves the
complete wide signal to its requested side.

No phase inversion or artificial decorrelator is added. A single voice remains
mono; so do identical retriggered voices at zero detune. Their coherent phase is
expected. Phase offset remains independent of Retrig. Frequency, waveform,
unison count, observation time and detune affect measured correlation.

CSV coverage: 44.1/48/96 kHz; 1/2/4/8 voices; widths 0/50/100%; extra sine
cases at 0/100 ct. StereoWidth CTest checks all six players, 2/4/8 voices,
mono/half/full width, level consistency, full Pan and coherent-phase cases.

## Algorithmic reverb return levels

Fixture: deterministic pink noise, seven seconds, first three excluded,
Dry/Wet 100%, Size 0.65, Damp 0.5, Decay 3.5 s, default character controls,
zero Predelay, Width 100%, Wet Gain 0 dB. Measure RMS after settling.

| Model | v0.9.6 wet/input dB | v0.9.6.1 wet/input dB |
| --- | --- | --- |
| Hall | -12.11 | -1.01 |
| Room | -5.90 | -0.70 |
| Spring | -11.31 | -1.01 |
| Vintage | -11.26 | -0.86 |
| Shimmer | -13.78 | -0.98 |

The CSV covers 44.1/48/96 kHz. These are calibrated starting points, not a
signal-independent loudness guarantee. Return compensation is outside feedback,
including Shimmer. The Dry/Wet law is equal-power; Wet Gain adds ±24 dB. Dry
plus wet may increase peaks, so patch Main trims are reauditioned separately.

## Convolution

JUCE non-uniform partitioned convolution is prepared before audio processing,
with zero added latency. Tests compare unnormalized stereo output to direct FIR
at 44.1/48/96 kHz and block sizes 1,17,256,511. Normalise uses unit L2 energy of
the actual prepared/resampled IR (maximum channel, preserving stereo balance).
It is not a perceptual loudness matcher. Live IR replacement crossfades 100 ms.
Tests also verify reset, lossless embedded IR recall without source files,
malformed input rejection and no C++ heap allocation in processing.

Reproduce CSVs with `stereo_reverb_calibration OUTPUT_DIRECTORY`. The before
capture was built before this release's DSP edits; the current target produces
the after capture. The fixture source is tests/StereoReverbCalibration.cpp.
