# logisth 1.0.4 native release

The published tag points to `012c81d20d1185585b47936b377abb75f290869e`.
Windows and Linux were published first. The macOS Universal package was then
added to the same release, preserving the Windows and Linux archives.

| Platform or check | Passed | Failed | Skipped |
| --- | ---: | ---: | ---: |
| Linux x86-64 | 20 | 0 | 0 |
| Windows x86-64 | 19 | 0 | 0 |
| Windows AddressSanitizer | 19 | 0 | 0 |
| macOS Universal | 19 | 0 | 0 |

Each package includes VST3, CLAP and Standalone. The published archive checksums
were verified, and the macOS binaries contain both Intel and Apple Silicon slices.
The VST3 smoke test reproduced the measured pitch shifts on the downloaded
Linux release binary.

[Release downloads](https://github.com/logik93/logisth/releases/tag/v1.0.4) · [CI build and tests](https://github.com/logik93/logisth/actions/runs/45693619179) · [VST3 frequency verification](docs/REVIEW-v1.0.4.md)
