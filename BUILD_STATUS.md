# logisth native release checks

Current candidate: **0.9.4.1**. The tagged release workflow records authoritative
results on `ci-release-v0.9.4.1`, with per-platform build receipts and
`ci-check-v0.9.4.1-windows-asan` for memory regressions.

Required: 12 Linux suites, 10 Windows/macOS suites, no failures or skips; Windows
AddressSanitizer must also pass. Native packages contain VST3, CLAP and Standalone.
Windows/Linux publish first. macOS Universal starts after publication and appends
to that release. Architecture, source identity and public SHA256SUMS are checked.

This candidate validates four supplied JSON files and unmodified Ioskeley fonts,
all ten themes, live text restyling, search dismissal and four-part publication.
Local validation: 12/12 CTest suites passed, including actual CLAP/VST3 binaries.
Theme/search checks and nine publisher/version regressions passed. Fresh
96-profile factory measurements remain below -3 dBFS in the audition fixture.
Authoritative results are stored in the native release receipts after CI completes.

Previous completed release: [v0.9.4](https://github.com/logik93/logisth/releases/tag/v0.9.4),
with Linux 12, Windows/macOS 10 and Windows AddressSanitizer 10 passing suites.
macOS runtime tests execute on Intel; both arm64/x86_64 slices are verified.
Windows is unsigned; macOS is ad-hoc signed, not notarized. VST2 needs a licensed
SDK and is excluded.
