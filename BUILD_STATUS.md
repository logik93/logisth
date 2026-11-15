# logisth native release checks

Current candidate: **0.9.5**. Authoritative receipts are recorded on
`ci-release-v0.9.5`, per-platform `ci-build-v0.9.5-*` branches and
`ci-check-v0.9.5-windows-asan`.

Required: 15 Linux suites, 13 Windows/macOS suites, no failures or skips.
Windows AddressSanitizer must pass before publication. Windows/Linux publish
first; macOS Universal starts afterward and appends to the same release.

New coverage includes Part C routing/four engines, full-first-pass sampler tail
loops with reverse/eight playback modes, all 954 parameter preset round-trips,
a frozen 736-parameter host contract and 20 legacy A/B audio/voice fixtures.
Local Linux passed 15/15 CTest suites, including actual VST3/CLAP loads; the
20 A/B reference renders are byte-identical. Nine publisher/version tests and
theme/font/upstream audits passed. Native package metadata and public SHA256SUMS
are verified after publication.

Previous completed release: [v0.9.4.1](https://github.com/logik93/logisth/releases/tag/v0.9.4.1),
with Linux 12, Windows/macOS 10 and Windows AddressSanitizer 10 passing suites.
macOS runtime tests execute on Intel; arm64/x86_64 slices are both verified.
Windows is unsigned; macOS is ad-hoc signed. VST2 needs a licensed SDK and is excluded.
