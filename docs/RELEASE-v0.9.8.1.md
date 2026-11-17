# logisth v0.9.8.1

Font rendering now operates directly at display resolution instead of resizing a
full-console bitmap. Native, Grayscale, 2x (default), 4x and Off are available,
with edge weight 50–200%. Fractional resizing uses one uniform scale. Existing
font size/style controls and global preferences remain available.

FILES adds CHOOSE/OPEN for Library, Preset Load/Save, Samples and Impulses. SKIN
imports portable PNG/JPEG backgrounds, supports Image + grid, image mix and panel
opacity, and makes grid backgrounds visible inside panels. Theme format 3 embeds
the compressed image; versions 1/2 remain readable. PNG is recommended for grids
and transparency, JPEG for photos (up to 4096 x 4096 / 16 MiB).

Distortion reserves a heading row above its four stages; inline-header views no
longer draw duplicate text behind controls. Patch and effect reorder context menus
follow the selected theme. All 1084 sound parameters and the audio DSP are retained.

English user/technical manuals, screenshots and appearance regression evidence
are updated. Windows/Linux publish first, then macOS Universal is appended to the
same release. Formats: VST3, CLAP, Standalone. Original-code MIT and upstream
notices remain included.
