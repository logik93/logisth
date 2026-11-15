# wavelogik logisth 0.9.3 — All-engine width, Kick stretching and MORPH

Width/PWM now reaches every Classic wave, Wavetable, Sampler and both granular
models. Pulse variants use duty modulation; other periodic waves use phase width,
sample players warp their read windows, and noise uses variable-width gating.
Each player adds tempo-synced PWM and internal Sine/A1/A2/B1/B2 FM/AM sources.
Cross-player cycles use bounded previous-sample signals, including muted modulators.

Sampler PLAYBACK adds Tonal (65 ms correlation windows), Transient (12 ms),
Spectral and Kick. Spectral/Kick integrate Signalsmith Stretch 1.1.0 under MIT;
Kick retains 0–100 ms of original attack (25 ms default), then crossfades over
8 ms to its pitched/stretched tail. Time Factor remains independent from note
pitch. This is an independent hybrid, not other instruments's external processing algorithm.
Analysis-window duration scales with host sample rate; prepared unison capacity
grows as needed. Processing buffers are prepared outside the audio callback, including with the
editor closed. Preparation jobs have generation checks, and late readiness
crossfades from the existing path. Sinc resampling,
20 ms algorithm switches and one-shot end fades remain available.

The SAMPLE view supports pointer-anchored wheel zoom, Shift-wheel pan, drag
selection, START/END handles, double-click fit and right-click reset. Changes are
nondestructive and stored with presets/projects. Granular trim is independent
of scanner position, keeping reverse-grain and modulation behaviour intact.

MORPH is the eleventh rack page, within the existing single-window console:
- Four editable macro names and four routes per macro, each with its own From/To.
- Last-assigned routing precedence for shared targets; other parameters retain bases.
- Mutate depth, OSC/FILTER/MOD/FX section locks and one-step Undo.
- Names, routes, ranges, amounts and locks saved with presets/DAW state.
- Live player traces, warped table previews and observed modulation-range rings.

The sixth standard theme, **AP always flexin'!**, combines black with blue/cyan/
violet accents and the white AP mark. The handwritten signature is removed at
the project owner's request. The skin and logo are embedded/packaged; no extra
font software is required. Five previous standard palettes remain available.

Voice resets copy a prepared heap-backed template in place, removing large stack
temporaries from note-on, voice stealing, mono, polyphony changes and All Sound
Off. This fixes a Windows integration-test stack overflow; a dedicated 128 KiB
thread regression now covers those paths with every source engine.

712 automatable parameters. New regression coverage checks all-engine width,
muted/cyclic cross-modulation, independent pitch/duration, protected Kick attack,
zero allocations inside prepared sample processing, granular crop boundaries,
macro/preset round-trips, locks/Undo, zoomed mouse selection and full GUI access.
The synthetic kick benchmark includes reference plus -12/-7/0/+7/+12-semitone WAVs
and attack-error/peak readings. It is a repeatable fixture, not a substitute for
listening to the user's own hardstyle material before a future 1.0.0 release.

Build configuration requires 11 passing Linux suites and 9 on Windows/macOS.
A separate Windows AddressSanitizer build gates publication too. Integration
fixtures are heap allocated to model DAW instances without exhausting a Windows
test thread's stack. Sanitizer binaries are excluded from release downloads.
Native formats remain VST3, CLAP and Standalone; VST2 needs a separately licensed
SDK and is excluded. Platform packages are published only after their own native
build/test/architecture checks. Windows and Linux are published first; the
macOS Universal build then starts and is added to this same release.
Original code is MIT; Signalsmith, JUCE/CLAP,
fonts and supplied artwork retain their notices.
