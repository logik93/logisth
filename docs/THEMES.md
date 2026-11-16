# logisth themes and window preferences

SETTINGS / APPEARANCE / OPEN FOLDER reveals the current theme directory (initially
`logisth/themes` under your system user application-data directory). USE FOLDER
selects another directory and remembers it. Ten standard palettes are written
there on first use; source/native packages include them in `assets/themes`.
Missing standard files are recreated; existing files are retained.

The plug-in polls .json files every **two minutes**. RESCAN reads them immediately.
Files larger than 64 KiB, malformed JSON, unsupported formats/versions and invalid
colours are ignored. Subfolders are not scanned. File names identify selections;
adding a theme does not switch the current selection. Removing the selected file
falls back to Midnight Blue.

The sixth standard skin, AP always flexin'!, uses the optional `"skin": "ap-flexin"`
property to show its embedded white AP mark (without a handwritten signature).
The skin travels with the plugin and package; it needs no external font file.

Version 2 adds font, background, knob styles and two dedicated knob colours:

```json
{
  "format": "logisth-theme",
  "version": 2,
  "name": "Ocean",
  "dark": true,
  "font": "Open Sans",
  "backgroundStyle": 1,
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
are 0 Flat, 1 Gradient, 2 Grid; knob styles are 0 Arc, 1 Solid, 2 Minimal. The
font is a family name (up to 128 characters). Open Sans and IoskeleyMonoTerm Nerd
Font Mono are embedded in logisth. Other fonts must be installed on the destination
machine and use system fallback if unavailable. JSON theme files do not contain font data.

Version 0.9.4.1 adds the supplied bokkenmmb-Solarized, bokkenmmb-Tokyo Night,
bokkenmmb-Solarized Osaka and bokkenmmb-gruvbox themes. Their original JSON files,
all twelve colours and knob/background styles are retained. All four select
IoskeleyMonoTerm Nerd Font Mono from Ioskeley Mono v2.2.0. Both Regular and Bold
are bundled unchanged, so these themes work without installing fonts separately.
The font and licenses are also included in the theme download under fonts/ioskeley.
STYLE lists both embedded families before the installed system fonts.

Changing font or palette updates existing text in comments, macro names and search
fields. Editing selections and preset metadata are preserved.

Version-1 files remain readable; missing extra knob colours default to accent
and border, font to Open Sans, and styles to Flat/Arc. All twelve palette roles
are emitted when saving version 2. Choose enough text/background contrast for
your display.

THEME EDITOR previews a draft: choose a role, use the colour picker or type a
hex value and press Enter. STYLE chooses font, background and knob drawing.
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

APPEARANCE accepts 80–200% UI scaling, including decimals. The displayed value is
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
