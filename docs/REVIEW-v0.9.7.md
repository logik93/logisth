# logisth v0.9.7 — engineering review

The rack previously represented a fixed processing path. It now uses validated
stage identities and one atomic saved permutation, with the same order driving
both the rail and the DSP. Chords/Arp and editing tools are separated from audio
stages. Main and final sample-peak protection remain fixed. The active page is
stable through a drag, and Sample is the initial page of a new editor.

Thirty-two-note storage and spectral indexing replace the prior sixteen-note
limit. Default rendering remains sixteen, with no waveform work for the extra
inactive voices. One companion parameter preserves every prior host definition.
Complete frozen metadata, voice stealing, reduction of active polyphony, all
source modes and prepared spectral voice 32 are regression-tested.

The review found and fixed two additional issues: serial IR/Delay gaps could be
shortened by an idle horizon based only on the largest individual delay, and
composite UI attachments could read the preceding raw host value. Tests now
cover the combined response horizon and immediate display of incoming values.

Light screen controls used the wrong face colors for their light needles.
Surface-aware cached colors repair that case and add contrast checks across
all ten themes, three styles and normal/screen/trim contexts. Slightly stronger
needles remain visible after rasterization. Supplied palettes/fonts stay intact.

[Rack, polyphony and skin evidence](RACK_REVIEW.md) describes the fixtures and
their limits. [Technical Manual](TECHNICAL_MANUAL.md) documents architecture,
DSP, state, dependencies and the release process. Existing oversampling, memory
and host-coverage limitations remain explicit; this release does not claim
exhaustive behavior across every DAW or arbitrary custom palette.
