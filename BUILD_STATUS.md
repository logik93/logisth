# logisth 1.0.1 native release

The published tag is `f88437927808ac0ee4d34d74a71e2d0ca8d50b5d`. All three native packages include VST3, CLAP and Standalone. [Release downloads](https://github.com/logik93/logisth/releases/tag/v1.0.1) · [Build and test run](https://github.com/logik93/logisth/actions/runs/45456076887)

| Platform or check | Passed | Failed | Skipped |
| --- | ---: | ---: | ---: |
| Linux x86_64 | 20 | 0 | 0 |
| Windows x86_64 | 19 | 0 | 0 |
| Windows AddressSanitizer | 19 | 0 | 0 |
| macOS Universal | 19 | 0 | 0 |

Windows and Linux were published first with 79 assets. macOS Universal was added to the same release, bringing the total to 81. A download check verified every SHA-256, the native binary architectures, the source archive and the two English manuals. The macOS package contains Intel and Apple Silicon executables. Each operating system exercised the compiled VST3 with a held MIDI note during a two-instance graph restart.

In the processor fixture, both stereo channels matched the uninterrupted reference sample for sample for Sampler, Cloud and Granulizer. The published 0.9.8.3 VST3 reproduced the original interruption. [Engineering review and measurements](docs/REVIEW-v1.0.1.md)
