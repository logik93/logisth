#include "Engine.h"
#include "Morph.h"
#include <atomic>
#include <cstdlib>
#include <iostream>
#include <new>
#include <stdexcept>
using namespace logisth;
std::atomic<bool> watchAlloc{false};
std::atomic<int> allocations{0};
void *operator new(std::size_t n) {
    if (watchAlloc)
        ++allocations;
    if (auto *p = std::malloc(n ? n : 1))
        return p;
    throw std::bad_alloc();
}
void operator delete(void *p) noexcept {
    std::free(p);
}
void operator delete(void *p, std::size_t) noexcept {
    std::free(p);
}
void *operator new[](std::size_t n) {
    return ::operator new(n);
}
void operator delete[](void *p) noexcept {
    ::operator delete(p);
}
void operator delete[](void *p, std::size_t) noexcept {
    ::operator delete(p);
}
void check(bool ok, const char *message) {
    if (!ok)
        throw std::runtime_error(message);
}
Params clean() {
    Params p;
    for (auto &o : p.osc) {
        o.level = 0;
        o.retrigger = true;
        o.unison = 1;
        o.semitone = 0;
        o.detune = 0;
        o.stereo = 0;
    }
    p.osc[0].level = .3f;
    for (auto &part : p.part) {
        part.filterType = 3;
        part.level = 1;
        part.amp = {0, 0, 1, 0};
    }
    return p;
}
std::vector<float> render(Params p, int length = 8192) {
    Engine e;
    e.prepare(48000);
    e.noteOn(69, 1, 1, p);
    std::vector<float> l(size_t(length), 0), r(size_t(length), 0);
    e.render(l.data(), r.data(), length, p);
    return l;
}
double difference(const std::vector<float> &a, const std::vector<float> &b) {
    double sum = 0;
    for (size_t i = 0; i < a.size(); ++i)
        sum += std::abs(a[i] - b[i]);
    return sum;
}
double power(const std::vector<float> &v, double hz, int from = 8000, double sr = 48000) {
    double coefficient = 2 * std::cos(2 * pi * hz / sr), a = 0, b = 0;
    for (size_t i = size_t(from); i < v.size(); ++i) {
        double x = v[i] + coefficient * a - b;
        b = a;
        a = x;
    }
    return std::max(0., a * a + b * b - coefficient * a * b) /
           std::pow(double(v.size() - size_t(from)), 2);
}
int main() {
    try {
        for (double width : {.01, .2, .5, .8, .99}) {
            double previous = -1;
            for (int i = 0; i <= 1000; ++i) {
                double t = i / 1000., x = phaseWidth(t, width);
                check(x >= previous && x >= 0 && x <= 1,
                      "phase warp is monotonic with fixed endpoints");
                previous = x;
                if (width == .5)
                    check(std::abs(t - x) < 1e-12, "neutral phase width is identity");
            }
        }
        for (int wave = 0; wave < 12; ++wave) {
            auto p = clean();
            p.osc[0].wave = wave;
            auto before = render(p);
            p.osc[0].pulseWidth = .27f;
            check(difference(before, render(p)) > 1,
                  "Width affects every Classic waveform, including noise gates");
        }
        auto factory = SourceData::factory();
        for (int engine = 1; engine < 4; ++engine)
            for (int model = 0; model < 2; ++model) {
                auto p = clean();
                p.osc[0].data = factory.get();
                p.osc[0].engine = engine;
                p.osc[0].gran.enabled = model > 0;
                auto before = render(p);
                p.osc[0].pulseWidth = .31f;
                check(difference(before, render(p)) > 1,
                      "Width/Warp affects Wavetable, Sampler and both granular models");
            }
        for (int source = 0; source < 4; ++source) {
            int carrier = (source + 1) % 4;
            auto p = clean();
            p.osc[0].level = 0;
            p.osc[size_t(carrier)].level = .3f;
            p.osc[size_t(carrier)].wave = 0;
            p.osc[size_t(carrier)].fmDepth = 1;
            p.osc[size_t(source)].wave = 1;
            p.osc[size_t(source)].semitone = 7;
            auto internal = render(p);
            p.osc[size_t(carrier)].fmSource = source + 1;
            check(difference(internal, render(p)) > 1,
                  "every muted player can modulate another player's FM");
            p.osc[size_t(carrier)].amSource = source + 1;
            p.osc[size_t(carrier)].amDepth = 1;
            for (float x : render(p))
                check(std::isfinite(x) && std::abs(x) < 5, "cross-player FM/AM remains finite");
        }
        {
            auto p = clean();
            p.osc[0].fmDepth = p.osc[1].fmDepth = 8;
            p.osc[0].fmSource = 2;
            p.osc[1].fmSource = 1;
            p.osc[1].level = .3f;
            for (float x : render(p, 48000))
                check(std::isfinite(x) && std::abs(x) < 5,
                      "cyclic modulation uses bounded previous-sample feedback");
        }
        SourceData data;
        data.sampleRate = 48000;
        for (auto &ch : data.samples) {
            ch.resize(48000);
            for (int i = 0; i < 48000; ++i)
                ch[size_t(i)] = .4f * float(std::sin(2 * pi * 220 * i / 48000));
        }
        for (int mode = 4; mode < 8; ++mode) {
            SpectralStream stream(48000);
            SamplerPlayer player;
            player.spectral = &stream;
            player.token = uint64_t(mode);
            std::vector<float> out(80000);
            watchAlloc = true;
            for (int i = 0; i < int(out.size()); ++i)
                out[size_t(i)] = player.tick(data, 2, 48000, 0, 1, false, false, mode, 2, 0)[0];
            watchAlloc = false;
            for (float x : out)
                check(std::isfinite(x) && std::abs(x) < 4, "new stretch engines remain finite");
            double expected = power(out, 440);
            std::cout << "mode " << mode << " 440 Hz power " << expected << " / 220 Hz "
                      << power(out, 220) << '\n';
            check(expected > power(out, 220) * 10 + 1e-6,
                  "new stretch algorithms preserve played pitch independently of duration");
            if (mode == 6)
                for (int i = 0; i < 1200; ++i)
                    check(std::abs(out[size_t(i)] - data.samples[0][size_t(i)]) < 1e-6,
                          "Kick retains the attack sample-exact while pitching its tail");
            int ticks = int(out.size());
            while (!player.finished && ticks++ < 120000)
                player.tick(data, 2, 48000, 0, 1, false, false, mode, 2, 0);
            check(ticks > 90000 && ticks < 100000,
                  "stretch factor gives expected one-shot length with attack protection");
        }
        for (double rate : {44100., 48000., 96000.}) {
            SourceData tone;
            tone.sampleRate = rate;
            for (auto &ch : tone.samples) {
                ch.resize(size_t(rate * 2));
                for (size_t i = 0; i < ch.size(); ++i)
                    ch[i] = .4f * float(std::sin(2 * pi * 220 * i / rate));
            }
            SpectralStream stream(rate);
            SamplerPlayer player;
            player.spectral = &stream;
            std::vector<float> output(size_t(rate), 0.f);
            double energy = 0;
            watchAlloc = true;
            for (size_t i = 0; i < output.size(); ++i)
                output[i] = player.tick(tone, 2, rate, 0, 1, true, false, 7, 1, 0)[0];
            watchAlloc = false;
            for (size_t i = size_t(rate * .3); i < output.size(); ++i)
                energy += output[i] * output[i];
            double rms = std::sqrt(energy / (output.size() - size_t(rate * .3)));
            check(rms > .24 && rms < .32,
                  "spectral neutral time retains source level at 44.1/48/96 kHz");
            double near = 0, wrong = 0;
            for (int f = 438; f <= 442; ++f)
                near += power(output, f, int(rate * .3), rate);
            for (int f = 218; f <= 222; ++f)
                wrong += power(output, f, int(rate * .3), rate);
            check(near > wrong * 100 + 1e-5,
                  "spectral pitch accuracy persists at all supported test sample rates");
        }
        {
            SpectralStream stream(48000);
            SamplerPlayer player;
            for (int i = 0; i < 5000; ++i)
                player.tick(data, 2, 48000, 0, 1, true, false, 7, 1, 0);
            player.spectral = &stream;
            watchAlloc = true;
            float previous = 0;
            for (int i = 0; i < 10000; ++i) {
                auto x = player.tick(data, 2, 48000, 0, 1, true, false, 7, 1, 0)[0];
                check(std::isfinite(x) && std::abs(x) < 2,
                      "background-ready spectral handoff remains finite");
                if (i > 0)
                    check(std::abs(x - previous) < .25f,
                          "late spectral readiness crossfades without a hard step");
                previous = x;
            }
            watchAlloc = false;
        }
        SourceData cropped;
        cropped.sampleRate = 48000;
        for (auto &ch : cropped.samples) {
            ch.resize(4800);
            for (size_t i = 0; i < ch.size(); ++i)
                ch[i] = (i >= 1200 && i < 3600) ? 0.f : 1.f;
        }
        GranularParams trim;
        trim.enabled = true;
        trim.trimStart = .251f;
        trim.attack = 0;
        trim.hold = .005f;
        for (bool reverse : {false, true}) {
            GranulizerPlayback player;
            for (int i = 0; i < 12000; ++i) {
                auto v = player.tick(cropped, trim, 48000, 1, 0, .749f, true, reverse, 60, 60, 120);
                check(v[0] == 0 && v[1] == 0,
                      "granular loops remain inside selected source region");
            }
            SourcePlayback cloud;
            for (int i = 0; i < 12000; ++i) {
                auto v = cloud.tick(cropped, 3, 1, 48000, 0, .749f, true, reverse, .02f, 20, .1f,
                                    .4f, .2f, 0, 1, 0, .5f, 0, 25, .251f);
                check(v[0] == 0 && v[1] == 0, "cloud loops remain inside selected source region");
            }
        }
        check(allocations.load() == 0,
              "prepared spectral and time-domain engines do not allocate during processing");
        std::array<float, parameterCount> values{};
        for (size_t i = 0; i < parameterCount; ++i)
            values[i] = parameterSpecs[i].initial;
        values[size_t(Param::m1target1)] = 1;
        values[size_t(Param::m1low1)] = .2f;
        values[size_t(Param::m1high1)] = .8f;
        values[size_t(Param::macro1)] = .5f;
        auto read = [&](size_t i) { return values[i]; };
        check(std::abs(morphedValue(Param::filtercutoff, read)) < 1e-5,
              "macro interpolates its own normalized From/To range");
        check(morphedValue(Param::master, read) == values[size_t(Param::master)],
              "unassigned parameters retain their base value");
        std::cout
            << "PASS performance DSP: all-engine width, cross-player FM/AM, bounded cycles, four "
               "stretch engines, Kick attack/pitch/duration, no RT allocations and macro ranges\n";
        return 0;
    } catch (const std::exception &e) {
        watchAlloc = false;
        std::cerr << e.what() << '\n';
        return 1;
    }
}
