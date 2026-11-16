#include "Dynamics.h"
#include "Effects.h"
#include "Engine.h"
#include <iostream>
#include <stdexcept>
using namespace logisth;
void check(bool ok, const char *m) {
    if (!ok)
        throw std::runtime_error(m);
}
double aliasRatio(bool aa) {
    Clipper c;
    DynamicsMeters m;
    ClipperSettings p{true, aa, 0, 0, 0, .5, 1};
    double fr = 0, fi = 0, ar = 0, ai = 0;
    const int size = 8192, k = 997, alias = size - 5 * k;
    float l[512], r[512];
    for (int block = 0; block < 32; ++block) {
        for (int i = 0; i < 512; ++i)
            l[i] = r[i] = 4 * std::sin(2 * 3.141592653589793 * k * (block * 512 + i) / size);
        c.process(l, r, 512, p, m);
        if (block >= 16)
            for (int i = 0; i < 512; ++i) {
                double at = block * 512 + i;
                fr += l[i] * std::cos(2 * 3.141592653589793 * k * at / size);
                fi += l[i] * std::sin(2 * 3.141592653589793 * k * at / size);
                ar += l[i] * std::cos(2 * 3.141592653589793 * alias * at / size);
                ai += l[i] * std::sin(2 * 3.141592653589793 * alias * at / size);
            }
    }
    return std::hypot(ar, ai) / std::hypot(fr, fi);
}

int main() {
    try {
        check(aliasRatio(true) < aliasRatio(false) * .8,
              "ADAA reduces folded fifth-harmonic alias relative to fundamental");
        for (float ratio : {1.f, 2.f, 4.f, 100.f})
            for (float knee : {0.f, 6.f, 24.f}) {
                float last = -200;
                for (int i = -800; i <= 240; ++i) {
                    float db = i * .1f, red = compressionReduction(db, -18, ratio, knee),
                          out = db - red;
                    check(red >= 0 && out >= last - 1e-5, "compressor monotonic static knee");
                    last = out;
                    if (db < -18 - knee * .5f)
                        check(red == 0, "below knee is uncompressed");
                }
                check(std::abs(compressionReduction(0, -18, ratio, knee) - 18 * (1 - 1 / ratio)) <
                          1e-5,
                      "above knee follows ratio");
            }
        for (double sr : {44100., 48000., 96000.}) {
            Compressor comp;
            comp.configure({-18, 4, 6, 10, 100, 0, 1}, sr);
            float l = 0, r = 0;
            for (int i = 0; i < int(sr); ++i) {
                l = .5f;
                r = .1f;
                comp.tick(l, r);
            }
            float expected = amplitude(-compressionReduction(decibels(.5f), -18, 4, 6));
            check(std::abs(l - .5f * expected) < 2e-4 && std::abs(l / r - 5) < 1e-5,
                  "compressor settles at correct stereo-linked gain");
            const float held = comp.maximumReduction;
            comp.configure({-18, 4, 6, 10, 100, 0, 1}, sr);
            for (int i = 0; i < int(sr); ++i) {
                l = r = 0;
                comp.tick(l, r);
            }
            comp.configure({-18, 4, 6, 10, 100, 0, 1}, sr);
            l = r = .001f;
            comp.tick(l, r);
            check(comp.maximumReduction < .01f && held > 5,
                  "compressor releases to unity on silence");
            for (int mode : {0, 1})
                for (bool aa : {false, true}) {
                    Clipper clip;
                    DynamicsMeters meter;
                    ClipperSettings p{true, aa, mode, 12, -3, .5f, 1};
                    float a[512], b[512];
                    for (int k = 0; k < 20; ++k) {
                        for (int i = 0; i < 512; ++i) {
                            a[i] = std::sin(float(k * 512 + i) * .071f) * 2;
                            b[i] = a[i] * .3f;
                        }
                        clip.process(a, b, 512, p, meter);
                        for (int i = 0; i < 512; ++i)
                            check(std::isfinite(a[i]) && std::abs(a[i]) <= amplitude(-3) + 2e-6,
                                  "clipper ceiling both curves and quality modes");
                    }
                }
            OutputLimiter limiter;
            DynamicsMeters m;
            LimiterSettings p{true, -1, 100, 0};
            for (int block = 0; block < 30; ++block) {
                float a[512], b[512];
                for (int i = 0; i < 512; ++i) {
                    a[i] = float(std::sin((block * 512 + i) * .3) * 20);
                    b[i] = a[i] * .3f;
                }
                if (block == 10)
                    p.ceiling = -12;
                limiter.process(a, b, 512, p, sr, m);
                for (int i = 0; i < 512; ++i) {
                    check(std::abs(a[i]) <= amplitude(p.ceiling) &&
                              std::abs(b[i]) <= amplitude(p.ceiling),
                          "limiter exact ceiling including automation");
                    if (std::abs(a[i]) > 1e-5)
                        check(std::abs(b[i] / a[i] - .3f) < 1e-5, "limiter preserves stereo image");
                }
            }
            float a[512]{}, b[512]{};
            p.enabled = false;
            a[0] = .4f;
            limiter.process(a, b, 512, p, sr, m);
            check(a[0] == .4f, "limiter bypass exact");
            auto fx = std::make_unique<Effects>();
            fx->prepare(sr);
            EffectsParams settings;
            settings.reverb = false;
            a[0] = .3f;
            fx->process(a, b, 512, b, b, settings);
            check(fx->processedSamples == 0 && a[0] == .3f, "all-off FX skip processing exactly");
            settings.reverb = true;
            std::fill_n(a, 512, 0);
            fx->process(a, b, 512, b, b, settings);
            check(fx->processedSamples == 0, "empty enabled reverb sleeps");
            a[0] = 1;
            fx->process(a, b, 512, b, b, settings);
            check(fx->processedSamples == 512, "signal wakes sleeping reverb");
        }
        {
            auto fx = std::make_unique<Effects>();
            fx->prepare(8000);
            EffectsParams p;
            p.reverb = false;
            p.delay = true;
            p.sync = false;
            p.delayL = p.delayR = 1;
            p.delayMix = 1;
            p.feedback = 0;
            p.highCut = 3000;
            float l[128]{}, r[128]{}, mod[128]{};
            l[0] = r[0] = 1;
            fx->process(l, r, 128, mod, mod, p);
            bool late = false;
            for (int at = 128; at < 24000; at += 128) {
                std::fill_n(l, 128, 0);
                std::fill_n(r, 128, 0);
                fx->process(l, r, 128, mod, mod, p);
                if (at >= 7936 && at < 8704)
                    for (float x : l)
                        late |= std::abs(x) > 1e-5;
            }
            check(late, "idle gating preserves a delayed first echo across a silent gap");
            check(fx->processedSamples == 0,
                  "silent effects sleep after their tails and delay horizon");
        }
        {
            auto engine = std::make_unique<Engine>();
            engine->prepare(48000);
            Params p;
            for (auto &o : p.osc) {
                o.level = 0;
                o.unison = 8;
            }
            for (auto &part : p.part) {
                part.amp = {0, 0, 1, .001f};
                part.filterType = 3;
                part.level = 1;
            }
            p.osc[0].level = .2f;
            p.osc[0].unison = 1;
            p.osc[0].fmSource = 2;
            p.osc[2].fmSource = 4;
            p.osc[2].fmDepth = 8;
            engine->noteOn(69, 1, 1, p);
            float l[256], r[256];
            engine->render(l, r, 256, p);
            check(engine->oscillatorSamples == 256,
                  "zero-depth FM and unrelated muted carriers require no extra oscillators");
            p.osc[0].fmDepth = 1;
            engine->render(l, r, 256, p);
            check(engine->oscillatorSamples == 256 * 9,
                  "active external FM retains its muted eight-voice source");
            p.osc[1].amSource = 3;
            p.osc[1].amDepth = .5f;
            engine->render(l, r, 256, p);
            check(engine->oscillatorSamples == 256 * 25,
                  "transitive FM/AM dependencies remain active");
        }
        std::cout << "PASS dynamics: knee/ratio, stereo-linked attack/release, clip curves/ADAA, "
                     "exact sample-peak ceiling, automation, bypass and idle FX\n";
    } catch (const std::exception &e) {
        std::cerr << e.what() << '\n';
        return 1;
    }
}
