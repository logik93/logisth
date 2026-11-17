# logisth v0.9.8

Chorus keeps a stable 4×2 grid: RATE/DUAL on the right, FEEDBACK after DRY/WET.
Distortion now contains four independently enabled serial stages. Select a
header to edit its model/Drive/Mix; drag, use Alt+Left/Right or the context menu
to change processing order. New instances and Init start at 1/2/3/4. Presets and
projects retain the chosen order. All previous 1071 host definitions are preserved;
thirteen appended controls bring the total to 1084. Bypassed slots skip DSP.

EQ gains stereo-power Pre/Post/Both spectra and Off, with display-only tilt
0/1.5/3/4.5/6 dB/oct. The 2048-point Hann FFT runs on the GUI thread and preserves
antiphase visibility. Hidden/off views stop capture/FFT work. The stereo convolution
review passed 48 actual-WAV cases plus two complete sampler integrations; no
lane-swap or unwanted mono fold appeared, so convolution processing was retained.

Settings now groups Display, Text, Colours, Skin, MIDI, Files and About. Font size,
weight/italic and glyph antialiasing persist across instances. AA Off affects
text coverage while vector graphics stay antialiased. Separate preset open/save,
sample, IR and library directories are remembered after successful operations.
Windows instructions recommend the system VST3/CLAP folders under Common Files,
with user alternatives documented for each platform.

The supplied exact SVG replaces the standard header mark at a smaller size;
manufacturer/product/description move to the lower-right footer. AP artwork
remains unchanged. Both English manuals, native VST3/CLAP/Standalone packages,
updated screenshots and real-WAV/CSV evidence are included. Original-code MIT
and all upstream notices remain in the packages.

Windows/Linux publish first; macOS Universal is appended to the same release.
Native gates require 20 Linux / 18 Windows and macOS suites plus Windows ASAN.
