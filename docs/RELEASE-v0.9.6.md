# logisth v0.9.6

The modulation matrix now offers A+C and B+C alongside A, B, C, A+B and A+B+C.
Menus group oscillator, filter, mixer/effect, LFO, WT position, grain, FM, AM
and Width/PWM destinations. Closed fields retain the function and selected scope.

Every player adds **BLEND** next to **DETUNE**, now reaching **400 ct**. The
knob midpoint is 100 ct for fine low-range adjustment. Blend fades additional
unison voices toward one tuned center, including even voice counts; 100% retains
the previous unison result. All four source engines are supported.

**Settings → Appearance → Fix MIDI velocity to maximum** sets synthesized and
generated Note On velocity to 127, including arp MIDI output. Off by default,
with normal dynamics retained. The setting is saved with the instance/patch.

**Graphic EQ** has 16 band slots with Add/Remove, independent enable, frequency,
Q and gain. Each band chooses Bell, Low shelf, High shelf, High-pass, Low-pass,
Notch, Band-pass or All-pass. The plot and selected-band inspector give labels
separate space. Ten initial Bell bands preserve older presets. Legacy remains.

**About** is scrollable and includes the requested credits for Logik, coldwave,
Moander and bokkenmmb, plus the open-source projects used.

All 954 previous host parameter definitions remain unchanged. New wide-detune,
blend, paired-choice, velocity and EQ controls are appended, for 1053 total.
The manual explains companion parameters for DAW automation. Editor reopening
also waits for all combined-control bindings to be initialized.

Validation requires 17 Linux / 15 Windows and macOS suites, plus Windows
AddressSanitizer. New tests measure actual 400 ct frequencies, unison blend,
all filter shapes at three rates, live EQ transitions, MIDI velocity, exact
credits, sixteen band slots, presets and original parameter definitions.
Prepared spectral center streams, tail loops, blend and EQ changes allocate no
audio memory in regression tests. The original A/B fixtures remain byte-identical
locally with the additions at their defaults.

Windows/Linux publish first; macOS Universal follows in the same release.
VST3, CLAP and Standalone, English manual, MIT and upstream notices are included.
Windows is unsigned; macOS is ad-hoc signed. VST2 requires a separately licensed SDK.
