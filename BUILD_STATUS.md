# logisth 1.0.3 native release

The published tag is `583b51be6c36880b1e7b2f398bf2e2c236d7b6dd`. All three native packages include VST3, CLAP and Standalone. [Release downloads](https://github.com/logik93/logisth/releases/tag/v1.0.3) · [Build and test run](https://github.com/logik93/logisth/actions/runs/45565418125)

| Platform or check | Passed | Failed | Skipped |
| --- | ---: | ---: | ---: |
| Linux x86_64 | 20 | 0 | 0 |
| Windows x86_64 | 19 | 0 | 0 |
| Windows AddressSanitizer | 19 | 0 | 0 |
| macOS Universal | 19 | 0 | 0 |

Windows and Linux were published first with 79 assets. macOS Universal was added to the same release, bringing the total to 81. A download check verified every SHA-256, the native binary architectures, the source archive and the two English manuals. The macOS package contains Intel and Apple Silicon executables. The native VST3 tests exercised held MIDI notes during graph restarts with playing, stopped and missing transport. They also checked independent note-offs, transport stops, seeks and offline resets.

In the processor fixture, both stereo channels matched the uninterrupted reference sample for sample for Sampler, Cloud and Granulizer. The published 1.0.2 Linux VST3 reproduced both newly covered cases. [Engineering review and measurements](docs/REVIEW-v1.0.3.md)
