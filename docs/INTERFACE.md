# logisth console

The console occupies one 1240 × 860 canvas, or 1240 × 640 with MODULATION folded, with no vertical scrolling.
Its initial size adapts to the monitor. Drag the lower-right corner to resize
proportionally from 80% to 200%.

SETTINGS opens in the central rack and offers Light, Midnight Blue, Cobalt,
Graphite, Forest, AP always flexin'! and four bokkenmmb themes, plus About. Menus, editors, keyboard and controls follow the
palette. The selected appearance persists as a shared user preference; changing
sounds keeps it and audio settings remain untouched. Midnight Blue is the default.
Vector drawing and embedded Open Sans keep the design consistent across systems.

- Top: factory/user presets and PATCH file menu, part A/B/C, Solo, Poly, Sync and Settings; below, two oscillators
  and the selected part's amplitude envelope.
- Left: the selected part's filter and shared Filter Control A+B+C.
- Centre: separate Chords/Arp generators, eight movable audio effects, fixed output Limiter and Sample/Morph tools.
- Right: A/B/C/L+R oscilloscope, Mix A/B/C, Main and stereo meters.
- Bottom: two Mod Env, two LFO, four Misc sources, wheels, keyboard and glide.

Each modulation block has two destinations and amounts. Destination menus use
functional submenus with symmetric A/B/C and paired scopes. Hover for tooltips and the bottom value readout.
Numeric values support units such as kHz, ms and percent. Sync displays musical
divisions instead of free time/rate controls. Arp steps use 01–08 and 09–16 views.

The current engine exposes 1089 audio parameters. Tests check access, bounds, click
targets, view changes, bindings, scaling and independent theme persistence.
See [the English user manual](MANUAL.md) for full operation.

The oscillator header selects engine and only its relevant pages: VOICE/ENV for
Classic, WAVETABLE for the table engine, SOURCE/PLAYBACK/STRETCH/LOOP TAIL for Sampler, and GRAINS for
Granular. MOTION/TIME are available only in the Granulizer model. File drops and
IMPORT load audio into that slot. CHORDS is in the separate GENERATORS row. Pattern arp
uses a seven-row drawing canvas with independent lane lengths, exact cell entry,
playheads and two 16-step views; Legacy retains the original arp page. MIDI OUT
is in the top header. All ten themes use the same layout.

SAMPLE opens first in new editors and remains in the tools row. For Sampler and Granular it shows the actual PCM
waveform, live cursor and grain markers. For Wavetable it shows a perspective
stack, highlights the selected cycle and displays its frame number. WT POS,
dragging the view or wheeling selects a frame. The header's AUTO/length menu
controls frame slicing on import. Graphs arp has been removed; Legacy and the
seven-row Pattern grid remain. The mixer scope is always visible.

Version 0.5.1 increases embedded font sizes by 18%, with an 11 px base minimum
and 28 px popup rows. The console uses one shared layout-state comparison for
refresh and layout; unchanged audio/UI state does not hide and rebuild controls.
Regression tests exercise 100 toggle interaction cases across timer ticks in both parts
and both themes, checking both directions of each switch. The cached layout
previously omitted granular model state used by the refresh comparison, causing
a permanent mismatch with default patches.

Version 0.6 adds file dropping onto the SAMPLE waveform and header, with a
highlighted selected-slot destination. File decoding and peak preparation are
performed on the import worker. The mixer includes a dBFS readout and red
overload indication. The fixed 12-second sample limit is removed; active
imported PCM uses a 512 MiB aggregate memory budget.

The comment box below the preset selector edits up to 4096 characters of preset
metadata and is saved with both .logisth presets and DAW state. New instances
select Init / saw with Main 65%; restoring an existing sound keeps its saved level.
WT Pos / Start targets include individual A1/A2/B1/B2/C1/C2 plus A, B, C, A+B and A+B+C groups.
The ADSR curve uses actual attack/decay/release durations and a 0.5-second held
segment; zero-length stages are vertical rather than forced to diagonal lines.

0.9.0 update: Settings includes editable percentage scaling and remembers
the last editor dimensions. Appearance, Theme Editor and About use tabs in the
central rack. Themes are editable JSON files, including the five factory themes;
the folder is scanned automatically. Reverb shows only the controls for its
selected Hall/Room/Spring/Vintage/Shimmer algorithm. Manufacturer: wavelogik.

0.9.1: PHASE is enabled in Classic/Wavetable regardless of RETRIG. Its live cyclic
offset reaches held notes and automation; RETRIG selects fixed/random new-note
starts. The GUI refresh timer preserves this availability.

0.9.2 adds MOD to every source (FM/AM plus pulse-only PWM), Sampler PLAYBACK
stretch/interpolation menus, corrected Legacy and 10-band Graphic EQ with a
response editor. SETTINGS groups DISPLAY, TEXT, COLOURS, SKIN, MIDI, FILES and ABOUT; theme editing supports chosen
save paths and custom renaming. Unsaved theme drafts survive editor recreation;
folder scans run every two minutes. All MOD ENV defaults are zero. Reverb adds
Diffusion and a rebuilt Hall with denser networks and four-window Shimmer.

0.9.3 adds MORPH as the eleventh rack page: four names/knobs, four editable
routes per macro and mutation/lock/Undo controls inside the existing console.
Each source adds ROUTING for FM/AM source selection and PWM sync. MOD exposes
Width/Warp for every engine; PLAYBACK adds Tonal/Transient/Kick/Spectral, showing
Kick Attack only when applicable. SAMPLE supports zoomed drag selections and
START/END handles, with separate granular trim/scanning. Six standard themes
include AP always flexin'! and its white AP mark without the signature.

0.9.4 adds a magnifying-glass preset search beside the selector. The same-window
panel supports multiple query words, factory descriptions, recursive user folders,
arrow keys, Enter and Escape. The library has 58 entries, including 48 new patches
with descriptions and four performance macros. Each player has a compact STRETCH
page: six controls for Signalsmith Stretch/Kick, four for windowed time-domain
algorithms and Rate/Semitones for coupled playback modes.

0.9.4.1 adds four supplied bokkenmmb themes and embedded Ioskeley Mono Regular/Bold,
bringing the palette menu to ten entries. Existing editor text follows live font
changes. Search also follows palette edits and closes on Settings/PATCH or Escape
from its result list.

0.9.5 adds Part C in the header: C1/C2, Amp Env C, Filter C, Mix C, Solo C and
scope C. All players have the same four engines and source controls. LOOP TAIL
adds independent IN/OUT/crossfade and Alt-drag handles below the sample waveform.
DETUNE reaches 200 ct. Original parameter definitions are retained with appended
additive detune and C-choice companions, combined into single editor controls.

0.9.6 adds A+C/B+C targets and reorganizes routing by function. VOICE has adjacent
DETUNE (400 ct, midpoint 100) and BLEND for each player. Settings/Appearance adds
an optional fixed-maximum velocity switch. About scrolls through project credits.
Graphic EQ shows separate axes and a selected-band inspector with type/frequency/
Q/gain/enable, a 16-slot selector and Add/Remove buttons. Eight filter types are
available; original ten-band presets retain their Bell settings.

0.9.6.1 adds small Copy/Paste buttons at the bottom-left of all six oscillator
panels. MORPH target menus group every part by function. REVERB / CONVOLUTION
shows Load IR, Reset IR, IR metadata, Normalise, Mix, Predelay, Width and Wet Gain;
model-specific algorithmic controls hide. The existing single rack window is used.

0.9.6.2 places Chords first and retains signal order in the thirteen-row rack.
Compressor, Clipper and Limiter have shared-size graph panels and compact control
rows. IR gains a cached stereo waveform and drag/drop inside Convolution.
The limiter is after Main; Sample and Morph remain editing tools.

0.9.7 separates Chords/Arp into GENERATORS, followed by AUDIO FX with eight
20-pixel draggable effect rows, a fixed final Limiter and a Sample/Morph tools
row. Drag the grip/label, use Alt+Up/Down or the right-click menu; the DSP order
and visible order agree and are saved with presets. The active page stays open
through a drag. Sample is the initial page in each new editor.
POLY combines the preserved 1–16 host parameter with an appended 0–16 companion
and exposes one 1–32 control, default 16. Light screen knobs now use dark faces;
cached contrast repair keeps enabled needles/arcs/tracks visible in all ten
themes and three styles without rewriting supplied palette files.

The current working tree adds a four-slot serial distortion editor. Slots use
identity numbers 1–4; they are selected independently from their processing
position. Dragging a slot preserves the active editing selection. Chorus uses a
stable 4×2 grid with RATE/DUAL at the right and FEEDBACK after DRY/WET.
EQ separates the analyzer row from its graph and band inspector. Application
font preferences affect existing labels and editable text without changing
preset audio data. The standard header contains only the smaller SVG mark;
manufacturer/product/description appear in the lower-right footer.

Version 0.9.8.1 renders glyphs directly at display resolution, with Native,
Grayscale, 2x/4x and Off plus edge weight 50–200%. Window scaling is uniform.
FILES provides CHOOSE/OPEN for five browser directories. SKIN imports portable
PNG/JPEG images, image/grid combinations and image/panel opacity controls.
Distortion reserves its heading row; inline-header pages omit redundant titles.
All Patch/reorder context menus bind the instrument theme explicitly.

The header MODULATION +/- button folds the whole lower modulation row and
moves the keyboard up by 220 logical pixels. The host window really resizes,
using one uniform scale. Hiding controls does not stop modulation. The last
layout is restored when opening another editor. SKIN offers 20 offline factory
backgrounds grouped by their matching palette. No top-edge highlight is drawn.

Version 0.9.8.3 reserves a separate toolbar for Distortion title, routing, band
count, analyzer and tilt. Multiband mode shows 1–4 mapped headers and On/Off
buttons, a log-frequency graph, selected module controls and only N−1 crossover
fields. Legacy retains the serial layout. Selecting bands changes editing only;
the power toggles remain separate. Drag/wheel/reset edits send host gestures.
SKIN adds gradient/grid amount sliders, with immediate repaint and no control
visibility changes while dragging. The About list adds mixedbytrzy.
