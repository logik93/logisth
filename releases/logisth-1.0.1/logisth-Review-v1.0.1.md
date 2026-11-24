# logisth 1.0.1 engineering review

## Held-note reproduction

Import a stereo recording, enable looping, send one note-on at 48 kHz and leave it held. Render a control instance alongside the track. Open another instance, play it, replace its material and close it. Then suspend and prepare the first track again at 48 kHz with a larger maximum block. Compare both stereo streams sample by sample without sending another note-on.

In the 0.9.8.3 processor fixture, opening the second editor produced zero difference. The subsequent graph rebuild silenced the first track. Reference peaks for Sampler, Cloud and Granulizer were 0.118298, 0.117664 and 0.0887175 respectively. The released Linux VST3 also lost the note in the binary host test.

The corrected processor preserves its voices, sample cursors, envelopes and effect histories across compatible preparations. For VST3, a processing suspension resumes a held voice when the next playing block begins at exactly the expected project sample. A seek, stopped transport, missing clock after suspension or changed sample rate clears suspended playback. Explicit incoming MIDI panic messages retain their channel and sample offset. The callback uses fixed per-instance MIDI tracking.

## Reproduction tools

- Run `processor_tests --continuity /absolute/output/directory` for the stereo measurements, trim-handle image, host parameter mapping, mouse gesture checks, MIDI ownership and sample import ordering.
- Run the CTest suite named `vst3_binary` to exercise the shipped module with two instruments, a held note, a block-size change, a seek and a stop. The fixture covers all three sample playback models and compares both channels to an uninterrupted instance.
- The Linux `clap_binary` test activates a second instrument and changes the first one's maximum block size while its sampler holds a note.

The host harness reproduces the reported transport sequence. An interactive session in the reporting DAW is outside this measurement. Before-and-after numerical results accompany the source and the native packages in `docs/continuity/`.

## Result at 48 kHz

| Player | Reference peak | 0.9.8.3 peak after restart | 1.0.1 peak after restart | Maximum 1.0.1 stereo sample difference |
| --- | ---: | ---: | ---: | ---: |
| Sampler | 0.118298 | 0 | 0.118298 | 0 |
| Cloud | 0.117664 | 0 | 0.117664 | 0 |
| Granulizer | 0.0887175 | 0 | 0.0887175 | 0 |

The explicit processor reset produced silence in all three cases. The binary VST3 test also checks that a transport stop or seek releases the held voice. [Before](continuity/before-0.9.8.3.csv) · [After](continuity/after-1.0.1.csv) · [Adjacent waveform handles](continuity/adjacent-trim-handles.png)

Selecting a multiband Distortion band count now updates the corresponding crossover controls on the same click. The regression checks each band count before any timer refresh. The earlier 1.0.0 source tag records the candidate that exposed this issue during Windows validation.
