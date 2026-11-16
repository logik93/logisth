# Dynamics and CPU measurements — v0.9.7

## CPU fixtures

Compare v0.9.6.2 with v0.9.7 using the same GNU C++17 -O2 compiler on Linux
x86_64. Render 48 kHz / 512 frames; seven rounds of 128 blocks, median elapsed
microseconds per block. Timing excludes preparation, file I/O and GUI painting.
The raw CSVs include output checksums; all six checksums agree exactly.

| Fixture | Before µs | After µs | Before/after |
| --- | --- | --- | --- |
| All FX bypassed | 0.046 | 0.045 | 1.03× |
| Empty enabled reverb | 0.368 | 0.365 | 1.01× |
| Inactive zero-depth FM source | 280.366 | 228.784 | 1.23× |
| Active muted FM source | 553.337 | 486.484 | 1.14× |
| Silent engine | 2.718 | 2.622 | 1.04× |
| Note after idle / free LFO | 261.679 | 224.368 | 1.17× |

These are fixed synthetic cases, not an overall speed guarantee. The bypass and
empty-reverb rows deliberately measure unused processing. The oscillator rows
use one held voice with the polyphony limit at one, one saw carrier and a muted
eight-voice noise source, with its external FM depth zero or active. The latter
must remain audible as a modulator. The idle/free-LFO case includes an initial
silent interval and verifies matching subsequent output. The silent engine
uses the normal sixteen-voice limit. Real patches vary with voices, unison,
source engines, effect tails, host and hardware. Active modulation still needs
its dependent oscillators.

Version 0.9.7 extends note capacity to 32 but visits only the configured voice
range in the inner audio loop. The fixture set keeps its prior limits for a
controlled comparison. Idle/bypass figures are near the measurement floor and
small differences should not be treated as reliable overall gains.

No audio callback allocation is introduced. Disabled FX bypass directly; enabled
empty effects sleep. A tail-complete gate sums serial first-echo delay and IR
length before sleeping, in either effect order. Modern/Legacy compressor recovery and phaser/chorus LFO
clocks advance during sleep. Idle synth LFO clocks and random-wrap ordering are
retained without generating unused waveforms. MIDI-only mode skips audio work.
Only the visible dynamics page captures waveform/activity data. IR drawing uses
cached extrema created during background import/state decoding.

Reproduce with tests/CpuCalibration.cpp: compile once against the v0.9.6.2 Source
folder and once against current Source, using the same flags, then run each with
an output CSV path. The current CMake target is cpu_calibration.

## Dynamics curves and behavior

compressor-curves.csv uses the same transfer function as the audio engine and
plot: threshold −18 dB, ratio 4:1, knees 0/6/12/24 dB. Below the knee reduction
is zero; above it the stated ratio applies; the middle is a continuous quadratic
transition. Modern smooths the required dB reduction; Legacy keeps its detector.
clipper-curves.csv covers Hard/Soft, knee 0/25/50/100% and ceiling amplitude one.

The unchanged clipper retains the following v0.9.6.2 alias fixture.
Optional first-order ADAA uses the curve's analytic integral. A coherent 8192-frame
fixture at 48 kHz, bin 997, amplitude four and hard ceiling one measures the folded
fifth harmonic relative to the fundamental. Direct ratio: 0.152896; ADAA: 0.071187,
approximately 6.64 dB lower. This is one alias measurement, not a universal alias
suppression figure. The wet path has approximately half-sample phase lag; no
host latency or oversampling buffer is added. Direct remains available.

The final limiter is stereo-linked, has instantaneous attack and exponential
release, and bounds actual output samples at the selected ceiling after Main.
It is a sample-peak limiter, with no intersample true-peak detector or lookahead.
Tests include ceiling automation, bypass, stereo balance, knee/ratio response,
release, clipper bounds, preset compatibility, actual post-Main output, audio
allocation guards, dynamic UI bindings and IR waveform/drop.

Reproduce transfer CSVs with dynamics_calibration OUTPUT_DIRECTORY. New DSP is
original wavelogik code; existing open-source dependencies/notices are unchanged.
