# Compact layout and background review — v0.9.8.2

MODULATION +/- in the header hides or shows the entire Mod Env 1/2, LFO 1/2 and
Misc 1/2 row. At 100%, the host editor changes from 1240 x 860 to 1240 x 640;
the keyboard, performance controls and footer move up 220 pixels. Audio parameters,
attachments and DSP are unchanged. Layout is an application appearance preference,
independent of presets, restored with exact typed scale and actual dimensions.

Automated checks use an actual mouse down/refresh/up sequence and verify first-click
collapse, visible widget bounds, complete parameter reachability, active hidden
host automation, 100 combinations of palette/scale/layout, uniform transforms,
preference reload and editor reconstruction. Both layouts cover 80%, 100%, 125%,
133.33% and 200% in ten palettes. Twenty distinct embedded PNGs are decoded,
selected through the real menu, encoded and decoded through the theme codec.

All background originals were generated for this release with OpenAI image
generation. Their palettes follow the ten standard themes. Each original is kept
as a separate unmodified PNG in assets/backgrounds, with catalogue.json mapping
artwork to its intended theme. The AP logo and owner SVG are unchanged.
SETTINGS > SKIN offers them offline; COLOURS > SAVE AS embeds the selected original
compressed bytes in a portable theme. Artwork is read/decoded only on the message
thread when selected, never on the audio thread. The original top highlight and
grid-at-y-zero line are removed.

Tests validate local rendering and native host-independent resize behavior.
Host-specific restrictions on programmatic editor resizing depend on the DAW.
Native CI runs the complete regression suite on Windows, Linux and macOS, with
an additional Windows AddressSanitizer build. The inherited 0.9.8.1 native glyph
probe remains historical evidence for the unchanged glyph renderer; current
native suites repeat its offscreen matrix and compact layout checks.

[Compact interface](images/logisth-compact.png) · [Factory background in AP](images/logisth-background-library.png)
