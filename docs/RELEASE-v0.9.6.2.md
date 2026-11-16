# logisth v0.9.6.2

Modern compressor adds a variable soft knee, makeup, parallel mix, transfer plot,
actual waveform and gain reduction. Old presets automatically retain Legacy.
Clipper adds Hard/Soft curves, Drive, Ceiling, Knee, Mix and first-order ADAA
antialiasing. Its static curve and actual input/output waveforms share a rack page.
The stereo-linked sample-peak limiter follows Main and shows reduction history,
peaks and activity. It enforces its ceiling without adding host latency.

Convolution now displays cached stereo IR waveforms and accepts audio drag/drop.
Loading remains asynchronous and IR PCM remains embedded in presets/project state.
Chords moves to the top of the rack, followed by Arpeggiator, effects in signal
order and Sample/Morph tools. The console remains a single window.

CPU improvements skip disabled processing, sleep safely after effect tails,
configure only selected/fading reverb models, track active FM/AM dependencies
and bypass all audio generation in MIDI-only mode. Compressor recovery and phaser/chorus
LFO phase continue during idle periods. Only the visible dynamics page
captures waveforms. Long first echoes and IR gaps are regression-tested.

All 1055 existing host definitions remain unchanged; 15 controls are appended.
Native validation requires 19 Linux / 17 Windows and macOS suites plus Windows
AddressSanitizer. New coverage includes knee continuity/ratios, attack/release,
stereo linking, clipper curves/ADAA, ceiling automation, post-Main limiting,
allocation guards, graph bindings, ordered tabs, IR waveform/drop, presets and
idle/dependency processing. CPU fixtures compare this release with v0.9.6.1.

Windows/Linux publish first, then macOS Universal appends to the same release.
VST3, CLAP, Standalone, English manual, measurement bundles and notices included.
Windows is unsigned; macOS is ad-hoc signed. VST2 needs a separately licensed SDK.
