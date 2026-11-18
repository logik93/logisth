# Multiband distortion and appearance review — v0.9.8.3

The new routing mode wraps the original distortion kernels in 1–4 parallel
frequency bands. Legacy remains the default and directly calls the old serial
path. It is sample-identical to the original rack for all five distortion models
in the parity fixture. The prior 1084 parameters retain IDs/order/defaults; mode,
count and three crossover frequencies append at the end, for 1089 total.

## Crossover and measured reconstruction

Each Linkwitz–Riley 24 dB/oct split cascades two Butterworth low/high stages.
Lower outputs traverse the subsequent split frequencies' second-order allpasses
so summation has unity magnitude. Coefficients/state use double precision;
processing buffers are float. The runtime filter is TPT state-variable rather
than direct-form state tied to old coefficients. Independent L/R state preserves
stereo. There is phase rotation but no additional fixed/block latency.

`Rack_tests --multiband-report /absolute/folder` renders 120 sine cases: three
sample rates (44.1/48/96 kHz), four band counts and ten frequencies spanning about
30 Hz–19 kHz. It warms the actual filters, then measures input/output RMS over
integer periods. The largest magnitude error is **2.61472848319e-7 dB**, against
a 0.005 dB tolerance. `reconstruction.csv` contains all cases; this tests runtime
recombination rather than only a theoretical response plot. Right-only silence
and antiphase checks establish channel independence/polarity in these fixtures.

Cutoffs are bounded to 20 Hz–min(20 kHz, 0.45 Fs), with 3% spacing. A 15 ms
log-domain smoother updates coefficients every 32 samples. Extreme cutoff sweeps
on DC show **zero measured DC error**. Mode/count/model/order changes fade down
5 ms, switch/reset at zero and fade up 5 ms. The measured maximum adjacent sample
jump in the topology-switch fixture is **0.00515945255756**, below its 0.08 bound.
The transition intentionally dips level; these measurements do not claim
inaudibility for every possible signal or arbitrary fast automation.

Band isolation feeds about 100 Hz or 6 kHz through a low-band-only nonlinear
module. Third-harmonic amplitude is about 0.41725 for the low tone and 2.03e-8 for
the high tone. `isolation.csv` records both; `summary.csv` records maxima and
Legacy exactness. These fixtures are specific measurements, not universal audio
quality scores. Nonlinearity remains host-rate; no oversampling is introduced.

## Routing, realtime behavior and spectrum

The first N ordered module identities map low to high; unused settings remain
saved but cannot activate or consume band DSP. Disabled bands pass clean audio.
Enable and dry/filtered blends settle over about 10 ms. All-off settles to exact
bypass, and silent history is cleared. Original module-1 Drive modulation follows
its identity through reassignment. Legacy controls keep their original meaning.
Preset/state and genuine missing-field patches test default Legacy behavior.

The graph supports band selection, divider drag, wheel, double-click reset and
explicit Hz entry. Controls show only the active bands/dividers. The reserved
toolbar prevents title overlap. Actual combo selections and crossover gestures
are exercised at full and compact/fractional scale. Audio allocation guards run
Multiband with capture; inactive-band checks prove zero Effects processed samples.

Pre/post taps sit immediately around Distortion's position in the movable rack.
Capture has four preallocated atomic lanes; no callback FFT, allocation or file
access occurs. GUI EqSpectrum uses a Hann-windowed 2048-point transform and mean
stereo power, including antiphase signals. Off/hidden/closed stops capture and
FFT. Distortion mode/tilt preferences are independent from EQ. Tilt changes only
the drawing. Response curves have a relative magnitude scale; spectrum is dBFS.

## Appearance and credits

All ten palettes test visible gradient differences in background/panel pixels.
Grid 0/50/100 checks monotonic line intensity. Actual held pointer drags cross
refresh ticks without component visibility changes. Optional version-3 theme
fields `gradientAmount`/`gridOpacity` default to 0.75 and round-trip through draft
and disk. Existing palette/font/logo assets are preserved. Fourteen exported PNGs
show gradients, grid amounts, About, 1–4 bands, compact 125%, spectrum and Legacy.

ABOUT adds: mixedbytrzy - Tester, suggested hiding the modulation menu and
contributed many other ideas.

Run `processor_tests --features0983-report /absolute/folder` to reproduce the
interface evidence. Ordinary CTest includes the same behavioral assertions.
Native packages include this review and CSVs; the multiband-review release ZIP
also contains the 14 PNGs. Host GUI automation/DAW coverage remains finite; macOS
Universal binaries are verified for both architectures and runtime CI uses Intel.
