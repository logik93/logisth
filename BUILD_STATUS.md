# logisth native release checks

Completed release: **[v0.9.8.3](https://github.com/logik93/logisth/releases/tag/v0.9.8.3)**.
[Native CI run](https://github.com/logik93/logisth/actions/runs/44382965274)
passed from tag commit `d77e5de8c02cf6aa3fc939dfba49fdeed97bcdc6`.

| Platform/check | Passed | Failed | Skipped |
| --- | ---: | ---: | ---: |
| Linux x86_64 | 20 | 0 | 0 |
| Windows x86_64 | 18 | 0 | 0 |
| macOS Universal | 18 | 0 | 0 |
| Windows AddressSanitizer | 18 | 0 | 0 |

Windows/Linux published first with 79 files. macOS Universal was appended to the
same release, bringing the total to 81. Every public asset was downloaded and
its SHA-256 verified. Checks covered native binaries/architectures, full version
and new feature markers, source fingerprints/archive, both English manuals,
exact SVG/AP artwork, bundled fonts/themes/backgrounds and measurement bundles.
Appending macOS preserved identifiers and bytes of every initial asset except
the expanded checksum list. Published older tags and assets remain unchanged.

The new Distortion routing supports 1–4 LR24/TPT bands, smoothed logarithmic
crossovers, per-band modules/bypass and independent pre/post spectrum controls.
Legacy remains sample-identical in all five model fixtures. All 1084 prior host
parameter definitions retain their positions; five new controls append at the
end. Actual runtime reconstruction covers 120 rate/count/frequency cases, with
maximum magnitude error 2.61472848319e-7 dB. DC cutoff sweeps measure zero error;
the topology-transition maximum adjacent-sample jump is 0.00515945255756 in its
specified fixture. Isolation, stereo, bypass/idle CPU, no-allocation, preset/state,
real GUI gestures and capture-off/hidden/close checks also pass. These are bounded
fixtures, not exhaustive claims about every signal or host.

All ten palettes test visible panel/canvas gradient changes. Grid 0/50/100 tests
line intensity; held pointer drags cross refreshes without hiding controls.
Gradient/grid amounts round-trip through drafts and portable version-3 themes.
Fourteen new GUI PNGs and three CSVs are included in the multiband-review bundle.
ABOUT includes mixedbytrzy's requested credit. Original logo/background/font
source bytes remain unchanged. Existing compact, typography, convolution and
rack regressions remain active on the native builds.

Local validation passed all twenty C++ suites, ten Python tool tests, workflow
validation, exact theme/artwork checks, source/manual identities and PDF bounds.
The 47 gain-calibration and 96 factory-preset profiles are byte-identical to
v0.9.8.2. Updated English manuals contain 26 user and 28 technical pages.

Receipts are in `ci-release-v0.9.8.3`, `ci-build-v0.9.8.3-*` and
`ci-check-v0.9.8.3-windows-asan`. Formats are VST3, CLAP and Standalone on all
three platforms. macOS runtime tests execute on Intel; both Universal slices
are checked. Linux records glibc 2.35. The historical v0.9.8.1 native Windows
glyph probe remains attached to unchanged-renderer evidence; current native
suites repeat its offscreen matrix and the new interface/DSP checks.
