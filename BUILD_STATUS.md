# logisth native release checks

Current candidate: **0.9.6.2**. Receipts: `ci-release-v0.9.6.2`,
`ci-build-v0.9.6.2-*` and `ci-check-v0.9.6.2-windows-asan`.
Required: 19 Linux / 17 Windows/macOS suites, no failures/skips, plus Windows ASAN.
Windows/Linux publish first; macOS Universal appends afterward.

New tests cover dynamics, knee curves, antialiasing, exact post-Main ceilings,
stereo linking, allocation guards, presets, graph UI, ordered tabs, IR waveform
and drag/drop, idle/tail behavior, transitive FM/AM and MIDI-only rendering.
The complete 1055-parameter contract is retained; 15 fields are appended.

Previous completed release: v0.9.6.1. All three native platforms passed
18 Linux / 16 Windows/macOS suites plus Windows ASAN. All 54 public assets,
checksums, architecture slices, native metadata, source archives, manuals,
themes, fonts and measurement bundles were downloaded and verified.

macOS runtime tests execute on Intel; both Universal slices are checked.
Windows is unsigned; macOS is ad-hoc signed. VST2 needs a licensed SDK.
