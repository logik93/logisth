# logisth 1.0.7

FL Studio Master Pitch uses a ±12-semitone input scale when the host sends
wheel positions without MIDI Pitch Bend Sensitivity. An explicit channel
sensitivity still takes precedence. **BEND / st** limits the applied shift
and defaults to 12 semitones. **PERFORMANCE / Bend** now spans −12 to +12,
with zero at the center. Both Bend and Mod remain automatable.

The VST3 regression exercises ±200, ±300, ±400 and ±1200 cents with and
without an explicit twelve-semitone sensitivity. It also checks that reducing
the plug-in limit to four semitones preserves a three-semitone shift and caps
a six-semitone request at four. The test uses a VST3 host that identifies as
FL Studio. See the [English manual](https://github.com/logik93/logisth/blob/v1.0.7/docs/logisth-Manual-English.pdf) for channel and wrapper settings.
