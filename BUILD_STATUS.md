# logisth native release checks

Current candidate: **0.9.4**. The tagged release workflow records authoritative
results on `ci-release-v0.9.4`, with per-platform build receipts and
`ci-check-v0.9.4-windows-asan` for memory regressions.

Required: 12 Linux suites, 10 Windows/macOS suites, no failures or skips; Windows
AddressSanitizer must also pass. Native packages contain VST3, CLAP and Standalone.
Windows/Linux publish first. macOS Universal starts after publication and appends
to that release. Architecture, source identity and public SHA256SUMS are checked.

Local validation for this candidate: 12/12 CTest suites passed; 6/6 publisher
regressions passed. The 48-patch bank passed 96 full-velocity auditions, with
a highest measured peak of -3.05 dBFS in that fixture. All 48 patches were saved
and restored independently. Native receipts become authoritative after CI completes.

Previous completed release: [v0.9.3](https://github.com/logik93/logisth/releases/tag/v0.9.3),
with Linux 11, Windows/macOS 9 and Windows AddressSanitizer 9 passing suites.
macOS runtime tests execute on Intel; both arm64/x86_64 slices are verified.
Windows is unsigned; macOS is ad-hoc signed, not notarized. VST2 needs a licensed
SDK and is excluded.
