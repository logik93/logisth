# logisth native release checks

Current candidate: **v0.9.8.1**. Windows/Linux publish first; macOS Universal
is appended afterward. Gates: 20 Linux / 18 Windows and macOS suites plus
18 Windows ASAN suites, no failures/skips.

Local verification passed all 20 suites and ten publisher regressions. New
appearance evidence covers 96 glyph scale/DPI/quality/Off cases with exact vector
pixel parity, edge weight, transparent text replacement, folder controls,
portable PNG/JPEG/grid backgrounds and all ten popup palettes. The full processor
suite, 48-WAV convolution review, 47 gain profiles, 48-preset bank, kick benchmark
and 120 theme/knob renders passed. Source audio algorithms and all 1084 host
parameters are unchanged. Both English manuals and screenshots are updated.

Completed release: **[v0.9.8](https://github.com/logik93/logisth/releases/tag/v0.9.8)**.
[Native CI run](https://github.com/logik93/logisth/actions/runs/44140883316)
passed from tag commit `452596fac14ce0457dcd3dccdedca84991900094`.

| Platform/check | Passed | Failed | Skipped |
| --- | ---: | ---: | ---: |
| Linux x86_64 | 20 | 0 | 0 |
| Windows x86_64 | 18 | 0 | 0 |
| macOS Universal | 18 | 0 | 0 |
| Windows AddressSanitizer | 18 | 0 | 0 |

Windows/Linux published first with 69 files. macOS Universal was appended to
the same release, bringing the total to 71. Every public asset was downloaded
and its SHA-256 verified. Native binaries, both Universal architecture slices,
version metadata, source fingerprints and archive, both English manuals,
notices, exact SVG/AP artwork, fonts/themes and all measurement bundles were
checked. Appending macOS preserved the identifiers and bytes of all initial
assets except the expanded checksum list.

Receipts are in `ci-release-v0.9.8`, `ci-build-v0.9.8-*` and
`ci-check-v0.9.8-windows-asan`. All platforms provide VST3, CLAP and Standalone.
macOS runtime tests execute on Intel; both Universal slices are checked.

New coverage includes four-stage distortion with all 24 orders and per-slot
switches, chorus layout in Sync/free modes, calibrated pre/post stereo-power
FFT and display tilts, 48 actual-WAV convolution cases and two sampler
integrations, glyph-mask AA, seven Settings categories and persistent font/folder
preferences. All 1084 controls are reachable; the prior 1071 host definitions
remain intact. The release includes 21 WAV review fixtures and measurement CSVs.
[GUI/DSP evidence](docs/GUI_DSP_REVIEW.md) records methods and limits.

Previous completed release: **[v0.9.7](https://github.com/logik93/logisth/releases/tag/v0.9.7)**.
[Native CI run](https://github.com/logik93/logisth/actions/runs/43963656614)
passed from tag commit `7975cb668c01b4c13206fdb6e69319b7926b4330`.

| Platform/check | Passed | Failed | Skipped |
| --- | ---: | ---: | ---: |
| Linux x86_64 | 20 | 0 | 0 |
| Windows x86_64 | 18 | 0 | 0 |
| macOS Universal | 18 | 0 | 0 |
| Windows AddressSanitizer | 18 | 0 | 0 |

Windows/Linux published first with 63 files. macOS Universal was appended to
the same release, bringing the total to 65. Every public asset was downloaded
and its SHA-256 verified. Native binaries, source fingerprints, architecture
slices, version metadata, source archive, both English manuals, notices,
font/theme bytes and measurement bundles were checked. Appending macOS preserved
the identifiers and bytes of all initial assets except the expanded checksum list.

Receipts are in `ci-release-v0.9.7`, `ci-build-v0.9.7-*` and
`ci-check-v0.9.7-windows-asan` branches. VST3, CLAP and Standalone are available
for all three platforms.

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
