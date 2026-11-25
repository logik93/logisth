# logisth 1.0.4 native release

The release workflow tests Windows x86-64 and Linux x86-64 before publishing
their packages, then builds and appends macOS Universal to the same release.
The Windows AddressSanitizer check also gates the first publication. Each
platform package includes VST3, CLAP and Standalone.

[Release downloads](https://github.com/logik93/logisth/releases/tag/v1.0.4) · [Live CI status](https://github.com/logik93/logisth/blob/ci-release-v1.0.4/status.json) · [VST3 frequency verification](docs/REVIEW-v1.0.4.md)
