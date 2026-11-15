# wavelogik logisth 0.9.4.1 — themes and interface fixes

Four supplied themes join the six existing palettes: bokkenmmb-Solarized,
bokkenmmb-Tokyo Night, bokkenmmb-Solarized Osaka and bokkenmmb-gruvbox.
Their JSON, twelve palette roles and knob/background styles are retained.
Choose them in **SETTINGS → APPEARANCE → COLOUR THEME**. Missing default files are
seeded into the theme folder; existing user edits and the selected theme remain.

IoskeleyMonoTerm Nerd Font Mono Regular and Bold from Ioskeley Mono v2.2.0 are
embedded unchanged. The four new themes use this font without requiring a system
installation. STYLE lists it alongside embedded Open Sans. Theme/source/native
packages include the fonts, OFL notices and pinned provenance/hashes.

The interface review fixes existing text retaining its old font after a theme edit,
search text retaining its old colour, Escape handling from the search result list,
and the search overlay staying open over Settings/PATCH. Font changes preserve
editing selection and preset metadata. Regression checks cover these interactions,
all ten palettes, appearance persistence, scaling, and real embedded typefaces.

SETTINGS now uses the same theme-aware toggle drawing, bold embedded font,
height, width and selected-state treatment as the neighbouring SYNC button.
Opening Settings preserves the audio Sync value. Other text buttons now explicitly
resolve the selected theme font, including tabs, PATCH, IMPORT and file actions.

The full release number is 0.9.4.1. Release verification, staged publication and CI
receipts now accept three- or four-part tags. Native metadata is checked; macOS uses
valid three-part bundle versions while plug-in/About strings retain 0.9.4.1.

The audio engine, 736 parameters and 58 factory entries are unchanged. The 48-patch
bank is retained; its level measurements are regenerated for this build. English documentation and
screenshots are refreshed. Required suites: Linux 12, Windows/macOS 10, Windows
AddressSanitizer 10; Windows/Linux publish first, then macOS Universal joins the same release.
