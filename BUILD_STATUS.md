# logisth native release checks

Current candidate: **0.9.7**. Receipts: `ci-release-v0.9.7`,
`ci-build-v0.9.7-*` and `ci-check-v0.9.7-windows-asan`.
Required: 20 Linux / 18 Windows/macOS suites, no failures/skips, plus Windows ASAN.
Windows/Linux publish first; macOS Universal appends afterward.

New checks cover all 40320 effect permutations, actual nonlinear ordering, default
FX parity, serial IR/Delay first responses, live zero-gain transitions, pointer
and keyboard interaction, preset/legacy routing, all four 32-voice source engines,
32-voice spectral preparation, allocation/ceiling guards, ten-theme contrast and
actual raster needles, Sample startup and full layout. All previous 1070 host
definitions remain intact; one polyphony companion is appended.

Previous completed release: v0.9.6.2. All three native platforms passed
19 Linux / 17 Windows/macOS suites plus Windows ASAN. All 58 public assets,
checksums, architecture slices, native metadata, source archives, manuals,
themes, fonts and measurement bundles were downloaded and verified.

macOS runtime tests execute on Intel; both Universal slices are checked.
Windows is unsigned; macOS is ad-hoc signed. VST2 needs a licensed SDK.
