# logisth native release checks

Current candidate: **0.9.6.1**. Native receipts are recorded on
`ci-release-v0.9.6.1`, `ci-build-v0.9.6.1-*` and `ci-check-v0.9.6.1-windows-asan`.

Required: 18 Linux suites, 16 Windows/macOS suites, without failures or skips.
Windows AddressSanitizer must pass before initial publication. Windows/Linux
publish first; macOS Universal appends afterward to the same release.

New checks cover all-six-player stereo correlation/Mid-Side levels, full Pan,
zero-latency convolution/direct FIR parity, resampled normalization, embedded IR
preset/project recall, live replacement, audio allocations, all oscillator
clipboard combinations and symmetric macro menus. A new complete 1055-parameter
contract freezes this release's deliberately revised definitions.

Previous completed release: [v0.9.6](https://github.com/logik93/logisth/releases/tag/v0.9.6),
verified on all platforms with 17 Linux, 15 Windows/macOS and 15 Windows ASAN
suites passed. Its 52 public assets and checksums were downloaded and verified.
Current native results are authoritative in the release receipts after CI.

macOS runtime tests execute on Intel; both arm64/x86_64 slices are checked.
Windows is unsigned; macOS is ad-hoc signed. VST2 requires a licensed SDK.
