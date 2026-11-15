# logisth native release checks

Current candidate: **0.9.6**. Native receipts are recorded on
`ci-release-v0.9.6`, `ci-build-v0.9.6-*` and `ci-check-v0.9.6-windows-asan`.

Required: 17 Linux suites, 15 Windows/macOS suites, without failures or skips.
Windows AddressSanitizer must pass before initial publication. Windows/Linux
publish first; macOS Universal appends afterward to the same release.

New checks cover 400 ct frequencies, six blend controls and their tuned centers,
paired A+C/B+C routes, fixed MIDI/arp velocity, scrollable credits, eight EQ filter
shapes across three rates, sixteen independent bands, live transitions and
allocation-free processing. Frozen contracts retain all 954 previous host
parameter definitions; original A/B audio/voice regression captures remain active.

Previous completed release: [v0.9.5](https://github.com/logik93/logisth/releases/tag/v0.9.5),
verified on all platforms with 15 Linux, 13 Windows/macOS and 13 Windows ASAN
suites passed. Its 50 public assets and checksums were downloaded and verified.
Local Linux passed 17/17 CTest suites, including actual VST3/CLAP loads. All
115 destination menu entries round-trip. Gain calibration and 96 factory profiles
are unchanged from 0.9.5; nine publisher/version tests and provenance checks passed.
Current native results are authoritative in the release receipts after CI.

macOS runtime tests execute on Intel; both arm64/x86_64 slices are checked.
Windows is unsigned; macOS is ad-hoc signed. VST2 requires a licensed SDK.
