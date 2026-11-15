# logisth 0.9.6.1 — project and algorithm review

## Algorithm provenance and conformance

| Component | Actual implementation | Evidence and limits |
| --- | --- | --- |
| Convolution reverb | JUCE 8.0.6 dsp::Convolution, non-uniform partitioning, zero added latency; original adapter | Direct stereo FIR comparison at 44.1/48/96 kHz and arbitrary blocks; resampled energy normalization, embedded IR recall, live replacement and allocation tests. |
| Signalsmith Stretch | Unmodified upstream 1.3.2, commit `88061fd4eda56a2353c5d65c74aa3f4ccf76f9da` | 3 upstream files compared byte for byte. Stereo adapter output agrees with direct upstream processing within 1e-6 at 44.1/48/96 kHz, with different rates, pitch shifts, tonality limits, formants, blocks and overlaps. |
| Signalsmith Linear | Unmodified upstream 0.6.6, commit `7bb1c8f36794d6980080ce817c91f8da8fe56a66` | 18 vendored files compared byte for byte; this is the dependency version specified by Stretch upstream. Portable FFT backend; optional external FFT backends are not enabled. |
| Kick | Original wavelogik hybrid: protected original attack, Signalsmith tail, 8 ms transition | Tests cover attack accuracy, pitch, duration, state/reset and real-time allocation. The attack/tail combination is an original part of logisth. |
| Granular, WSOLA, Tonal, Transient | Original time-domain overlap/add and waveform-correlation implementations | Tests cover pitch/duration separation and window gain. The names describe algorithm families, not imported third-party implementations. |
| Granulizer / Cloud | Original logisth implementations | Tests cover grain timing, reverse, selection boundaries and motion. Granular behaviour is verified through the documented logisth controls. |
| Subtractive engine, filters, envelopes, effects and EQ | Original wavelogik DSP; standard synthesis/filter techniques | Functional and calibration tests define the implemented behaviour. Nonlinear voicing and filter response remain logisth's own. |
| Chords and arpeggiator | Original legacy/pattern/chord implementation | MIDI scheduling/latch/overlap tests. Native chord maps and patterns round-trip through logisth presets. |

Run `python scripts/audit_algorithms.py` to verify the recorded vendor hashes.
With pinned upstream checkouts, add `--upstream-stretch PATH --upstream-linear PATH`
for direct byte comparison. [ALGORITHM_AUDIT.json](ALGORITHM_AUDIT.json) records
that comparison. `StretchControls` CTest performs the processing comparison.
These checks establish the identity of the open-source algorithms actually used;
they do not establish equivalence to proprietary plug-ins.

## Changes made during this review

- Preset search and 48 authored patches with descriptions, four macros and level measurements.
- Six independent controls per player, exposed in a compact STRETCH page. Unsupported controls are hidden for algorithms that do not implement them.
- Real upstream formant processing and tonality mapping, rather than an EQ substitute.
- Configurable FFT windows prepared outside audio processing. Superseded requests cannot publish stale pools.
- Pool rebinding clears obsolete stream references before playback resumes, including muted players. Retired pools can be reclaimed without waiting for every voice to stop.
- Stream identities handle address reuse. Disabled spectral modes release their pool.
- Allocation requests are bounded using a conservative 512 MiB estimate per spectral bank; only requested sources/unison/polyphony are prepared. Replacement can temporarily retain both old and new banks; this is not a cap on the entire process.
- Time-domain overlap gain is normalized, and Off-mode sinc bandwidth follows the new actual playback rate.
- Native CI verifies vendor identities and Windows AddressSanitizer results before publishing Windows/Linux; macOS is added afterward to the same release.

## Improvements I would prioritize next

1. **Measure aliasing and add optional 2x/4x oversampling.** `Effects.h` applies distortion at the host sample rate. Strong FM/AM/phase warp also creates additional high-frequency energy. Add swept-tone and hardstyle/vocal A/B fixtures, then offer an explicit quality/CPU tradeoff.
2. **Make sample analysis lazy and measure whole-instance memory.** `loadSource()` currently builds wavetable data even for sampler-only files. Decoded-sample and spectral-pool guards cover different allocations; a unified usage meter and deferred table analysis would help long recordings and six-player patches.
3. **Measure worst-case CPU and MIDI bursts.** Extend benchmarks across polyphony, unison, window/overlap, formants and dense arp/chord MIDI. Report cost before increasing the number of expensive spectral players. Keep configuration and file access off the audio callback.
4. **Add real host coverage on Windows/macOS.** Linux tests load actual CLAP/VST3 binaries; other platforms primarily test the processor and binary architecture. Prioritize plug-in scanning, automation, sample-rate changes, saved projects and repeated editor open/close in representative DAWs. Universal macOS builds verify both slices, while current runtime CI executes on Intel.
5. **Reduce build and maintenance coupling.** `PluginEditor.cpp` combines many views, while parameter groups rely on numeric strides. Split views and source descriptions into smaller modules, validate slot metadata, and avoid compiling JUCE modules separately for plug-in and processor-test targets where feasible.
6. **Improve discovery without expanding the main panel.** Add preset tags/favourites and per-source preparation status, and optional MORPH/LFO targets for Rate/Formant. Keep typed entry, units, scaling and mode-specific visibility consistent.

The review proposes these further changes; it does not silently add new synthesis
engines or claim unperformed DAW/reference-binary tests.

The [v0.9.6.1 review](REVIEW-v0.9.6.1.md) documents stereo/reverb calibration, convolution, oscillator clipboard, symmetric MORPH menus and the new compatibility baseline.
