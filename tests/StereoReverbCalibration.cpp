#include "Effects.h"
#include "Engine.h"
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <stdexcept>
using namespace logisth;
struct Meter {
    double l = 0, r = 0, lr = 0, mid = 0, side = 0, peak = 0;
    size_t count = 0;
    void add(float a, float b) {
        l += double(a) * a;
        r += double(b) * b;
        lr += double(a) * b;
        mid += double(a + b) * (a + b) * .5;
        side += double(a - b) * (a - b) * .5;
        peak = std::max(peak, double(std::max(std::abs(a), std::abs(b))));
        ++count;
    }
    double correlation() const {
        return lr / std::max(1e-30, std::sqrt(l * r));
    }
    double sideMid() const {
        return 10 * std::log10(std::max(1e-24, side) / std::max(1e-24, mid));
    }
    double rms() const {
        return std::sqrt((l + r) / std::max(size_t(1), count * 2));
    }
};
static Meter oscillator(int voices, float stereo, float detune, double rate, int wave = 1) {
    auto e = std::make_unique<Engine>();
    e->prepare(rate);
    Params p;
    for (auto &o : p.osc) {
        o.level = 0;
        o.semitone = 0;
        o.wave = wave;
        o.retrigger = true;
        o.stereo = 0;
        o.detune = detune;
        o.unison = 1;
    }
    for (auto &part : p.part) {
        part.filterType = 3;
        part.amp = {0, 0, 1, 0};
        part.level = 1;
    }
    p.osc[0].level = .1f;
    p.osc[0].stereo = stereo;
    p.osc[0].unison = voices;
    e->noteOn(69, 1, 1, p);
    float l[256], r[256];
    Meter m;
    for (int offset = 0; offset < int(rate * 5); offset += 256) {
        int n = std::min(256, int(rate * 5) - offset);
        e->render(l, r, n, p);
        if (offset > rate)
            for (int i = 0; i < n; ++i)
                m.add(l[i], r[i]);
    }
    return m;
}
int main(int argc, char **argv) {
    try {
        std::filesystem::path out = argc > 1 ? argv[1] : ".";
        std::filesystem::create_directories(out);
        std::ofstream stereo(out / "stereo.csv"), reverb(out / "reverb.csv");
        stereo << std::setprecision(10);
        reverb << std::setprecision(10);
        stereo << "sample_rate,voices,stereo,detune_ct,wave,correlation,side_mid_db,rms,peak\n";
        for (double rate : {44100., 48000., 96000.})
            for (int voices : {1, 2, 4, 8})
                for (float width : {0.f, .5f, 1.f}) {
                    auto m = oscillator(voices, width, 20, rate);
                    stereo << rate << ',' << voices << ',' << width << ",20,Saw," << m.correlation()
                           << ',' << m.sideMid() << ',' << m.rms() << ',' << m.peak << '\n';
                }
        for (int voices : {2, 8})
            for (float detune : {0.f, 100.f}) {
                auto m = oscillator(voices, 1, detune, 48000, 0);
                stereo << "48000," << voices << ",1," << detune << ",Sine," << m.correlation()
                       << ',' << m.sideMid() << ',' << m.rms() << ',' << m.peak << '\n';
            }
        reverb << "sample_rate,type,input_rms,wet_rms,wet_gain_db,wet_peak\n";
        const char *names[]{"Hall", "Room", "Spring", "Vintage", "Shimmer"};
        for (double rate : {44100., 48000., 96000.})
            for (int type = 0; type < 5; ++type) {
                auto fx = std::make_unique<Effects>();
                fx->prepare(rate);
                EffectsParams p;
                p.reverb = true;
                p.reverbType = type;
                p.reverbMix = 1;
                p.reverbDecay = 3.5f;
                p.reverbDamp = .5f;
                p.reverbSize = .65f;
                float l[256]{}, r[256]{}, mod[256]{};
                for (int i = 0; i < 300; ++i)
                    fx->process(l, r, 256, mod, mod, p);
                uint32_t seed = 123456789;
                double pink[7]{};
                Meter input, wet;
                for (int offset = 0; offset < int(rate * 7); offset += 256) {
                    int n = std::min(256, int(rate * 7) - offset);
                    for (int i = 0; i < n; ++i) {
                        seed ^= seed << 13;
                        seed ^= seed >> 17;
                        seed ^= seed << 5;
                        double w = double(seed) / 2147483648. - 1;
                        pink[0] = .99886 * pink[0] + w * .0555179;
                        pink[1] = .99332 * pink[1] + w * .0750759;
                        pink[2] = .969 * pink[2] + w * .153852;
                        pink[3] = .8665 * pink[3] + w * .3104856;
                        pink[4] = .55 * pink[4] + w * .5329522;
                        pink[5] = -.7616 * pink[5] - w * .016898;
                        l[i] = r[i] = float((pink[0] + pink[1] + pink[2] + pink[3] + pink[4] +
                                             pink[5] + pink[6] + w * .5362) *
                                            .011);
                        pink[6] = w * .115926;
                        if (offset > rate * 3)
                            input.add(l[i], r[i]);
                    }
                    fx->process(l, r, n, mod, mod, p);
                    if (offset > rate * 3)
                        for (int i = 0; i < n; ++i) {
                            if (!std::isfinite(l[i]) || !std::isfinite(r[i]))
                                throw std::runtime_error("Non-finite reverb calibration");
                            wet.add(l[i], r[i]);
                        }
                }
                reverb << rate << ',' << names[type] << ',' << input.rms() << ',' << wet.rms()
                       << ',' << 20 * std::log10(wet.rms() / input.rms()) << ',' << wet.peak
                       << '\n';
            }
        std::cout << "Stereo correlation/Side-Mid and full-wet pink-noise measurements exported\n";
    } catch (const std::exception &e) {
        std::cerr << e.what() << '\n';
        return 1;
    }
}
