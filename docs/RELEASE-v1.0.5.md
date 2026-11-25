# logisth 1.0.5

FL Studio MIDI pitch bend now uses a two-semitone fallback when the host
sends only wheel position. MIDI Pitch Bend Sensitivity takes priority, so
a host sending a 12-semitone range can drive the full octave of Master Pitch.
Bend / st defaults to 12 and limits the result to at most 24 semitones.

The **PERFORMANCE / Bend** and **PERFORMANCE / Mod** controls are available
for host automation. Bend travels from −10 to +10 with 0 at the center.
See the English manuals for the FL Studio wrapper settings and
[the VST3 frequency report](https://github.com/logik93/logisth/blob/v1.0.5/docs/REVIEW-v1.0.5.md)
for verification.
