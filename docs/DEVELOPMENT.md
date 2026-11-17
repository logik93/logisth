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
4. Review `docs/TECHNICAL_MANUAL.md` against changed runtime code and set its
   software baseline to the CMake release version. Refresh documentation images
   and both PDFs (`python scripts/build_manual.py`, requires ReportLab).
   Run `python scripts/verify_technical_manual.py`; it checks the version and
   SHA-256 identities of the implementation, Markdown and generated PDF.
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
   Current minimum suites: Linux 20, Windows/macOS 18, with no failures or skips.
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

## v0.9.6.1 compatibility baseline

The stereo mapping and algorithmic reverb output/mix law intentionally change in
this release. Reverb type and macro companion choice ranges extend. The new full
host contract is tests/fixtures/parameters-0.9.6.1.json (1055 entries). Subsequent
releases must retain its IDs, host order, ranges, defaults, integer flags and choices.
Old mono A/B capture regression remains active; deliberate stereo changes have
separate correlation, Mid/Side, level and full-pan tests across all six players.
PreparedImpulse owns JUCE convolution; ImpulseRack crossfades and publishes only
prepared engines. Host sample-rate changes reprepare outside processing. Imported
IR data is immutable, bounded and embedded; retired engines are destroyed outside
the audio callback. Thread-local allocation guards cover actual processing.

## v0.9.6.2 dynamics and idle processing

Dynamics.h contains the shared transfer curves, modern compressor, analytic
clipper/ADAA and final linked sample-peak limiter. Effects owns compressor state;
Processor applies Clipper before Main and Limiter after Main. DynamicsView capture
is opt-in through an atomic selected-page value. Only audio resets/writes the
ring during processing. No heap allocation is introduced in callbacks.
Old states missing compmode use Legacy; original 1055 fields remain unchanged.
Effects' quiet gate accounts for delayed first responses/IR duration. It advances
compressor recovery and free effect LFO phase rather than freezing them. Engine
resolves carrier/dependency needs once per render, including route bounds and
smoothed modulation depths. IR peak caches are immutable/off-thread.

## v0.9.7 rack and voice policy

Eight EffectStage IDs form a validated permutation packed into one atomic 24-bit
value. Store it as optional effectOrder metadata, reject malformed permutations
before state publication, and use the original order for older patches. Chords,
Arp and Sample/Morph tools never enter this audio permutation. Clipper is movable;
Main and the final sample-peak Limiter remain fixed. RackTransition splits work
at a 5 ms fade-down boundary, switches at zero and fades up for 5 ms without
allocating or duplicating a DSP bank. Serial first-response horizons are additive.

maxPolyphony=32 and defaultPolyphony=16 are separate from the sixteen MIDI
channels. The old polyphony parameter range stays frozen; polyphonyextra is
appended and the editor exposes their sum. Preparation and spectral bounds must
use the same effective count. Inner audio rendering visits the configured count,
so default 16 does not generate work for the additional inactive voices.

Do not alter supplied theme JSON to repair contrast. ThemeContrast computes
context/style colors on palette changes; rotary painting uses the cached values.
Core Rack tests include a frozen prior FX implementation, all permutations,
noncommuting audio fixtures, serial IR/Delay gaps and live transition bounds.
Processor tests cover pointer/keyboard routing, metadata, 32 notes/streams,
allocation/ceiling guards, all controls and ten-theme raster/contrast fixtures.

## Technical documentation in every release

The owner requested a professional English technical reference and its continued
inclusion on 2026-11-16. Keep `docs/TECHNICAL_MANUAL.md`, its PDF and
`docs/technical-manual.json` current. The manual identifies original DSP,
open-source dependencies/versions/licenses, data formats, thread ownership,
resource limits and validation evidence. Describe actual implementation and
measurement conditions; do not add comparisons to proprietary products or their
manufacturers. Retain required upstream license texts separately.

`build_manual.py` renders both manuals. `build_native.py` rejects stale technical
documentation and includes all three reference files in native packages.
`package_release.py` also publishes the Markdown, PDF and identity manifest as
separate assets. Release verification requires them after v0.9.6.2. Update the
manual's documentation history when its content changes. A documentation-only
push does not require rebuilding unchanged plug-ins or altering a published tag;
the next product release must carry the updated reference in all packages.

Version 0.9.7 also packages docs/RACK_REVIEW.md and docs/rack/theme-contrast.csv
as a rack-skin-review bundle. Keep all 120 palette/style/context/trim cases,
including enabled needle, arc/track and raster targets. Native packages include
the report and raw CSV. Refresh snapshots and both manuals after the final UI build.

## v0.9.8 GUI/DSP review

Keep the earlier 1071 host definitions frozen. Append the thirteen distortion
controls; preserve Stage 1 modulation and retain independent state for all four
serial stages. DistortionOrder is optional validated metadata, default 1/2/3/4.
Use audio-owned fade state for live changes and skip inactive/zero-Mix stages.

EQ captures pre/post stereo at the actual stage position. Its 2048-point windowed
FFT and display tilt run on the GUI thread; disable capture when hidden/off.
The power calculation must not cancel antiphase signals. UserSettings contains
application-only typography, analyzer and chooser preferences; never read it in
processBlock. Font AA Off must affect glyph coverage, preserving vector AA.
Keep the supplied SVG exact and preserve the AP bitmap hash.

`processor_tests --review-check` exercises real-WAV convolution and GUI/state/
preference tests. `--review-report /absolute/folder` also exports WAV/CSV/PNG
artifacts. Rack includes four-stage permutations, nonlinear-order/bypass checks,
FFT calibration and display slope tests. Existing full CTest gates remain.

Version 0.9.8 also includes a gui-dsp-review ZIP with the stereo CSV, the review
and 21 original WAV fixtures/examples. Native archives include GUI_DSP_REVIEW.md,
docs/gui-review and the supplied branding SVG. Validate the 48 distinct cases,
error thresholds and exact SVG bytes before publishing.

Appearance 0.9.8.1 regressions use processor_tests --appearance0981-check or
--appearance0981-report /absolute/folder. Preserve exact vector pixels across
UI/DPI scales when changing glyph rendering. TextRenderer must not reintroduce a
resampled full-interface image. Keep image decoding/file access off the audio
thread, retain theme v1/v2 import, bound decoded image dimensions, and use the
owning LogisthLook::contextMenu() for manual popups.
