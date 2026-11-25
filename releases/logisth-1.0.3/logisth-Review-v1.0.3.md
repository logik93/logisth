# logisth 1.0.3 engineering review

## Reproduction

Hold a note while the host runs realtime audio at 48 kHz. Supply a stopped transport report on every block for the live MIDI case. For the second case, omit the VST3 ProcessContext starting with the first note-on. Suspend the first instrument using setProcessing(false) and setActive(false). Create a second instrument and play the same key there. Resume the first instrument with a larger maximum block size and compare its stereo output to an uninterrupted reference.

The published Linux 1.0.2 VST3 produced silence after the restart in both cases. In the Classic fixture, the uninterrupted block peaked at approximately 0.235 while the restarted block peaked at zero. The processor required an earlier positive transport report before it would defer the reset.

## Verification

The realtime VST3 suspension now defers the reset of an already prepared instrument. A reported stop from playback or a seek detected on resume clears the pending voices. The first offline block also clears a deferred realtime reset. An offline suspension resets immediately.

The `vst3_binary` CTest exercises the shipped module for Classic, Sampler, Cloud and Granulizer. It compares each resumed stereo block to an uninterrupted reference and checks that a note-off and destruction in the second instrument leave the first note sounding. The first instrument then receives its own note-off at sample 37. The release envelope must still match in both channels. Other checks exercise a reported stop, transport seek, clock rebasing and fresh offline bounces at the same sample rate. The test host controls the callback sequence directly.

The `processor_tests --continuity /absolute/output/directory` fixture exports [stereo measurements](continuity/after-1.0.3.csv) for the earlier graph restart and sample-import scenarios. The separate CLAP binary fixture covers activation and same-rate preparation.
