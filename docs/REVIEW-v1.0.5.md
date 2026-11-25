# logisth 1.0.5 — FL Studio Master Pitch

The regression harness loads the VST3 module and connects its edit controller
to a host stub that reports FL Studio through IHostApplication. It holds
MIDI note 60 at 48 kHz and sends 14-bit wheel positions through IMidiMapping.
The 1.0.4 development build with automatable wheels measured 523.251 Hz
when the FL Studio profile example called for +200 cents (293.665 Hz).

The earlier conversion used the plug-in's 12-semitone range for a wheel
endpoint without sensitivity metadata. FluidSynth initializes MIDI wheel
sensitivity to two semitones and applies RPN 0,0 as explicit sensitivity.
Vital scales normalized MIDI wheel position by the patch bend range. Surge
XT also reads RPN 0,0 for pitch bend sensitivity in its MPE handling.
Logisth uses the two-semitone fallback specifically for FL Studio and accepts
an explicit sensitivity from any host.

- [FluidSynth channel initialization](https://github.com/FluidSynth/fluidsynth/blob/master/src/synth/fluid_chan.c) · [FluidSynth RPN handling](https://github.com/FluidSynth/fluidsynth/blob/master/src/synth/fluid_synth.c)
- [Vital MIDI handling](https://github.com/mtytel/vital/blob/main/src/common/midi_manager.cpp) · [Vital parameters](https://github.com/mtytel/vital/blob/main/src/common/synth_parameters.cpp)
- [Surge XT RPN handling](https://github.com/surge-synthesizer/surge/blob/main/src/common/SurgeSynthesizer.cpp)

The test distinguishes a FL Studio two-semitone endpoint from the intermediate
position for the same ±200-cent shift at an explicit twelve-semitone range.
It also covers ±1200 cents with that range, other host profiles, and
automation of the two plug-in wheels. Its tolerance is one cent.

## Measured Linux VST3 output

| FL Studio profile | Requested shift | Down | Up |
| --- | --- | ---: | ---: |
| No RPN, wheel endpoint | ±200 cents | 233.082 Hz | 293.665 Hz |
| RPN ±12 semitones, intermediate wheel position | ±200 cents | 233.088 Hz | 293.661 Hz |
| RPN ±12 semitones, wheel endpoint | ±1200 cents | 130.813 Hz | 523.251 Hz |

The intermediate positions are quantized to a 14-bit MIDI wheel value.
The harness models both input encodings and checks output from the actual
VST3 module. It does not capture events from a running copy of FL Studio.
