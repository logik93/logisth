#include "Equalizer.h"
#include <iostream>
#include <stdexcept>
using namespace logisth;
static void check(bool ok, const char *s) {
    if (!ok)
        throw std::runtime_error(s);
}
static double measured(EqualizerParams p, double hz, double rate) {
    Equalizer eq;
    eq.prepare(rate);
    eq.configure(p, 0, 200, 0, 6000);
    double a = 0, b = 0;
    for (int i = 0; i < 24000; ++i) {
        float x = float(.1 * std::sin(2 * pi * hz * i / rate));
        auto y = eq.tick(x, x);
        check(std::isfinite(y[0]) && y[0] == y[1], "EQ preserves stereo stability");
        if (i > 12000) {
            a += x * x;
            b += y[0] * y[0];
        }
    }
    return 10 * std::log10(std::max(1e-30, b / a));
}
int main() {
    try {
        EqualizerParams p;
        p.eqMode = 1;
        p.eqEnabled.fill(false);
        p.eqEnabled[0] = true;
        p.eqFrequencies[0] = 1000;
        p.eqQ[0] = .70710678f;
        p.eqBands[0] = 6;
        for (double rate : {44100., 48000., 96000.})
            for (int type = 0; type < 8; ++type) {
                p.eqTypes[0] = type;
                for (double hz : {100., 500., 1000., 5000.}) {
                    double model = Equalizer::response(p, 0, 200, 0, 6000, hz, rate),
                           actual = measured(p, hz, rate);
                    check(type == 5 && hz == 1000 ? actual < -75 : std::abs(model - actual) < .14,
                          "each filter shape agrees with its measured audio response at all rates");
                }
            }
        p.eqTypes[0] = 3;
        check(Equalizer::response(p, 0, 200, 0, 6000, 50, 48000) < -50,
              "high-pass removes low frequencies");
        p.eqTypes[0] = 4;
        check(Equalizer::response(p, 0, 200, 0, 6000, 15000, 48000) < -45,
              "low-pass removes high frequencies");
        p.eqTypes[0] = 7;
        check(std::abs(Equalizer::response(p, 0, 200, 0, 6000, 3000, 48000)) < 1e-8,
              "all-pass retains magnitude");
        for (double rate : {44100., 48000., 96000.})
            for (int type : {3, 4})
                for (int slope = 0; slope < eqMaxCutStages; ++slope) {
                    p.eqTypes[0] = type;
                    p.eqSlopes[0] = slope;
                    const double near = type == 3 ? 500. : 2000., far = type == 3 ? 250. : 4000.;
                    const double attenuation = measured(p, near, rate) - measured(p, far, rate);
                    auto graph = [&](double hz) {
                        return Equalizer::response(p, 0, 200, 0, 6000, hz, rate);
                    };
                    check(std::abs(attenuation - 12 * (slope + 1)) < 1.8 &&
                              std::abs(measured(p, far, rate) - graph(far)) < .35,
                          "cut slopes match the requested order in audio and the response plot");
                    check(std::abs(graph(1000) + 3.0103) < .001,
                          "Butterworth Q preserves the cutoff reference at every order");
                }
        p.eqTypes[0] = 0;
        check(std::abs(measured(p, 1000, 48000) - 6) < .01,
              "slope does not affect a bell band");
        for (int band = 0; band < eqMaxBands; ++band) {
            p = {};
            p.eqMode = 1;
            p.eqEnabled.fill(false);
            p.eqEnabled[size_t(band)] = true;
            p.eqFrequencies[size_t(band)] = 1000;
            p.eqBands[size_t(band)] = 6;
            check(std::abs(Equalizer::response(p, 0, 200, 0, 6000, 1000, 48000) - 6) < 1e-4,
                  "all sixteen independent bands are usable");
            p.eqEnabled[size_t(band)] = false;
            check(std::abs(measured(p, 1000, 48000)) < 1e-5, "disabled bands are transparent");
        }
        Equalizer eq;
        eq.prepare(48000);
        p = {};
        p.eqMode = 1;
        for (int block = 0; block < 120; ++block) {
            for (int band = 0; band < eqMaxBands; ++band) {
                auto n = size_t(band);
                p.eqTypes[n] = (block + band) % 8;
                p.eqSlopes[n] = (block / 3 + band) % eqMaxCutStages;
                p.eqFrequencies[n] = float(40 * std::pow(1.45, band));
                p.eqQ[n] = .5f + float((block + band) % 6) * .3f;
                p.eqEnabled[n] = (block + band) % 3 != 0;
                p.eqBands[n] = float(block % 7) - 3;
            }
            eq.configure(p, 0, 200, 0, 6000);
            for (int i = 0; i < 256; ++i) {
                auto y = eq.tick(.01f * std::sin(float(block * 256 + i) * .02f), .005f);
                check(std::isfinite(y[0]) && std::abs(y[0]) < 100,
                      "live filter type/slope/frequency/enabled changes stay stable");
            }
        }
        std::cout << "PASS flexible EQ: eight filter shapes, 16 independent bands, model/audio "
                     "parity at three rates, bypass and live transitions\n";
    } catch (const std::exception &e) {
        std::cerr << e.what() << '\n';
        return 1;
    }
}
