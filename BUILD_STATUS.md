# logisth 1.0.2 native release

The published tag is `71789d7f5f40059d8775c51a6c4f2b295d469a7d`. All three native packages include VST3, CLAP and Standalone. [Release downloads](https://github.com/logik93/logisth/releases/tag/v1.0.2) · [Build and test run](https://github.com/logik93/logisth/actions/runs/45503819696)

| Platform or check | Passed | Failed | Skipped |
| --- | ---: | ---: | ---: |
| Linux x86_64 | 20 | 0 | 0 |
| Windows x86_64 | 19 | 0 | 0 |
| Windows AddressSanitizer | 19 | 0 | 0 |
| macOS Universal | 19 | 0 | 0 |

Windows and Linux were published first with 79 assets. macOS Universal was added to the same release, bringing the total to 81. A download check verified every SHA-256, the native binary architectures, the source archive and the two English manuals. The macOS package contains Intel and Apple Silicon executables. The native VST3 tests exercised held MIDI notes during a graph restart, missing timing reports and a sample-counter rebase. They also checked an incoming note-off during a missing report and explicit transport stops and seeks.

In the processor fixture, both stereo channels matched the uninterrupted reference sample for sample for Sampler, Cloud and Granulizer. The published 1.0.1 Linux VST3 reproduced the sequenced-note interruption in the host fixture. The older 0.9.8.3 VST3 also reproduced the separate two-instance graph-restart issue. [Engineering review and measurements](docs/REVIEW-v1.0.2.md)
