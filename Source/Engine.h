#pragma once
#include "Envelope.h"
#include "Gain.h"
#include "Granular.h"
#include "Modulation.h"
#include "SourceData.h"
#include "Stereo.h"
#include "Telemetry.h"
#include <cstdint>
#include <cstring>
#include <type_traits>
namespace logisth {
struct OscParams {
    int wave = 1, unison = 1;
    float blend = 1;
    float level = .65f, detune = 12, pan = 0, semitone = 0, phase = 0, stereo = .6f;
    bool retrigger = false, invert = false;
    int engine = 0, root = 60;
    float position = 0, end = 1, size = .08f, density = 16, spray = .15f, scan = 0, warp = 0;
    bool loop = true, reverse = false, envelopeEnabled = false;
    EnvelopeParams envelope{.008f, .25f, 1, .35f};
    float fmDepth = 0, fmRatio = 1, amDepth = 0, amRatio = 1, pulseWidth = .5f, pwmDepth = 0,
          pwmRate = .5f, stretch = 1;
    int stretchMode = 0, interpolation = 0;
    int fmSource = 0, amSource = 0, pwmDivision = 7;
    bool pwmSync = false;
    float kickAttack = 25;
    StretchControls stretchControls;
    TailLoopSettings tail;
    const SourceData *data = nullptr;
    GranularParams gran;
};
struct PartParams {
    EnvelopeParams amp;
    float cutoff = 5000, resonance = .15f, drive = 0, level = .7f;
    int filterType = 0, input = 0, slope = 0;
};
struct Params {
    SpectralBank *spectralBank = nullptr;
    std::array<OscParams, playerCount> osc{};
    std::array<PartParams, partCount> part{};
    std::array<EnvelopeParams, 2> envelopes{};
    std::array<Routes, 2> envelopeRoutes{};
    std::array<LfoParams, 2> lfo{};
    std::array<Misc, 4> misc{};
    float filterCutoff = 0, filterResonance = 0, keytrack = 0, portamento = 0, bendRange = 2;
    float bendInputRange = 2;
    double bpm = 120;
    int polyphony = defaultPolyphony, solo = 0, portaMode = 0;
    bool fixedVelocity = false;
    bool warmDrive = false, sync = true, monoLegato = false;
    Params() {
        osc[1].level = .3f;
        osc[1].semitone = -12;
        osc[2].level = osc[3].level = osc[4].level = osc[5].level = 0;
        part[2].input = 4;
        part[1].input = 1;
        for (auto &e : envelopes)
            e = {0, 0, 0, 0};
    }
};
struct Filter {
    float ic1 = 0, ic2 = 0;
    float tick(float input, float cutoff, float resonance, int type, double sr, float drive,
               bool warm) {
        const float gain = 1 + drive * 12;
        auto saturate = [&](float x) { return warm ? std::tanh(x) : x / (1 + std::abs(x)); };
        if (drive > 0)
            input = saturate(input * gain);
        const float g = std::tan(float(pi) * clamp(cutoff, 20, float(sr * .43)) / float(sr));
        const float k = 2 - 2.01f * clamp(resonance, 0, 1), a1 = 1 / (1 + g * (g + k));
        const float v1 = a1 * (ic1 + g * (input - ic2)), v2 = ic2 + g * v1;
        ic1 = 2 * v1 - ic1;
        ic2 = 2 * v2 - ic2;
        if (drive > 0 || resonance > .97f) {
            ic1 = saturate(ic1);
            ic2 = saturate(ic2);
        }
        return type == 1 ? input - k * v1 - v2 : type == 2 ? v1 : v2;
    }
};
class Engine {
    struct LfoState {
        double phase = 0;
        float held = 0, next = 0;
    };
    struct Voice {
        int note = -1, channel = 1;
        bool held = false, sustained = false;
        uint64_t age = 0;
        float velocity = 0, pressure = -1, random = 0, pitch = 60, glideStep = 0;
        std::array<std::array<double, unisonCapacity>, playerCount> phases{};
        std::array<double, playerCount> phaseOffset{};
        std::array<std::array<double, unisonCapacity>, playerCount> fmPhase{}, amPhase{};
        std::array<double, playerCount> pwmPhase{};
        std::array<float, playerCount> modulationSignal{};
        std::array<std::array<float, 7>, playerCount> oscillatorControls{};
        struct NoiseState {
            std::array<float, 7> pink{};
            float brown = 0;
        };
        std::array<std::array<NoiseState, unisonCapacity>, playerCount> noiseState{};
        std::array<AudioEnvelope, partCount> amp;
        std::array<Envelope, 2> env;
        std::array<AudioEnvelope, playerCount> sourceEnv;
        std::array<float, playerCount> sourcePosition{}, blend{};
        std::array<int, playerCount> sourceModes{-1, -1, -1, -1, -1, -1};
        std::array<std::array<SourcePlayback, unisonCapacity>, playerCount> playback{};
        std::array<std::array<GranulizerPlayback, unisonCapacity>, playerCount> granular{};
        std::array<std::array<std::array<Filter, 2>, 2>, partCount> filters{};
        std::array<float, partCount> cutoff{5000, 5000, 5000};
        std::array<LfoState, 2> lfo{};
        ModValues previous{};
        bool cEnabled = false;
        bool active() const {
            return amp[0].active() || amp[1].active() || (cEnabled && amp[2].active());
        }
    };
    static_assert(std::is_trivially_copyable_v<Voice>,
                  "Voice resets must remain allocation-free copies of prepared storage");
    std::vector<Voice> voices = std::vector<Voice>(maxPolyphony);
    const std::unique_ptr<const Voice> initialVoice = std::make_unique<Voice>();
    void clearVoice(Voice &voice) const noexcept {
        // MSVC can materialize nested aggregate initializers on the audio stack,
        // even for placement construction. Copy an already prepared heap object.
        std::memcpy(&voice, initialVoice.get(), sizeof(Voice));
    }
    std::array<PlaybackPicture, playerCount> pictures{};
    std::array<float, 16> bend{}, wheel{}, pressure{};
    std::array<bool, 16> localBend{};
    std::array<bool, 16> sustain{};
    struct Key {
        int note = -1, channel = 1;
        float velocity = 0;
        uint64_t age = 0;
    };
    std::array<Key, 128> keys{};
    std::array<LfoState, 2> freeLfo{};
    double sr = 44100;
    uint64_t counter = 0;
    uint32_t randomState = 0x67a8451;
    float lastPitch = 60, stepVelocity = 0;
    float noise() {
        randomState ^= randomState << 13;
        randomState ^= randomState >> 17;
        randomState ^= randomState << 5;
        return float(double(randomState) / 2147483648. - 1);
    }
    static double blep(double t, double dt) {
        if (t < dt) {
            t /= dt;
            return t + t - t * t - 1;
        }
        if (t > 1 - dt) {
            t = (t - 1) / dt;
            return t * t + t + t + 1;
        }
        return 0;
    }
    float wave(double t, double dt, int shape, Voice::NoiseState &state, float width = .5f) {
        switch (shape) {
        case 0:
            return float(std::sin(2 * pi * t));
        case 1:
            return float(2 * t - 1 - blep(t, dt));
        case 2:
            return float((t < width ? 1 : -1) + blep(t, dt) -
                         blep(std::fmod(t + 1 - width, 1.), dt));
        case 3:
            return float(1 - 4 * std::abs(t - .5));
        case 4:
            return noise();
        case 5:
        case 6: {
            const double duty = width * (shape == 5 ? .5 : .25);
            return float((t < duty ? 1 : -1) + blep(t, dt) - blep(std::fmod(t + 1 - duty, 1.), dt));
        }
        case 8: {
            float white = noise();
            auto &b = state.pink;
            b[0] = .99886f * b[0] + white * .0555179f;
            b[1] = .99332f * b[1] + white * .0750759f;
            b[2] = .969f * b[2] + white * .153852f;
            b[3] = .8665f * b[3] + white * .3104856f;
            b[4] = .55f * b[4] + white * .5329522f;
            b[5] = -.7616f * b[5] - white * .016898f;
            float out = (b[0] + b[1] + b[2] + b[3] + b[4] + b[5] + b[6] + white * .5362f) * .11f;
            b[6] = white * .115926f;
            return out;
        }
        case 9:
            state.brown = (state.brown + noise() * .04f) / 1.02f;
            return state.brown * 3.5f;
        case 10:
            return float(
                (std::sin(2 * pi * t) + .5 * std::sin(4 * pi * t) + .25 * std::sin(8 * pi * t)) /
                1.75);
        case 11:
            return float(std::sin(3 * std::sin(2 * pi * t)));
        default:
            return float((2 * t - 1 + 1 - 4 * std::abs(t - .5)) * .5 - blep(t, dt) * .5);
        }
    }
    static bool wantsPartC(const Params &p) {
        if (p.part[2].input != 4 && p.part[2].input != 3 && p.part[2].level > 0)
            return true;
        if ((p.osc[4].unison > 0 && p.osc[4].level > 0) ||
            (p.osc[5].unison > 0 && p.osc[5].level > 0))
            return true;
        auto routed = [](const Routes &routes) {
            for (const auto &route : routes)
                if (route.amount != 0 && (route.destination == Destination::VolumeC ||
                                          route.destination == Destination::VolumeABC ||
                                          route.destination == Destination::VolumeAC ||
                                          route.destination == Destination::VolumeBC))
                    return true;
            return false;
        };
        for (const auto &misc : p.misc)
            if (misc.source == Source::AmpEnvC || routed(misc.routes))
                return true;
        for (int i = 0; i < 2; ++i)
            if (routed(p.envelopeRoutes[i]) || routed(p.lfo[i].routes))
                return true;
        return false;
    }
    void releaseVoice(Voice &v, const Params &p) {
        v.held = v.sustained = false;
        for (int i = 0; i < playerCount; ++i)
            v.sourceEnv[size_t(i)].releaseNote(p.osc[size_t(i)].envelope.release, sr);
        for (int i = 0; i < partCount; ++i)
            v.amp[i].releaseNote(p.part[i].amp.release, sr);
        for (int i = 0; i < 2; ++i)
            v.env[i].releaseNote(p.envelopes[i].release, sr);
    }
    float tickLfo(LfoState &state, const LfoParams &lp, const Params &p, float rateMod,
                  float gainMod) {
        const float result =
            lfoWave(state.phase, lp.wave, state.held, state.next) * clamp(lp.gain + gainMod, 0, 2) +
            lp.offset;
        const double rate = std::clamp(
            lfoRate(lp, p.sync, p.bpm) * std::exp2(double(clamp(rateMod, -4, 4)) * 4), .001, 200.);
        state.phase += rate / sr;
        if (state.phase >= 1) {
            state.phase -= std::floor(state.phase);
            state.held = state.next;
            state.next = noise();
        }
        return result;
    }
    static float sourceValue(Source s, const Voice &v, const std::array<float, partCount> &amps,
                             const std::array<float, 2> &envs, const std::array<float, 2> &lfos,
                             float aftertouch, float modwheel, float pitchwheel, float step) {
        switch (s) {
        case Source::None:
            return 0;
        case Source::Velocity:
            return v.velocity;
        case Source::Keytrack:
            return (v.note - 60) / 60.f;
        case Source::Aftertouch:
            return aftertouch;
        case Source::ModWheel:
            return modwheel;
        case Source::PitchWheel:
            return pitchwheel;
        case Source::AmpEnvA:
            return amps[0];
        case Source::AmpEnvB:
            return amps[1];
        case Source::AmpEnvC:
            return amps[2];
        case Source::ModEnv1:
            return envs[0];
        case Source::ModEnv2:
            return envs[1];
        case Source::Lfo1:
            return lfos[0];
        case Source::Lfo2:
            return lfos[1];
        case Source::StepVelocity:
            return step;
        case Source::Random:
            return v.random;
        case Source::RandomBipolar:
            return 2 * v.random - 1;
        default:
            return 0;
        }
    }

  public:
    void prepare(double sampleRate) {
        sr = std::max(8000., sampleRate);
        reset();
    }
    void reset() {
        for (auto &voice : voices)
            clearVoice(voice);
        bend = {};
        localBend = {};
        wheel = {};
        pressure = {};
        sustain = {};
        keys = {};
        freeLfo = {};
        counter = 0;
        lastPitch = 60;
        stepVelocity = 0;
    }
    void silenceVoices() {
        for (auto &v : voices)
            clearVoice(v);
        keys = {};
    }
    void setStepVelocity(float value) {
        stepVelocity = clamp(value, 0, 1);
    }
    void syncPosition(double ppq, const Params &p) {
        if (p.sync)
            for (int i = 0; i < 2; ++i)
                if (!p.lfo[i].free) {
                    double phase = ppq / beatLength(p.lfo[i].division);
                    phase -= std::floor(phase);
                    for (auto &v : voices)
                        v.lfo[i].phase = phase;
                }
    }
    void noteOn(int note, float velocity, int channel, const Params &p) {
        if (channel < 1 || channel > 16 || note < 0 || note > 127)
            return;
        if (velocity <= 0) {
            noteOff(note, channel, p);
            return;
        }
        const bool overlap =
            std::any_of(keys.begin(), keys.end(), [](auto &k) { return k.note >= 0; });
        auto key = std::find_if(keys.begin(), keys.end(),
                                [&](auto &k) { return k.note == note && k.channel == channel; });
        if (key == keys.end())
            key = std::find_if(keys.begin(), keys.end(), [](auto &k) { return k.note < 0; });
        if (key != keys.end())
            *key = {note, channel, velocity, ++counter};
        Voice *selected = nullptr;
        const int limit = p.monoLegato ? 1 : std::clamp(p.polyphony, 1, maxPolyphony);
        if (p.monoLegato) {
            selected = &voices[0];
            for (size_t i = 1; i < voices.size(); ++i)
                clearVoice(voices[i]);
            if (selected->active() && overlap) {
                selected->glideStep = std::abs(float(note) - selected->pitch) /
                                      float(std::max(1., p.portamento * sr));
                selected->note = note;
                selected->channel = channel;
                selected->velocity = p.fixedVelocity ? 1.f : velocity;
                selected->held = true;
                selected->sustained = false;
                selected->age = counter;
                if (p.portamento <= 0)
                    selected->pitch = float(note);
                lastPitch = selected->pitch;
                return;
            }
        }
        for (int i = 0; i < limit && !selected; ++i)
            if (!voices[size_t(i)].active())
                selected = &voices[size_t(i)];
        if (!selected)
            selected = &*std::min_element(voices.begin(), voices.begin() + limit,
                                          [](auto &a, auto &b) { return a.age < b.age; });
        auto &v = *selected;
        clearVoice(v);
        v.note = note;
        v.channel = channel;
        v.held = true;
        v.velocity = p.fixedVelocity ? 1.f : clamp(velocity, 0, 1);
        v.age = ++counter;
        v.random = (noise() + 1) * .5f;
        v.pitch = p.portamento > 0 && (overlap || p.portaMode == 1) ? lastPitch : float(note);
        lastPitch = float(note);
        v.glideStep = std::abs(float(note) - v.pitch) / float(std::max(1., p.portamento * sr));
        for (auto &part : v.filters)
            for (auto &channel : part)
                for (auto &filter : channel)
                    filter.ic1 = 1e-6f;
        for (int i = 0; i < 2; ++i) {
            v.amp[i].on();
            v.env[i].on();
            v.cutoff[i] = p.part[i].cutoff;
            v.lfo[i].held = noise();
            v.lfo[i].next = noise();
            if (p.lfo[i].free)
                v.lfo[i] = freeLfo[i];
        }
        v.amp[2].on();
        v.cutoff[2] = p.part[2].cutoff;
        v.cEnabled = wantsPartC(p);
        for (size_t o = 0; o < playerCount; ++o) {
            v.oscillatorControls[o] = {p.osc[o].fmDepth, p.osc[o].fmRatio,    p.osc[o].amDepth,
                                       p.osc[o].amRatio, p.osc[o].pulseWidth, p.osc[o].pwmDepth,
                                       p.osc[o].pwmRate};
            v.sourceEnv[o].on();
            v.sourcePosition[o] = p.osc[o].position;
            v.blend[o] = p.osc[o].blend;
            v.phaseOffset[o] = double(p.osc[o].phase) / 360.;
            v.phaseOffset[o] -= std::floor(v.phaseOffset[o]);
            for (size_t u = 0; u < v.phases[o].size(); ++u) {
                // C has an independent seed so adding a silent part never advances A/B's RNG.
                uint32_t seed = uint32_t(v.age * 747796405u + o * 2891336453u + u * 277803737u);
                seed ^= seed >> 16;
                seed *= 2246822519u;
                seed ^= seed >> 13;
                v.phases[o][u] = p.osc[o].retrigger ? 0
                                 : o < 4 && u < 8   ? (noise() + 1) * .5
                                                    : double(seed) / 4294967296.;
            }
        }
    }
    void noteOff(int note, int channel, const Params &p) {
        if (channel < 1 || channel > 16)
            return;
        for (auto &k : keys)
            if (k.note == note && k.channel == channel)
                k = Key{};
        if (p.monoLegato && voices[0].note == note && voices[0].channel == channel) {
            auto k = std::max_element(keys.begin(), keys.end(),
                                      [](auto &a, auto &b) { return a.age < b.age; });
            if (k->note >= 0) {
                auto &v = voices[0];
                v.glideStep =
                    std::abs(float(k->note) - v.pitch) / float(std::max(1., p.portamento * sr));
                v.note = k->note;
                v.channel = k->channel;
                v.velocity = p.fixedVelocity ? 1.f : k->velocity;
                if (p.portamento <= 0)
                    v.pitch = float(k->note);
                return;
            }
        }
        for (auto &v : voices)
            if (v.note == note && v.channel == channel && v.held) {
                v.held = false;
                if (sustain[size_t(channel - 1)])
                    v.sustained = true;
                else
                    releaseVoice(v, p);
            }
    }
    void pitchWheel(int channel, float normalized, bool local = false) {
        if (channel >= 1 && channel <= 16) {
            bend[size_t(channel - 1)] = clamp(normalized, -1, 1);
            localBend[size_t(channel - 1)] = local;
        }
    }
    float pitchBendCents(int channel, const Params &p) const {
        const auto index = size_t(channel - 1);
        const float limit = p.bendRange * 100.f;
        const float input = localBend[index] ? limit : p.bendInputRange * 100.f;
        return clamp(bend[index] * input, -limit, limit);
    }
    void aftertouch(int channel, int note, float value) {
        if (channel < 1 || channel > 16)
            return;
        if (note < 0)
            pressure[size_t(channel - 1)] = clamp(value, 0, 1);
        else
            for (auto &v : voices)
                if (v.channel == channel && v.note == note)
                    v.pressure = clamp(value, 0, 1);
    }
    void controller(int channel, int cc, int value, const Params &p) {
        if (channel < 1 || channel > 16)
            return;
        const auto ch = size_t(channel - 1);
        if (cc == 1)
            wheel[ch] = value / 127.f;
        if (cc == 64) {
            sustain[ch] = value >= 64;
            if (!sustain[ch])
                for (auto &v : voices)
                    if (v.channel == channel && v.sustained)
                        releaseVoice(v, p);
        }
        if (cc == 123 || cc == 120) {
            for (auto &k : keys)
                if (k.channel == channel)
                    k = Key{};
            for (auto &v : voices)
                if (v.channel == channel) {
                    if (cc == 120)
                        clearVoice(v);
                    else
                        releaseVoice(v, p);
                }
        }
        if (cc == 121) {
            wheel[ch] = bend[ch] = pressure[ch] = 0;
            localBend[ch] = false;
            sustain[ch] = false;
            for (auto &v : voices)
                if (v.channel == channel && v.sustained)
                    releaseVoice(v, p);
        }
    }
    uint64_t oscillatorSamples = 0;
    int activeVoices() const {
        int n = 0;
        for (auto &v : voices)
            n += v.active();
        return n;
    }
    void render(float *left, float *right, int count, const Params &p, float *distortion = nullptr,
                float *phaser = nullptr, float *partA = nullptr, float *partB = nullptr,
                const std::array<float *, playerCount> &oscScopes = {}, float *partC = nullptr) {
        oscillatorSamples = 0;
        pictures = {};
        const int maxVoices = p.monoLegato ? 1 : std::clamp(p.polyphony, 1, maxPolyphony);
        for (size_t i = size_t(maxVoices); i < voices.size(); ++i)
            if (voices[i].active())
                clearVoice(voices[i]);
        if (activeVoices() == 0) {
            std::array<double, 2> step{};
            for (size_t i = 0; i < 2; ++i)
                step[i] = std::clamp(lfoRate(p.lfo[i], p.sync, p.bpm), .001, 200.) / sr;
            for (int s = 0; s < count; ++s)
                for (size_t i = 0; i < 2; ++i) {
                    auto &l = freeLfo[i];
                    l.phase += step[i];
                    if (l.phase >= 1) {
                        l.phase -= std::floor(l.phase);
                        l.held = l.next;
                        l.next = noise();
                    }
                }
            for (auto *buffer : {left, right, distortion, phaser, partA, partB, partC})
                if (buffer)
                    std::fill_n(buffer, count, 0);
            for (auto *buffer : oscScopes)
                if (buffer)
                    std::fill_n(buffer, count, 0);
            return;
        }
        std::array<bool, playerCount> needed{};
        const bool cWanted = wantsPartC(p);
        // Include only audible carriers and their active FM/AM dependencies.
        // Route magnitudes bound bipolar modulation; retain smoothed depths while
        // they decay after automation. This runs once per render, not per sample.
        ModValues potential;
        auto addPotential = [&](Routes routes) {
            for (auto &r : routes)
                r.amount = std::abs(r.amount);
            potential.add(routes, 1);
        };
        for (const auto &r : p.envelopeRoutes)
            addPotential(r);
        for (const auto &l : p.lfo)
            addPotential(l.routes);
        for (const auto &m : p.misc)
            if (m.source != Source::None)
                addPotential(m.routes);
        std::array<bool, playerCount> reachable{};
        std::array<float, playerCount> fmDepth{}, amDepth{};
        for (int i = 0; i < playerCount; ++i) {
            reachable[size_t(i)] =
                p.osc[size_t(i)].unison > 0 &&
                (p.osc[size_t(i)].level > 0 || potential.pair(Destination::VolumeA, i / 2) > 0);
            fmDepth[size_t(i)] = p.osc[size_t(i)].fmDepth + 8 * potential.osc(Destination::FmA1, i);
            amDepth[size_t(i)] = p.osc[size_t(i)].amDepth + potential.osc(Destination::AmA1, i);
            for (const auto &v : voices)
                if (v.active()) {
                    fmDepth[size_t(i)] =
                        std::max(fmDepth[size_t(i)], v.oscillatorControls[size_t(i)][0]);
                    amDepth[size_t(i)] =
                        std::max(amDepth[size_t(i)], v.oscillatorControls[size_t(i)][2]);
                }
        }
        for (int pass = 0; pass < playerCount; ++pass)
            for (int i = 0; i < playerCount; ++i)
                if (reachable[size_t(i)]) {
                    auto require = [&](int source, float depth) {
                        if (source > 0 && source <= playerCount && depth > 1e-7f &&
                            p.osc[size_t(source - 1)].unison > 0) {
                            needed[size_t(source - 1)] = true;
                            reachable[size_t(source - 1)] = true;
                        }
                    };
                    require(p.osc[size_t(i)].fmSource, fmDepth[size_t(i)]);
                    require(p.osc[size_t(i)].amSource, amDepth[size_t(i)]);
                }
        oscillatorSamples = 0;
        pictures = {};
        std::array<uint64_t, playerCount> pictureAges{};
        const float smoothing = float(1 - std::exp(-1 / (.01 * sr)));
        const double phaseSmoothing = 1 - std::exp(-1 / (.005 * sr));
        std::array<float, 16> bendSemitones{};
        for (int channel = 1; channel <= 16; ++channel)
            bendSemitones[size_t(channel - 1)] = pitchBendCents(channel, p) / 100.f;

        for (int s = 0; s < count; ++s) {
            float out[2]{};
            float parts[partCount]{};
            std::array<float, playerCount> traces{};

            float fxDrive = 0, fxPhaser = 0;
            uint64_t fxAge = 0;
            for (int i = 0; i < 2; ++i)
                tickLfo(freeLfo[i], p.lfo[i], p, 0, 0);
            for (int voiceIndex = 0; voiceIndex < maxVoices; ++voiceIndex) {
                auto &v = voices[size_t(voiceIndex)];
                if (cWanted && v.note >= 0 && (v.held || v.sustained))
                    v.cEnabled = true;
                if (!v.active())
                    continue;
                if (p.portamento > 0) {
                    float step = v.glideStep;
                    v.pitch += clamp(float(v.note) - v.pitch, -step, step);
                } else
                    v.pitch = float(v.note);
                std::array<float, partCount> amps{};
                std::array<float, 2> envs{}, lfos{};
                for (int i = 0; i < 2; ++i) {
                    amps[i] = v.amp[i].tick(p.part[i].amp, sr);
                    envs[i] = v.env[i].tick(p.envelopes[i], sr);
                    const auto r = i == 0 ? Destination::Lfo1Rate : Destination::Lfo2Rate;
                    const auto g = i == 0 ? Destination::Lfo1Gain : Destination::Lfo2Gain;
                    // Feedback destinations use the preceding sample, avoiding an algebraic loop.
                    lfos[i] = tickLfo(v.lfo[i], p.lfo[i], p, v.previous.get(r), v.previous.get(g));
                }
                amps[2] = v.amp[2].tick(p.part[2].amp, sr);
                ModValues mods;
                for (int i = 0; i < 2; ++i) {
                    mods.add(p.envelopeRoutes[i], envs[i]);
                    mods.add(p.lfo[i].routes, lfos[i]);
                }
                for (const auto &misc : p.misc)
                    mods.add(
                        misc.routes,
                        sourceValue(misc.source, v, amps, envs, lfos,
                                    v.pressure >= 0 ? v.pressure : pressure[size_t(v.channel - 1)],
                                    wheel[size_t(v.channel - 1)], bend[size_t(v.channel - 1)],
                                    stepVelocity));
                v.previous = mods;
                if (v.age >= fxAge) {
                    fxAge = v.age;
                    fxDrive = mods.get(Destination::DistortionAmount);
                    fxPhaser = mods.get(Destination::PhaserFrequency);
                }
                float oscillators[partCount][2]{};
                const auto previousSignals = v.modulationSignal;
                v.modulationSignal = {};
                for (int osc = 0; osc < playerCount; ++osc) {
                    const int part = osc / 2;
                    const auto &op = p.osc[size_t(osc)];
                    // PHASE is a live cyclic offset, independent of the note-start
                    // reset policy. Update once per oscillator, including while muted.
                    auto &offset = v.phaseOffset[size_t(osc)];
                    double phaseDelta = double(op.phase) / 360. - offset;
                    if (phaseDelta > .5)
                        phaseDelta -= 1;
                    if (phaseDelta < -.5)
                        phaseDelta += 1;
                    if (phaseDelta != 0) {
                        offset += phaseSmoothing * phaseDelta;
                        offset -= std::floor(offset);
                    }
                    auto &control = v.oscillatorControls[size_t(osc)];
                    const std::array<float, 7> target{op.fmDepth, op.fmRatio,    op.amDepth,
                                                      op.amRatio, op.pulseWidth, op.pwmDepth,
                                                      op.pwmRate};
                    for (int c = 0; c < 7; ++c)
                        control[size_t(c)] +=
                            float(phaseSmoothing) * (target[size_t(c)] - control[size_t(c)]);
                    int mode = op.engine == 3 && op.gran.enabled ? 4 : op.engine;
                    if (v.sourceModes[size_t(osc)] != mode) {
                        v.sourceModes[size_t(osc)] = mode;
                        for (auto &player : v.playback[size_t(osc)])
                            player.reset();
                        for (auto &player : v.granular[size_t(osc)])
                            player.reset();
                    }
                    float sourceEnvelope = v.sourceEnv[size_t(osc)].tick(op.envelope, sr);
                    const float level =
                        clamp(op.level + mods.pair(Destination::VolumeA, part), 0, 2) *
                        (op.envelopeEnabled ? sourceEnvelope : 1.f);
                    const int n = std::clamp(op.unison, 0, 8);
                    if (n == 0 || (level <= 0 && !needed[size_t(osc)]))
                        continue;
                    oscillatorSamples += uint64_t(n);
                    const float pitch = bendSemitones[size_t(v.channel - 1)] +
                                        mods.pair(Destination::PitchA, part) * 48;
                    float targetPosition = clamp(op.position + mods.position(osc), 0, 1);
                    auto &position = v.sourcePosition[size_t(osc)];
                    position += smoothing * (targetPosition - position);
                    float fm = clamp(control[0] + mods.osc(Destination::FmA1, osc) * 8, 0, 8);
                    float am = clamp(control[2] + mods.osc(Destination::AmA1, osc), 0, 1);
                    float duty =
                        clamp(control[4] + mods.osc(Destination::WidthA1, osc) +
                                  control[5] * float(std::sin(2 * pi * v.pwmPhase[size_t(osc)])),
                              .01f, .99f);
                    const float pwmRate =
                        op.pwmSync ? float(p.bpm / (60 * beatLength(op.pwmDivision))) : control[6];
                    v.pwmPhase[size_t(osc)] += pwmRate / sr;
                    v.pwmPhase[size_t(osc)] -= std::floor(v.pwmPhase[size_t(osc)]);
                    auto &blend = v.blend[size_t(osc)];
                    const float blendTarget = clamp(op.blend, 0, 1);
                    blend += smoothing * (blendTarget - blend);
                    if (std::abs(blend - blendTarget) < 1e-5f)
                        blend = blendTarget;
                    const bool even = n > 1 && n % 2 == 0;
                    const bool dryLane = even && blend < 1;
                    const float norm = even ? n * blend * blend + (1 - blend) * (1 - blend)
                                            : 1 + (n - 1) * blend * blend;
                    const float weightSum = even ? n * blend + (1 - blend) : 1 + (n - 1) * blend;
                    const int displayLane = dryLane && blend < .5f ? n : 0;
                    for (int lane = 0; lane < n + int(dryLane); ++lane) {
                        const int u = lane;
                        const bool center = lane == n || (!even && u == n / 2);
                        const float weight = center ? (even ? 1 - blend : 1.f) : blend;
                        if (weight == 0)
                            continue;
                        const float spread = center ? 0 : 2.f * u / (n - 1) - 1;
                        const double hz = 440 * std::exp2((v.pitch - 69 + pitch + op.semitone +
                                                           spread * op.detune * .01) /
                                                          12.);
                        const double dt = std::min(.45, hz / sr);
                        auto &phase = v.phases[size_t(osc)][size_t(u)];
                        auto &fmPhase = v.fmPhase[size_t(osc)][size_t(u)];
                        auto &amPhase = v.amPhase[size_t(osc)][size_t(u)];
                        const double fmInput =
                            fm > 0 ? (op.fmSource > 0 ? previousSignals[size_t(op.fmSource - 1)]
                                                      : std::sin(2 * pi * fmPhase))
                                   : 0;
                        const double phaseMod = fm > 0 ? fm / (2 * pi) * fmInput : 0;
                        double t = phase + offset + mods.pair(Destination::PhaseA, part) + phaseMod;
                        fmPhase += dt * control[1];
                        fmPhase -= std::floor(fmPhase);
                        const double amInput =
                            am > 0 ? (op.amSource > 0 ? previousSignals[size_t(op.amSource - 1)]
                                                      : std::sin(2 * pi * amPhase))
                                   : 0;
                        double amGain =
                            am > 0 ? 1 - am * .5 * (1 - std::clamp(amInput, -1., 1.)) : 1;
                        amPhase += dt * control[3];
                        amPhase -= std::floor(amPhase);
                        t -= std::floor(t);
                        std::array<float, 2> signal{};
                        if (op.engine == 0) {
                            bool pulse = op.wave == 2 || op.wave == 5 || op.wave == 6;
                            bool noiseWave = op.wave == 4 || op.wave == 8 || op.wave == 9;
                            signal.fill(wave(pulse || noiseWave ? t : phaseWidth(t, duty), dt,
                                             op.wave, v.noiseState[size_t(osc)][size_t(u)], duty));
                            if (noiseWave) {
                                float strength =
                                    clamp(std::abs(control[4] - .5f) * 2 + control[5] * 2 +
                                              std::abs(mods.osc(Destination::WidthA1, osc)) * 2,
                                          0, 1);
                                double gatePhase = op.pwmSync ? v.pwmPhase[size_t(osc)] : t;
                                const double edge =
                                    std::min(.02, std::min(double(duty), 1 - double(duty)) * .25);
                                double gate = std::clamp((double(duty) - gatePhase) / edge, 0., 1.);
                                gate *= std::clamp(gatePhase / edge, 0., 1.);
                                for (auto &x : signal)
                                    x *= 1 - strength * float(1 - gate);
                            }
                        } else if (op.data) {
                            if (op.engine == 1) {
                                t = phaseWidth(t, duty);
                                t += op.warp * .15 * std::sin(2 * pi * t);
                                t -= std::floor(t);
                                signal.fill(op.data->table(t, position, dt));
                            } else {
                                const double samplePhase =
                                    phaseMod * op.data->sampleRate / std::max(20., hz);
                                double ratio = std::exp2((v.pitch - op.root + pitch + op.semitone +
                                                          spread * op.detune * .01) /
                                                         12.);
                                if (op.engine == 3 && op.gran.enabled) {
                                    signal = v.granular[size_t(osc)][size_t(u)].tick(
                                        *op.data, op.gran, sr, ratio, position, op.end, op.loop,
                                        op.reverse, v.note, op.root, p.bpm,
                                        std::exp2(
                                            clamp(mods.pair(Destination::GrainSizeA, part), -4, 4) *
                                            4),
                                        duty, samplePhase);
                                } else {
                                    v.playback[size_t(osc)][size_t(u)].setSpectral(
                                        p.spectralBank ? p.spectralBank->get(
                                                             int(&v - voices.data()), osc, u, 0)
                                                       : nullptr,
                                        p.spectralBank ? p.spectralBank->get(
                                                             int(&v - voices.data()), osc, u, 1)
                                                       : nullptr,
                                        v.age);
                                    signal = v.playback[size_t(osc)][size_t(u)].tick(
                                        *op.data, op.engine, ratio, sr, position, op.end, op.loop,
                                        op.reverse,
                                        clamp(op.size *
                                                  std::exp2(
                                                      mods.pair(Destination::GrainSizeA, part) * 4),
                                              .005f, .25f),
                                        op.density, op.spray, op.scan,
                                        op.engine == 3 &&
                                                v.playback[size_t(osc)][size_t(u)].untilGrain <= 0
                                            ? noise()
                                            : 0,
                                        op.stretchMode, op.stretch, op.interpolation, duty,
                                        samplePhase, op.kickAttack, op.gran.trimStart,
                                        op.stretchControls, op.tail);
                                }
                            }
                        }
                        v.modulationSignal[size_t(osc)] +=
                            (signal[0] + signal[1]) * .5f * weight / weightSum;
                        if (u == displayLane && op.data && v.age >= pictureAges[size_t(osc)]) {
                            pictureAges[size_t(osc)] = v.age;
                            auto &view = pictures[size_t(osc)];
                            view = {};
                            view.width = duty;
                            view.fm = fm;
                            view.am = am;
                            view.active = true;
                            view.revision = op.data->revision;
                            double length = std::max(2., double(op.data->samples[0].size()));
                            if (op.engine == 1)
                                view.position =
                                    op.data->framePosition(op.data->frameIndex(position));
                            else if (op.engine == 2) {
                                auto &player = v.playback[size_t(osc)][size_t(displayLane)];
                                view.active = !player.finished;
                                view.position =
                                    float(op.reverse ? op.end - player.cursor / (length - 1)
                                                     : position + player.cursor / (length - 1));
                            } else if (op.engine == 3 && op.gran.enabled) {
                                auto &player = v.granular[size_t(osc)][size_t(displayLane)];
                                view.position = float(player.head / (length - 1));
                                view.active =
                                    !player.finished ||
                                    std::any_of(player.grains.begin(), player.grains.end(),
                                                [](const auto &grain) { return grain.active; });
                                for (const auto &grain : player.grains)
                                    if (grain.active && view.grainCount < 16)
                                        view.grains[size_t(view.grainCount++)] =
                                            float(grain.cursor / (length - 1));
                            } else if (op.engine == 3) {
                                auto &player = v.playback[size_t(osc)][size_t(displayLane)];
                                view.position = float(position + player.elapsed * op.scan);
                                view.position -= std::floor(view.position);
                                for (const auto &grain : player.grains)
                                    if (grain.active && view.grainCount < 16)
                                        view.grains[size_t(view.grainCount++)] =
                                            float(grain.cursor / (length - 1));
                            } else
                                view.active = false;
                        }
                        const float gain = float(amGain) * level * (op.invert ? -1.f : 1.f) *
                                           weight / std::sqrt(norm);
                        traces[size_t(osc)] +=
                            (signal[0] + signal[1]) * .5f * gain * amps[part] * v.velocity;
                        phase += dt;
                        phase -= std::floor(phase);
                        const float pan = unisonPan(spread, op.stereo,
                                                    op.pan + mods.pair(Destination::PanA, part));
                        oscillators[part][0] += signal[0] * gain * std::sqrt((1 - pan) * .5f);
                        oscillators[part][1] += signal[1] * gain * std::sqrt((1 + pan) * .5f);
                    }
                }
                for (auto &x : v.modulationSignal)
                    x = std::clamp(x, -1.f, 1.f);
                for (int part = 0; part < partCount; ++part) {
                    if (part == 2 && !v.cEnabled)
                        continue;
                    const auto &pp = p.part[size_t(part)];
                    const float key = (v.note - 60) * p.keytrack;
                    const float cutoff =
                        effectiveCutoff(pp.cutoff, p.filterCutoff,
                                        mods.pair(Destination::CutoffA, part) * 120, key, sr);
                    v.cutoff[part] += smoothing * (cutoff - v.cutoff[part]);
                    const float res = clamp(pp.resonance + p.filterResonance +
                                                mods.pair(Destination::ResonanceA, part),
                                            0, 1);
                    const float mix = clamp(pp.level + mods.pair(Destination::MixA, part), 0, 2);
                    for (int ch = 0; ch < 2; ++ch) {
                        const int masks[]{1, 2, 3, 0, 4, 5, 6, 7};
                        const int mask = masks[std::clamp(pp.input, 0, 7)];
                        float input = 0;
                        for (int bus = 0; bus < partCount; ++bus)
                            if (mask & (1 << bus))
                                input += oscillators[bus][ch];
                        if (pp.filterType != 3) {
                            input = v.filters[part][ch][0].tick(input, v.cutoff[part], res,
                                                                pp.filterType, sr, pp.drive,
                                                                p.warmDrive);
                            if (pp.slope == 1)
                                input = v.filters[part][ch][1].tick(input, v.cutoff[part], res,
                                                                    pp.filterType, sr, pp.drive,
                                                                    p.warmDrive);
                        }
                        float contribution = input * amps[part] * v.velocity * mix;
                        parts[part] += contribution;
                        if (p.solo == 0 || p.solo == part + 1)
                            out[ch] += contribution;
                    }
                }
            }
            for (int osc = 0; osc < playerCount; ++osc)
                if (oscScopes[size_t(osc)])
                    oscScopes[size_t(osc)][s] = traces[size_t(osc)];
            left[s] = out[0] * engineOutputCalibration;
            right[s] = out[1] * engineOutputCalibration;
            if (partA)
                partA[s] = parts[0] * engineOutputCalibration;
            if (partB)
                partB[s] = parts[1] * engineOutputCalibration;
            if (partC)
                partC[s] = parts[2] * engineOutputCalibration;
            if (distortion)
                distortion[s] = fxDrive;
            if (phaser)
                phaser[s] = fxPhaser;
        }
    }
    const std::array<PlaybackPicture, playerCount> &playbackPictures() const {
        return pictures;
    }
    static float effectiveCutoff(float local, float global, float modulation, float key,
                                 double sampleRate) {
        return clamp(local * std::exp2(clamp(global + modulation + key, -180, 180) / 12), 20,
                     float(sampleRate * .43));
    }
};
} // namespace logisth
