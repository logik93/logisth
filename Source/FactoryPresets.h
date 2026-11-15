#pragma once
#include <array>
#include <string>
namespace logisth {
struct FactorySound {
    const char *name, *description;
};
inline constexpr std::array<const char *, 10> legacyFactoryNames{
    "Init / saw",    "Wide supersaw", "Sub bass",    "Slow atmosphere", "Pulse pluck",
    "Step sequence", "Morph glass",   "Sample keys", "Grain cloud",     "Chord pulse"};
inline constexpr std::array<FactorySound, 48> designedSounds{{
    {"Bass / Deep Anchor", "Centred sine/triangle sub. Clean low register; try C1-C3."},
    {"Bass / Rubber Line", "Narrow pulse with a short resonant filter contour."},
    {"Bass / Concrete Drift", "Detuned saw layers with controlled stereo and a dry low end."},
    {"Bass / Elastic Fold", "Sine FM and gentle saturation; Harmonics increases movement."},
    {"Bass / Digital Current", "Slow wavetable motion over a firm low-register envelope."},
    {"Bass / Basalt Hit", "Pitch-decaying sine hit with a short saturated tail; use C1-C3."},
    {"Lead / Neon Summit", "Seven-voice saw with restrained delay and a focused filter."},
    {"Lead / Pulse Lantern", "Pulse-width lead with bright octave support."},
    {"Lead / Hollow Voltage", "FM triangle lead with a hollow, singing centre."},
    {"Lead / Prism Edge", "Bright wavetable lead with slow spectral movement."},
    {"Lead / Octave Signal", "Open saw/octave stack for melodic hooks."},
    {"Lead / Night Slide", "Mono last-note glide with a soft attack and short space."},
    {"Pluck / Copper Drop", "Short triangle pluck, clean transient and dotted echoes."},
    {"Pluck / Glass Pin", "Sine FM mallet with a bright attack and a short body."},
    {"Pluck / Muted Wire", "Dark pulse pluck with a quick low-pass contour."},
    {"Pluck / Vector Chime", "Wavetable chime with a clear upper-register decay."},
    {"Pluck / Metal Dew", "Metallic FM pluck with a soft low-frequency foundation."},
    {"Pluck / Soft Spark", "Compact digital pluck with a rounded, quiet tail."},
    {"Pad / Midnight Bloom", "Slow detuned saw pad with wide chorus and room ambience."},
    {"Pad / Blue Horizon", "Soft wavetable layer with gentle movement and a long release."},
    {"Pad / Slow Orbit", "Triangle/saw pad with a restrained moving filter."},
    {"Pad / Neon Air", "A quiet noise edge surrounds a smooth pitched foundation."},
    {"Pad / Violet Chamber", "Warm stacked pad with vintage space and slow stereo motion."},
    {"Pad / Cloud Aurora", "Granular glass and a soft sine foundation bloom into a shimmer tail."},
    {"Keys / Warm Felt", "Soft triangle keys with a rounded filter and short room tail."},
    {"Keys / Crystal Circuit", "FM electric keys with a clean attack and delicate echoes."},
    {"Keys / Hollow Bell", "Longer metallic bell, played best in the middle register."},
    {"Keys / Quiet Organ", "Sine/octave keys with an even sustain and small room."},
    {"Keys / Tape Ribbon", "Soft pulse/string keys with gentle chorus motion."},
    {"Keys / Signal Tines", "Signalsmith glass keys with a lightly shifted formant envelope."},
    {"Arp / Night Circuit", "Tight saw sequence with alternating octave steps."},
    {"Arp / Clockwork Pulse", "Pulse pattern with a short filter envelope and stereo echoes."},
    {"Arp / Glass Orbit", "Wavetable steps with a bright upper-register accent."},
    {"Arp / Running Copper", "Rounded triangle sequence with varied step velocity."},
    {"Arp / Step Lantern", "A rising interval pattern over a compact saw voice."},
    {"Arp / Chord Beacon", "Chord memory feeds a multi-lane pattern; start with one held note."},
    {"Wavetable / Prism Sweep", "Slow movement through the harmonic table; a full sustained tone."},
    {"Wavetable / Digital Bite", "Focused table position, narrow width and restrained drive."},
    {"Wavetable / Phase Ribbon", "Gentle FM brings a moving, rounded digital texture."},
    {"Wavetable / Vector Mallet", "A short table-based mallet with clear delayed reflections."},
    {"Wavetable / Low Motion", "Lower-register wavetable voice with a controlled filter."},
    {"Wavetable / Crystal Stack", "Wide digital stack with a long, transparent release."},
    {"Sample / Stretch Glow", "Slower Signalsmith glass with a soft sustained envelope."},
    {"Sample / Formant Glass", "Signalsmith formant shift over the embedded glass sample."},
    {"Sample / Half Time Bell", "Protected attack followed by a slower spectral bell tail."},
    {"Sample / Grain Dust", "Small overlapping grains with modest random motion."},
    {"Sample / Reverse Bloom", "Reversed glass grows into a soft, stretched tail."},
    {"Sample / Cloud Loom",
     "A moving glass cloud over a soft triangle foundation and spacious release."},
}};
inline constexpr int factoryPresetCount = int(legacyFactoryNames.size() + designedSounds.size());
// Output trims are measured against the factory audition fixture, not randomized.
inline constexpr std::array<float, 48> designedMainLevels{
    0.691831454f, 0.691831041f, 0.691830864f, 0.691831454f, 0.691836036f, 0.691831023f,
    0.246522846f, 0.212637693f, 0.341060752f, 0.344776006f, 0.272851817f, 0.722356782f,
    0.375343309f, 0.326705954f, 0.298319823f, 0.550854954f, 0.317109098f, 0.561613203f,
    0.266646118f, 0.352481916f, 0.282094390f, 0.236467738f, 0.295668759f, 1.000000000f,
    0.327127111f, 0.266590525f, 0.254160447f, 0.216481118f, 0.234092630f, 0.453162717f,
    1.000000000f, 1.000000000f, 1.000000000f, 1.000000000f, 1.000000000f, 1.000000000f,
    0.397948243f, 0.325181202f, 0.325918900f, 0.376017905f, 0.306643547f, 0.321686637f,
    0.575953281f, 0.470372397f, 0.570637088f, 0.974213627f, 1.000000000f, 0.732567420f};
template <class Set> void applyDesignedSound(int index, Set set) {
    int group = index / 6, variant = index % 6;
    auto osc = [&](int slot, const char *suffix, float value) {
        auto id = "o" + std::to_string(slot) + suffix;
        set(id.c_str(), value);
    };
    set("polyphony", 8);
    set("mixA", .8f);
    set("mixB", .55f);
    set("o1level", .78f);
    set("o1wave", 1);
    set("o1voices", 1);
    set("o2level", 0);
    set("o2voices", 1);
    set("o3level", 0);
    set("o4level", 0);
    set("p1type", 0);
    set("p1slope", 1);
    set("p1cutoff", 6000);
    set("p1res", .12f);
    set("p1ampa", .004f);
    set("p1ampd", .35f);
    set("p1amps", .7f);
    set("p1ampr", .18f);
    set("p2type", 0);
    set("p2cutoff", 5000);
    set("p2amps", .6f);
    set("env1dest1", 13);
    set("env1d", .2f);
    set("env1s", 0);
    set("env1amt1", 0);
    set("lfo1free", 1);
    set("lfo1rate", .18f);
    set("lfo1gain", 1);
    set("reverbtype", 1);
    set("reverbdecay", 1.4f);
    set("reverbdiffusion", .85f);
    set("chorus", .14f);
    set("delay", .15f);
    set("feedback", .25f);
    if (group == 0) {
        set("monolegato", 1);
        set("polyphony", 1);
        set("p1cutoff", 800 + variant * 170.f);
        set("p1amps", .8f);
        set("p1ampr", .08f);
        set("diston", 1);
        set("drive", .1f + variant * .025f);
        set("o1stereo", 0);
        set("o2stereo", 0);
        if (variant == 0) {
            set("o1wave", 0);
            set("o2wave", 3);
            set("o2octave", -1);
            set("o2level", .18f);
        }
        if (variant == 1) {
            set("o1wave", 2);
            set("o1pulsewidth", .32f);
            set("env1amt1", 2.2f);
            set("env1d", .12f);
            set("p1res", .32f);
        }
        if (variant == 2) {
            set("o1voices", 3);
            set("o1detune", 12);
            set("o1stereo", .35f);
            set("o2level", .22f);
            set("o2wave", 1);
            set("o2voices", 2);
            set("o2detune", 7);
        }
        if (variant == 3) {
            set("o1wave", 0);
            set("o1fmdepth", 2.2f);
            set("o1fmratio", .5f);
            set("p1cutoff", 2600);
        }
        if (variant == 4) {
            set("o1engine", 1);
            set("o1position", .72f);
            set("lfo1dest1", 34);
            set("lfo1amt1", .22f);
            set("lfo1rate", .11f);
        }
        if (variant == 5) {
            set("o1wave", 0);
            set("monolegato", 0);
            set("o1octave", -1);
            set("p1amps", 0);
            set("p1ampd", .28f);
            set("env1dest1", 4);
            set("env1amt1", 2.5f);
            set("env1d", .035f);
            set("drive", .3f);
        }
    } else if (group == 1) {
        set("o1voices", variant == 0 ? 7.f : 3.f);
        set("o1detune", 10 + variant * 2.f);
        set("o1stereo", .65f);
        set("o2level", .2f);
        set("o2wave", 0);
        set("reverbon", 1);
        set("reverb", .12f);
        set("delayon", 1);
        set("p1cutoff", 4200 + variant * 700.f);
        if (variant == 1) {
            set("o1wave", 2);
            set("o1pulsewidth", .38f);
            set("o2octave", 1);
        }
        if (variant == 2) {
            set("o1wave", 3);
            set("o1fmdepth", 1.3f);
            set("o1fmratio", 1.5f);
        }
        if (variant == 3) {
            set("o1engine", 1);
            set("o1position", .78f);
            set("lfo1dest1", 34);
            set("lfo1amt1", .1f);
        }
        if (variant == 4) {
            set("o2wave", 1);
            set("o2octave", 1);
            set("o2voices", 2);
        }
        if (variant == 5) {
            set("monolegato", 1);
            set("portamento", .09f);
            set("o1wave", 1);
            set("p1ampa", .012f);
        }
    } else if (group == 2) {
        const int waves[]{3, 0, 2, 1, 0, 3};
        set("o1wave", float(waves[variant]));
        set("p1amps", 0);
        set("p1ampd", .16f + variant * .045f);
        set("p1ampr", .12f);
        set("p1cutoff", 2200 + variant * 600.f);
        set("env1amt1", 2.5f);
        set("env1d", .1f + variant * .02f);
        set("delayon", 1);
        set("delay", .18f);
        set("delaydivL", 7);
        set("delaydivR", 9);
        if (variant == 1 || variant == 4) {
            set("o1fmdepth", variant == 1 ? .7f : 2.f);
            set("o1fmratio", variant == 1 ? 2.f : 3.5f);
        }
        if (variant == 2)
            set("o1pulsewidth", .23f);
        if (variant == 3 || variant == 5) {
            set("o1engine", 1);
            set("o1position", variant == 3 ? .5f : .23f);
        }
    } else if (group == 3) {
        set("polyphony", 6);
        set("o1voices", 4);
        set("o1detune", 8 + variant * 2.f);
        set("o1stereo", .85f);
        set("p1ampa", .55f + variant * .11f);
        set("p1ampd", 1.1f);
        set("p1amps", .72f);
        set("p1ampr", 1.6f + variant * .25f);
        set("p1cutoff", 1400 + variant * 350.f);
        set("choruson", 1);
        set("chorus", .22f);
        set("reverbon", 1);
        set("reverb", .25f);
        set("reverbdecay", 3.8f);
        set("reverbtype", variant == 4 ? 3.f : 1.f);
        set("lfo1dest1", 15);
        set("lfo1amt1", .35f);
        set("lfo1rate", .07f + variant * .016f);
        if (variant == 1) {
            set("o1engine", 1);
            set("o1position", .08f);
            set("lfo1dest1", 34);
            set("lfo1amt1", .27f);
        }
        if (variant == 2) {
            set("o1wave", 3);
            set("o2wave", 1);
            set("o2level", .17f);
        }
        if (variant == 3) {
            set("o2wave", 8);
            set("o2level", .025f);
            set("p1cutoff", 3600);
        }
        if (variant == 4) {
            set("o3engine", 1);
            set("o3position", .2f);
            set("o3level", .18f);
            set("o3voices", 2);
            set("p2ampa", 1);
            set("p2ampr", 2.6f);
        }
        if (variant == 5) {
            set("o1engine", 3);
            set("o1voices", 1);
            set("o1gmodel", 1);
            set("o1gattack", 35);
            set("o1ghold", 120);
            set("o1gspace", .65f);
            set("o1gwave", .4f);
            set("o1gpan", .45f);
            set("o1gdepth", .3f);
            set("reverbtype", 4);
            set("shimmeramount", .25f);
            set("o3wave", 0);
            set("o3level", .18f);
            set("o3voices", 2);
            set("p2ampa", .7f);
            set("p2ampr", 2.5f);
            set("p2cutoff", 2200);
        }
    } else if (group == 4) {
        set("o1wave", variant == 3 ? 0.f : 3.f);
        set("p1cutoff", 2600 + variant * 550.f);
        set("p1ampd", .65f + variant * .1f);
        set("p1amps", variant == 3 ? .8f : .25f);
        set("p1ampr", .35f);
        set("reverbon", 1);
        set("reverb", .14f);
        set("reverbdecay", 1.8f);
        if (variant == 1 || variant == 2) {
            set("o1wave", 0);
            set("o1fmdepth", variant == 1 ? 1.2f : 2.4f);
            set("o1fmratio", variant == 1 ? 2.f : 3.5f);
            set("p1amps", 0);
        }
        if (variant == 3) {
            set("o2level", .25f);
            set("o2wave", 0);
            set("o2octave", 1);
        }
        if (variant == 4) {
            set("o1wave", 2);
            set("o1pulsewidth", .22f);
            set("choruson", 1);
        }
        if (variant == 5) {
            set("polyphony", 4);
            set("o1engine", 2);
            set("o1stretchmode", 7);
            set("o1ssblock", 48);
            set("o1ssformant", 3);
            set("o1loop", 0);
        }
    } else if (group == 5) {
        set("arpon", 1);
        set("arpmode", 8);
        set("arpsteps", 8);
        set("arpdivision", float(7 + variant % 3));
        set("arpgate", .58f);
        set("p1cutoff", 1800 + variant * 400.f);
        set("env1amt1", 2.5f);
        set("env1d", .12f);
        set("p1ampd", .16f);
        set("p1amps", .18f);
        set("p1ampr", .09f);
        set("o1wave", variant == 1 ? 2.f : variant == 3 ? 3.f : 1.f);
        set("delayon", 1);
        set("delay", .17f);
        const int sequence[]{0, 12, 0, 7, 3, 12, 7, 10};
        for (int step = 1; step <= 8; ++step) {
            auto base = "step" + std::to_string(step);
            set((base + "transpose").c_str(), float(sequence[(step + variant) % 8]));
            set((base + "velocity").c_str(), float(75 + (step * 7 + variant * 5) % 40));
        }
        if (variant == 2) {
            set("o1engine", 1);
            set("o1position", .65f);
        }
        if (variant == 5) {
            set("chordon", 1);
            set("arppattern", 1);
            set("arpmode", 0);
        }
    } else if (group == 6) {
        const float positions[]{.25f, .8f, .48f, .6f, .72f, .35f};
        set("o1engine", 1);
        set("o1position", positions[variant]);
        set("o1voices", 3);
        set("o1detune", 9);
        set("o1stereo", .7f);
        set("lfo1dest1", 34);
        set("lfo1amt1", .16f);
        set("lfo1rate", .09f + variant * .025f);
        set("p1cutoff", variant == 4 ? 1600.f : 6500.f);
        set("reverbon", 1);
        set("reverb", .16f);
        if (variant == 1) {
            set("o1pulsewidth", .3f);
            set("diston", 1);
            set("drive", .12f);
        }
        if (variant == 2) {
            set("o1fmdepth", .45f);
            set("o1fmratio", 2);
        }
        if (variant == 3) {
            set("p1amps", 0);
            set("p1ampd", .4f);
            set("p1ampr", .25f);
            set("delayon", 1);
        }
        if (variant == 5) {
            set("o1voices", 5);
            set("p1ampa", .12f);
            set("p1ampr", 1.1f);
        }
    } else {
        set("polyphony", 4);
        set("o1engine", 2);
        set("o1stretchmode", 7);
        set("o1loop", 1);
        set("o1ssblock", 80);
        set("o1ssoverlap", 7);
        set("p1cutoff", 8000);
        set("p1ampa", .04f);
        set("p1ampd", .6f);
        set("p1amps", .65f);
        set("p1ampr", .7f);
        set("reverbon", 1);
        set("reverb", .21f);
        set("reverbdecay", 2.6f);
        if (variant == 0)
            set("o1ssrate", .75f);
        if (variant == 1) {
            set("o1ssformant", 7);
            set("o1sstonality", 2500);
        }
        if (variant == 2) {
            set("o1stretchmode", 6);
            set("o1ssrate", .5f);
            set("o1loop", 0);
            set("o1kickattack", 25);
        }
        if (variant == 3) {
            set("o1engine", 3);
            set("o1gmodel", 1);
            set("o1gattack", 15);
            set("o1ghold", 55);
            set("o1gspace", .5f);
            set("o1gwave", .7f);
            set("o1grandom", .12f);
        }
        if (variant == 4) {
            set("o1stretchmode", 1);
            set("o1reverse", 1);
            set("o1stretch", 1.75f);
            set("o1loop", 0);
        }
        if (variant == 5) {
            set("o1engine", 3);
            set("o1gmodel", 0);
            set("o1position", .08f);
            set("o1size", 120);
            set("o1density", 18);
            set("o1spray", .22f);
            set("o1scan", .05f);
            set("o1level", .9f);
            set("o2wave", 3);
            set("o2level", .22f);
            set("p1amps", .78f);
            set("p1ampa", .4f);
            set("p1ampr", 1.8f);
            set("reverbtype", 3);
        }
    }
    set("master", designedMainLevels[size_t(index)]);
}
} // namespace logisth
