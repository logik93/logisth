# logisth native release checks

## Version 1.0.1

The release workflow builds Windows x64 and Linux x86_64 first. Windows AddressSanitizer runs the same native test suite before publication. macOS Universal builds afterward and joins the same release. The workflow writes the native results into each package's BUILD_INFO.json.

The local 48 kHz held-note measurement produced zero sample differences after a same-rate restart for Sampler, Cloud and Granulizer. The published 0.9.8.3 VST3 reproduced the interruption in the binary-host fixture. [Engineering review](docs/REVIEW-v1.0.1.md)

[Native workflow](https://github.com/logik93/logisth/actions/workflows/build.yml) · [Version 1.0.1 downloads](https://github.com/logik93/logisth/releases/tag/v1.0.1)
