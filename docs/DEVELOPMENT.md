# Maintainer workflow

The project owner requested on 2026-11-14: after each completed set of requested
changes, compile and publish a new GitHub release automatically ("rób to za każdym
razem"). Continue through native builds and verification without asking again.
A later explicit instruction to wait or avoid publishing overrides this default.

1. Finish the changes and appropriate regression checks. Keep producer wavelogik,
   product logisth, original-code MIT licensing and third-party notices consistent.
2. Set the next unused CMake version and update README/manual/release notes in
   English. Never replace a published version tag or published release assets. A failed,
   unpublished candidate may be repaired only after verifying no public release
   exists, retaining its original commit and using a lease on the exact tag ref.
3. Build locally and verify the change. Export current preview/demo assets with
   `build/processor_tests /absolute/preview-folder` and calibration materials with
   `build/processor_tests --gain-check /absolute/calibration-folder`.
4. Refresh documentation images and the PDF (`python scripts/build_manual.py`).
   Commit source/documentation before creating the source archive.
5. Run `python scripts/package_release.py --preview /absolute/preview-folder
   --calibration /absolute/calibration-folder --kick-benchmark /absolute/kick-folder --factory-bank /absolute/factory-folder`, then `python scripts/verify_release.py
   v<version>`. Source packages include LICENSE, notices, theme templates and scripts.
6. Commit the new release asset folder, create an annotated unused `v<version>`
   tag, and push the branch/tag atomically. The existing native workflow builds
   VST3, CLAP and Standalone on Windows x64, Linux x64 and macOS Universal.
7. Publish the tested Windows x64 and Linux x64 packages together first. Only
   after that public release exists, start the macOS Universal build and append
   its package and build metadata to the same release. Preserve existing download
   IDs and bytes; only SHA256SUMS and the release description are cumulative.
   Wait for every native build/test/architecture check and publication to finish.
   Current minimum suites: Linux 17, Windows/macOS 15, with no failures or skips.
   A separate Windows AddressSanitizer build must also pass before publication;
   its binaries are diagnostic only. Its receipt is `ci-check-v<version>-windows-asan`.
   Read the status branches `ci-build-v<version>-<platform>` and
   `ci-release-v<version>`. If branch HTTP responses are stale, resolve their
   current Git SHA and read status.json at that immutable revision.
8. Download the public release assets, check SHA256SUMS and binary/build metadata,
   both macOS architectures, English manual, MIT/third-party notices and all bundled
   theme templates. Report completion with the release link only after verification.

VST2 remains optional and requires a separately licensed SDK. macOS packages are
ad-hoc signed, not notarized; Windows builds are unsigned. Preserve upstream
library/SDK/font licenses independently of the original-code MIT license.

The owner resumed automatic small 0.9.x releases on 2026-11-15. Keep Windows/Linux
first and append macOS afterward. Do not advance to 1.0.0 without a later request.

Version 0.9.4.1 uses a fourth release component. Tags and scripts support either
three or four numeric parts. Keep full plug-in strings; Apple marketing versions
use major.minor.patch, and build versions use (major*100+minor).patch.revision.
Always verify theme/font provenance with `python scripts/verify_theme_assets.py`.
