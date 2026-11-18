# logisth native release checks

Release candidate: **v0.9.8.3**. Local Linux release checks cover all twenty C++
suites, ten Python tool regressions, exact bundled themes/fonts/backgrounds,
workflow validation, current source/manual identities and PDF text bounds.
The new runtime crossover matrix has 120 reconstruction cases, two isolation
cases and continuity/Legacy parity evidence. Fourteen interface PNGs accompany
pointer, preset, capture and allocation checks. The 47 gain and 96 factory
profiles remain byte-identical to v0.9.8.2. User/technical manuals contain 26/28
pages. Native Windows/Linux publication and subsequent macOS append are pending
at this candidate snapshot; the completed prior release is recorded below.


Completed release: **[v0.9.8.2](https://github.com/logik93/logisth/releases/tag/v0.9.8.2)**.
[Native CI run](https://github.com/logik93/logisth/actions/runs/44352067388)
passed from tag commit `06ac787b53f3188888b32ec514e569e4c18c39bb`.

| Platform/check | Passed | Failed | Skipped |
| --- | ---: | ---: | ---: |
| Linux x86_64 | 20 | 0 | 0 |
| Windows x86_64 | 18 | 0 | 0 |
| macOS Universal | 18 | 0 | 0 |
| Windows AddressSanitizer | 18 | 0 | 0 |

Windows/Linux published first with 77 files. macOS Universal was appended to the
same release, bringing the total to 79. Every public asset was downloaded and
its SHA-256 verified. Checks covered native binaries and architectures, version
metadata, source fingerprints/archive, both English manuals, exact SVG/AP art,
fonts/themes, all twenty original backgrounds, PNG integrity and measurement
bundles. Appending macOS preserved the identifiers and bytes of every initial
asset except the expanded checksum list.

The MODULATION disclosure folds the complete lower modulation row and changes
the actual editor from 1240 x 860 to 1240 x 640 at 100%. Coverage includes the
first mouse click across refresh, visible bounds, parameter reachability, 100
palette/scale/layout combinations, uniform transforms, hidden automation,
persisted fractional scale/layout and editor recreation. All twenty embedded
PNGs were decoded and selected through the real menu, round-tripped through the
theme codec and saved/reloaded without external image paths. The top edge line
is removed. AP artwork and the project-owner SVG retain their original hashes.

The 47 gain-calibration profiles and 96 factory-preset profiles are byte-for-byte
identical to v0.9.8.1. Existing glyph, real-WAV convolution, sampler, gain,
factory-bank, rack/contrast and memory regressions remain active. Local release
verification also passed all ten Python tool tests, theme/artwork/manual checks
and workflow validation. English manuals contain 26 user and 27 technical pages.

Receipts are in `ci-release-v0.9.8.2`, `ci-build-v0.9.8.2-*` and
`ci-check-v0.9.8.2-windows-asan`. Formats are VST3, CLAP and Standalone on all
three platforms. macOS runtime tests execute on Intel; both Universal slices
are checked. The historical v0.9.8.1 native Windows glyph probe remains attached
to the unchanged-renderer evidence; current native suites repeat the offscreen
matrix and new compact-layout/background checks.
