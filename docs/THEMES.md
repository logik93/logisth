# logisth themes and window preferences

SETTINGS / DISPLAY / OPEN FOLDER reveals the current theme directory (initially
`logisth/themes` under your system user application-data directory). USE FOLDER
selects another directory and remembers it. Ten standard palettes are written
there on first use; source/native packages include them in `assets/themes`.
Missing standard files are recreated; existing files are retained.

The plug-in polls .json files every **two minutes**. RESCAN reads them immediately.
Malformed JSON, unsupported formats/versions and invalid colours are ignored.
The JSON size bound is the maximum 16 MiB image encoded as base64 plus 64 KiB
for metadata. Subfolders are not scanned. File names identify selections;
adding a theme does not switch the current selection. Removing the selected file
falls back to Midnight Blue.

The sixth standard skin, AP always flexin'!, uses the optional `"skin": "ap-flexin"`
property to show its embedded white AP mark (without a handwritten signature).
The skin travels with the plugin and package; it needs no external font file.

Version 3 adds portable image backgrounds and opacity controls; versions 1/2
remain readable. A minimal version-3 theme is:

```json
{
  "format": "logisth-theme",
  "version": 3,
  "name": "Ocean",
  "dark": true,
  "font": "Open Sans",
  "backgroundStyle": 1,
  "backgroundImage": "",
  "backgroundMix": 0.5,
  "panelOpacity": 0.85,
  "knobStyle": 0,
  "colours": {
    "body": "#0D1118",
    "panel": "#151D29",
    "border": "#283A50",
    "text": "#E0EDFF",
    "muted": "#9AADC5",
    "accent": "#18C4D2",
    "highlight": "#91C8FF",
    "screen": "#080D15",
    "screenBorder": "#243952",
    "screenText": "#E0EDFF",
    "knob": "#18C4D2",
    "knobTrack": "#283A50"
  }
}
```

RGB values are six hexadecimal digits, optionally prefixed by #. Name is 1–64
characters; dark is a boolean controlling light/dark controls. Background styles
are 0 Flat, 1 Gradient, 2 Grid, 3 Image, 4 Image + grid; knob styles are 0 Arc, 1 Solid, 2 Minimal. The
font is a family name (up to 128 characters). Open Sans and IoskeleyMonoTerm Nerd
Font Mono are embedded in logisth. Other fonts must be installed on the destination
machine and use system fallback if unavailable. JSON theme files do not contain font data.

Version 0.9.4.1 adds the supplied bokkenmmb-Solarized, bokkenmmb-Tokyo Night,
bokkenmmb-Solarized Osaka and bokkenmmb-gruvbox themes. Their original JSON files,
all twelve colours and knob/background styles are retained. All four select
IoskeleyMonoTerm Nerd Font Mono from Ioskeley Mono v2.2.0. Both Regular and Bold
are bundled unchanged, so these themes work without installing fonts separately.
The font and licenses are also included in the theme download under fonts/ioskeley.
TEXT lists both embedded families before the installed system fonts.

Changing font or palette updates existing text in comments, macro names and search
fields. Editing selections and preset metadata are preserved.

Version-1 files remain readable; missing extra knob colours default to accent
and border, font to Open Sans, and styles to Flat/Arc. All twelve palette roles
are emitted when saving version 3. Choose enough text/background contrast for
your display.

COLOURS previews a draft: choose a role, use the colour picker or type a
hex value and press Enter. TEXT chooses the theme font family; SKIN chooses background and knob drawing.
LOAD IMAGE... accepts PNG/JPEG up to 4096 x 4096 and 16 MiB. PNG is best for grids
and transparency; 1240 x 860 or 2480 x 1720 matches the canvas. Image data is embedded
as base64 in backgroundImage, preserving the original compressed file. No external
path is required. IMAGE MIX is 0–100%; PANELS is 40–100% for non-flat styles. These
map to backgroundMix [0,1] and panelOpacity [0.4,1]. Images crop centrally to fill
the canvas. CLEAR removes the image; saving makes the image and controls portable.
SAVE NEW writes a unique custom file in the current directory; duplicate names
receive a suffix. SAVE AS... chooses an exact folder/file name, and makes that
folder active. RENAME changes the selected custom theme's name/file and saves
the draft; standard files are protected. RESET restores the saved selection.

Unsaved colours and styles stay with the instrument instance when its editor
is destroyed/recreated or minimized. They are excluded from audio presets and
are not retained after destroying the instrument/closing the host: save a theme
for that. Folder rescans preserve a draft if its saved baseline is unchanged.
Selecting another saved theme replaces the draft. Saved selection changes
propagate between open instances in the same host process. Audio presets do not
change theme, scale or editor size.

DISPLAY accepts 80–200% UI scaling, including decimals. The displayed value is
the last accepted percentage; manual input accepts a decimal dot or comma.
Invalid input restores the last accepted value. Resizing preserves the aspect
ratio. Actual pixel dimensions and scale are stored in `appearance.json` and
restored in new/reopened editors. Existing windows retain their own size.

## Bundled bokkenmmb gallery

[Solarized](images/logisth-bokkenmmb-solarized.png) ·
[Tokyo Night](images/logisth-bokkenmmb-tokyo-night.png) ·
[Solarized Osaka](images/logisth-bokkenmmb-solarized-osaka.png) ·
[Gruvbox](images/logisth-bokkenmmb-gruvbox.png)

Version 0.9.7 derives enabled knob/plot colors for their actual rendering surface.
Light screen knobs use a dark face, with stronger needles and contrast-aware
tracks/arcs. These cached drawing colors preserve the saved palette values.
The [rack/skin review](RACK_REVIEW.md) covers all ten themes and three knob styles.

## Text preferences and brand artwork

TEXT also provides 80–125% font size, Theme default/Regular/Bold/Italic/Bold italic,
and font antialiasing. These settings are stored globally in preferences.json;
the theme retains its font family. The glyph renderer handles editable text and
menus; disabling glyph AA leaves vector shapes antialiased. Italic can use a
synthetic slant where the bundled typeface has no separate italic face.
The heavier bundled face supplies Bold (Open Sans Semibold or Ioskeley Bold).
Font-size changes do not change the window scale or saved palette draft.

The standard header uses the exact project-owner SVG in assets/branding/logisth.svg,
tinted to the theme text color and reduced in size. The text description is in
the lower-right footer. AP header/About artwork uses the existing AP bitmap;
its source bytes are unchanged.

Text quality is independent of the theme: Native, Grayscale, 2x (default), 4x,
AA Off and edge weight 50–200% are saved in preferences.json. Only glyph masks
are supersampled; vector drawing is direct. 4x performs more GUI work than 2x.

## Factory background library (0.9.8.2)

SETTINGS > SKIN contains twenty embedded generated PNG originals: two per
factory palette, grouped by theme. They work offline and need no installation
folder. Selecting artwork changes the image only, sets IMAGE MIX to 100%, and
retains the palette, font, knob drawing, panel opacity and logos. Use PANELS to
control visibility under the interface, then COLOURS > SAVE AS to preserve it.
The selected compressed PNG travels inside the theme JSON. The release also
provides a backgrounds ZIP with the unchanged originals and catalogue.

The header MODULATION button switches the window between 1240 x 860 and
1240 x 640 at 100%. Resize aspect ratio follows the active layout. The saved
appearance preference retains this state and fractional scale independently
of sound presets; collapsing does not disable modulation.
