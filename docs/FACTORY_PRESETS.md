# logisth 0.9.8.1 factory collection

48 authored patches, six per category; 58 built-in entries including Init and the nine previous sounds. Each new patch has an editable description and four performance macros: Brightness, Harmonics, Warp, and Space/Drive. Zero preserves the authored sound.

Open the magnifying-glass button beside the preset selector and search a name, category or factory description. The downloadable bank uses native `.logisth` files in category folders; choose its Presets folder with PATCH → Choose library folder to browse these copies as user sounds. They also remain available in the built-in selector without importing files.

## Audition and calibration

The bank includes a twelve-second, 48 kHz stereo demo: the first sound from each category, 1.5 seconds each. The CSV records all 48 patches twice: default macros and all four macros at maximum. The fixture uses full velocity, a three-second render with note-off halfway, MIDI 36 for bass and MIDI 48/55/60/64 for the other categories. Peak readings include release samples and are measured before WAV conversion. Main trims target at least 3 dB of fixture headroom; quieter textures retain their dynamics. This fixture does not cover every note, longer sustained pads, arbitrary macro combinations or user edits. There is no hidden limiter. Version 0.9.6.1 remeasures and trims Main after the stereo/reverb correction; both default and maximum-macro profiles must retain at least 3 dB in this fixture.

Every patch is saved and restored in a separate processor during testing, checking all 1084 controls, its name, description and macro names. Factory material is embedded; no external sample path is required. Init / saw still opens at Main 65%.

| Sound | Description |
| --- | --- |
| Bass / Deep Anchor | Centred sine/triangle sub. Clean low register; try C1-C3. |
| Bass / Rubber Line | Narrow pulse with a short resonant filter contour. |
| Bass / Concrete Drift | Detuned saw layers with controlled stereo and a dry low end. |
| Bass / Elastic Fold | Sine FM and gentle saturation; Harmonics increases movement. |
| Bass / Digital Current | Slow wavetable motion over a firm low-register envelope. |
| Bass / Basalt Hit | Pitch-decaying sine hit with a short saturated tail; use C1-C3. |
| Lead / Neon Summit | Seven-voice saw with restrained delay and a focused filter. |
| Lead / Pulse Lantern | Pulse-width lead with bright octave support. |
| Lead / Hollow Voltage | FM triangle lead with a hollow, singing centre. |
| Lead / Prism Edge | Bright wavetable lead with slow spectral movement. |
| Lead / Octave Signal | Open saw/octave stack for melodic hooks. |
| Lead / Night Slide | Mono last-note glide with a soft attack and short space. |
| Pluck / Copper Drop | Short triangle pluck, clean transient and dotted echoes. |
| Pluck / Glass Pin | Sine FM mallet with a bright attack and a short body. |
| Pluck / Muted Wire | Dark pulse pluck with a quick low-pass contour. |
| Pluck / Vector Chime | Wavetable chime with a clear upper-register decay. |
| Pluck / Metal Dew | Metallic FM pluck with a soft low-frequency foundation. |
| Pluck / Soft Spark | Compact digital pluck with a rounded, quiet tail. |
| Pad / Midnight Bloom | Slow detuned saw pad with wide chorus and room ambience. |
| Pad / Blue Horizon | Soft wavetable layer with gentle movement and a long release. |
| Pad / Slow Orbit | Triangle/saw pad with a restrained moving filter. |
| Pad / Neon Air | A quiet noise edge surrounds a smooth pitched foundation. |
| Pad / Violet Chamber | Warm stacked pad with vintage space and slow stereo motion. |
| Pad / Cloud Aurora | Granular glass and a soft sine foundation bloom into a shimmer tail. |
| Keys / Warm Felt | Soft triangle keys with a rounded filter and short room tail. |
| Keys / Crystal Circuit | FM electric keys with a clean attack and delicate echoes. |
| Keys / Hollow Bell | Longer metallic bell, played best in the middle register. |
| Keys / Quiet Organ | Sine/octave keys with an even sustain and small room. |
| Keys / Tape Ribbon | Soft pulse/string keys with gentle chorus motion. |
| Keys / Signal Tines | Signalsmith glass keys with a lightly shifted formant envelope. |
| Arp / Night Circuit | Tight saw sequence with alternating octave steps. |
| Arp / Clockwork Pulse | Pulse pattern with a short filter envelope and stereo echoes. |
| Arp / Glass Orbit | Wavetable steps with a bright upper-register accent. |
| Arp / Running Copper | Rounded triangle sequence with varied step velocity. |
| Arp / Step Lantern | A rising interval pattern over a compact saw voice. |
| Arp / Chord Beacon | Chord memory feeds a multi-lane pattern; start with one held note. |
| Wavetable / Prism Sweep | Slow movement through the harmonic table; a full sustained tone. |
| Wavetable / Digital Bite | Focused table position, narrow width and restrained drive. |
| Wavetable / Phase Ribbon | Gentle FM brings a moving, rounded digital texture. |
| Wavetable / Vector Mallet | A short table-based mallet with clear delayed reflections. |
| Wavetable / Low Motion | Lower-register wavetable voice with a controlled filter. |
| Wavetable / Crystal Stack | Wide digital stack with a long, transparent release. |
| Sample / Stretch Glow | Slower Signalsmith glass with a soft sustained envelope. |
| Sample / Formant Glass | Signalsmith formant shift over the embedded glass sample. |
| Sample / Half Time Bell | Protected attack followed by a slower spectral bell tail. |
| Sample / Grain Dust | Small overlapping grains with modest random motion. |
| Sample / Reverse Bloom | Reversed glass grows into a soft, stretched tail. |
| Sample / Cloud Loom | A moving glass cloud over a soft triangle foundation and spacious release. |

Version 0.9.8.1 reauditions all 96 profiles with the current engine. Native files
include saved default effect/distortion order and all 1084 controls. Additional
distortion slots, clipper and limiter remain off in this bank. Older files retain
Legacy compressor behavior when recalled.
