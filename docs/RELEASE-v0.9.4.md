# wavelogik logisth 0.9.4 — Stretch controls, preset search and 48 sounds

The former Spectral sampler mode is named **Signalsmith Stretch**. Its processor
is updated to the unmodified upstream 1.3.2 release, with pinned Signalsmith
Linear 0.6.6 (both MIT). Native formant shifting is now available.

Each of A1/A2/B1/B2 has a compact **Sampler → STRETCH** page:
- Rate: 0.125–8× speed multiplier in addition to the existing Time Factor.
- Semitones: independent -48 to +48 st offset (in independent stretch modes).
- Tonality Limit: 0/off to 20 kHz, normalized to host sample rate for upstream.
- Formant: -24 to +24 st spectral-envelope shift; zero is neutral.
- Block: Auto or a requested window in milliseconds (up to 200 ms).
- Overlap: Auto or 2–16×. Auto retains the algorithm's normal window/overlap.

Rate/Semitones also work with other sampler algorithms. Windowed time-domain
modes expose Block/Overlap; Tonality/Formant appear for Signalsmith and Kick.
Off/Tape retain coupled rate/pitch behaviour. Kick keeps its protected attack.
Time-domain overlap gain is normalized, and sinc bandwidth follows actual rate.

Window changes prepare fresh pools outside the audio callback. Generation checks,
stream rebinding and pool retirement cover live/muted players and rapid edits.
Only requested sources/unison/polyphony are prepared, with a conservative 512 MiB
bank estimate; excessive requests show an actionable preparation status.

The new conformance suite compares stereo adapter output directly with upstream
at 44.1/48/96 kHz. Vendor files were also compared byte for byte against pinned
checkouts; hashes and the scope of proprietary-algorithm comparisons are recorded
in ALGORITHM_AUDIT.json and PROJECT_REVIEW.md. Independent algorithms remain
original logisth implementations, not sample-exact clones of proprietary DSP.

A magnifying-glass search panel filters factory names/descriptions and user
names/folders, with keyboard selection and Enter/Escape. The factory library
adds 48 authored patches in eight categories (58 entries including Init and the
previous sounds), editable descriptions and four neutral-at-zero macros per new
patch. The downloadable bank includes native preset files, a demo and level
measurements for default and maximum macro auditions.

Authored repository files and the manual describe logisth and the open-source
projects actually used. Historical generated packages are kept through their
published releases/version tags rather than duplicated in the current tree.

736 automatable parameters. New controls round-trip with presets/projects.
Native formats remain VST3, CLAP and Standalone. Required suites: Linux 12,
Windows/macOS 10, plus Windows AddressSanitizer. Windows and Linux publish first;
macOS Universal is built afterward and appended to the same release.
