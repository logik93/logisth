# logisth v0.9.5

Part C adds C1/C2 players, Filter C, Amp Env C, Mix C, Solo C and a C scope view.
Each player supports Classic, Wavetable, Sampler and Granular, source envelopes,
FM/AM/PWM, imported material and sampler stretching. Global Filter Control
also affects C. Every filter accepts A/B/C source combinations or None.

The modulation matrix adds independent C1/C2, part C and A+B+C destinations.
A+B continues to mean the original A/B players. C envelope sources, FM/AM
modulators and macro targets complete the routing.

Sampler **LOOP TAIL** plays the complete selected sound once, then repeats an
independent tail region. Enable it in the player view menu, set IN/OUT and
CROSSFADE, or Alt-drag the region in SAMPLE. Reverse, stretching and resampling
are supported. The main sample selection remains separate.

All players now offer **0–200 ct detune**. The first 736 host parameters retain
their IDs, indices, ranges, defaults and choice lists. Additive detune and C
choice companions are appended, keeping older normalized automation intact.
Older presets restore silent C, tail loops off and zero additive detune.
The editor combines companion controls; see the manual for DAW automation.

## Validation

- 15 Linux / 13 Windows and macOS CTest suites required, including new Part C,
  Loop Tail, frozen parameter-contract and legacy A/B rendering checks.
- Windows AddressSanitizer is required before initial publication.
- Twenty A/B fixtures compare audio and voice lifetimes with the frozen
  v0.9.4.1 engine; local Linux output is byte-identical.
- Preset round-trips cover all 954 parameters; every player and new page is
  checked for control visibility and overlap.
- Existing 58 factory entries, levels, theme/font provenance and public package
  checks remain part of release validation.

Windows x64 and Linux x64 publish first; macOS Universal is appended afterward
to the same release. Packages include VST3, CLAP, Standalone, the English manual,
MIT and required upstream notices. Windows is unsigned; macOS is ad-hoc signed.
VST2 requires a separately licensed SDK and is not included.
