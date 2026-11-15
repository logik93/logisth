# logisth 0.9.4.1 review and corrections

The review covered theme loading/saving and appearance drafts, font resolution and
text updates, preset search and focus, parameter/menu interaction, scaling/layout,
release metadata, and the existing audio/state regression suites.

## Corrected

- TextEditor::setFont changes the font for newly typed characters. Theme updates
  now use applyFontToAllText for existing text, and avoid redundant restyling when
  the font is unchanged. Selection and preset comments must survive the operation.
- Existing search text and placeholder colours now follow a changed palette.
- Escape is accepted from the result list as well as the query. Settings/PATCH
  dismiss the search overlay before showing their controls.
- Supplied palettes compile directly from their exact JSON resources, eliminating
  a second colour table. Existing files are retained when defaults are seeded.
- Embedded Ioskeley Regular/Bold resolve through explicit typefaces, avoiding
  reliance on installed fonts. STYLE exposes the bundled family on all systems.
- Release tags, receipts and publication support the requested fourth version part.
  Windows/plug-in strings retain it; Apple bundle metadata uses valid three-part
  fields and includes the full release in its information string.
- Readability assertions compare the actual palette text colour, rather than
  assuming every dark theme uses near-white text.

SETTINGS now uses the same theme-aware toggle drawing, bold embedded font,
height, width and selected-state treatment as the neighbouring SYNC button.
Opening Settings preserves the audio Sync value. Other text buttons now explicitly
resolve the selected theme font, including tabs, PATCH, IMPORT and file actions.

## Validation and remaining priorities

Regression coverage checks ten themes, native font identities/styles, existing-text
geometry and selection, search repaint/dismissal, JSON/font hashes, portable theme
files and staged publication for three/four-part tags. Existing suites cover MIDI,
state, filters, EQ/reverbs, sample selection, stretch controls, button bindings and
real Linux CLAP/VST3 binaries. CI checks both macOS binary slices and runs on Intel.
Local validation passed all 12 CTest suites and nine publisher/version tests.
All 96 factory default/maximum-macro auditions retain at least 3 dB of reference
headroom. The native release receipt records platform outcomes after builds finish.

The audio/CPU priorities in PROJECT_REVIEW.md remain useful: measured optional
oversampling, lazy sample analysis with a whole-instance memory meter, and broader
real-DAW coverage. This maintenance release does not claim new DSP or host tests
that were not performed. It preserves the calibrated sound engine and preset bank.
