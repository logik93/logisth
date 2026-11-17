# logisth v0.9.8.1 — appearance regression review

The optional v0.9.8 text path rasterised the full console and composited it with
low-quality resampling. Fractional UI/display transforms could produce uneven
edges; transparent partial repaints could retain older text. The new path wraps
normal painting and intercepts glyphs only. Vector geometry/pixels are preserved.
A uniform editor transform also prevents separate X/Y window rounding from
stretching glyphs.

TEXT offers Native, Grayscale, 2x (default), 4x and AA Off. Supersampling applies
only to glyph alpha masks; average coverage is reduced to physical pixels. Edge
weight 50–200% applies coverage^(1/weight); 100% is neutral. Native uses platform
hinting at neutral weight, otherwise grayscale masks. Synthetic italic and native
fallback for glyphs without outlines are retained. Host/OS native dialog fonts
remain controlled by the system. The controls are application preferences, not
sound parameters.

The rendering matrix has 96 cases: six UI scales (80/100/110/125/133.3/200%), two
display scales (1x/2x), four qualities and AA on/off. It measures actual grayscale
coverage and compares neighboring vector pixels exactly with the unwrapped
renderer. Off must have two glyph gray levels; smoothing must have intermediate
coverage. Additional cases check monotonic edge weight and transparent text
replacement without stale letters. Real editor snapshots exercise fractional
window sizes and uniform transforms. These are software-rendering regressions,
not a claim of identical native font hinting on every operating system.

Five FILES rows expose CHOOSE/OPEN: Library, Preset Load, Preset Save, Samples and
Impulses. Tests select real directories through the same handler used by chooser
callbacks, verify labels/persistence/library scanning, and reject missing paths.
Native OS file-dialog interaction itself is not automated by these fixtures.

The skin fixtures import actual PNG/JPEG files, measure image mix and grid pixels
inside panels, retain the earlier draft on invalid imports, reject excessive
header dimensions before decompression, save an embedded image and reopen it
after deleting the original files. Theme format 3 keeps versions 1/2 readable.
Inputs are limited to 16 MiB compressed / 4096 x 4096. PNG is recommended for grids,
illustrations and transparency; JPEG for photos. Image mix and panel opacity are
saved with the theme, independently of sound presets.

Patch, outer FX reorder and distortion-stage context menus use a common factory
bound to the owning look-and-feel. Every standard theme is checked for actual
popup background pixels and text/background palette bindings. Distortion reserves
its title row. Inline-header pages omit redundant parent headings behind controls;
parameter reachability/layout regression coverage remains active.

`processor_tests --appearance0981-report /absolute/folder` exports the matrix and
illustrative PNG captures. No audio algorithms, 1084 sound parameters, standard
palette assets, AP artwork or the supplied standard SVG were changed.

Report writers replace existing PNGs. Repeating the export is checked; release
validation rejects trailing/concatenated PNG data after IEND.

Physical glyph masks are coloured in software and drawn as small RGBA images.
Native alpha-clip bitmap brushes can re-filter masks and affect later primitive
rendering; the glyph path avoids those native clip layers. Vector operations
continue directly through the original graphics context.
