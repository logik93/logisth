#include "Effects.h"
#include "Engine.h"
#include <chrono>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <vector>
using namespace logisth;
using Clock = std::chrono::steady_clock;
#if defined(_MSC_VER)
#define NOINLINE __declspec(noinline)
#else
#define NOINLINE __attribute__((noinline))
#endif
NOINLINE void fxBlock(Effects &fx, float *l, float *r, EffectsParams &p) {
    float mod[512]{};
    fx.process(l, r, 512, mod, mod, p);
}
int main(int argc, char **argv) {
    std::ofstream file(argc > 1 ? argv[1] : "cpu.csv");
    file << "fixture,median_us_per_512,checksum\n";
    for (int fixture = 0; fixture < 6; ++fixture) {
        auto fx = std::make_unique<Effects>();
        fx->prepare(48000);
        EffectsParams settings;
        settings.reverb = fixture == 1;
        auto e = std::make_unique<Engine>();
        e->prepare(48000);
        Params p;
        for (auto &o : p.osc) {
            o.level = 0;
            o.unison = 8;
            o.wave = 4;
        }
        for (auto &part : p.part) {
            part.filterType = 3;
            part.amp = {0, 0, 1, .001f};
            part.level = 1;
        }
        p.polyphony = 1;
        p.osc[0].level = .2f;
        p.osc[0].unison = 1;
        p.osc[0].wave = 1;
        p.osc[0].fmSource = 2;
        if (fixture == 3)
            p.osc[0].fmDepth = 1;
        float l[512]{}, r[512]{};
        if (fixture == 4)
            p.polyphony = 16;
        if (fixture == 5) {
            p.sync = false;
            p.lfo[0].free = true;
            p.lfo[0].rate = 20;
            p.lfo[0].wave = 5;
            p.lfo[0].routes[0] = {Destination::PanA, 4};
            p.lfo[1].rate = 13;
            for (int i = 0; i < 100; ++i)
                e->render(l, r, 512, p);
        }
        if (fixture != 4)
            e->noteOn(69, 1, 1, p);
        double sum = 0;
        std::vector<double> timings;
        for (int round = 0; round < 7; ++round) {
            auto start = Clock::now();
            for (int block = 0; block < 128; ++block) {
                if (fixture < 2)
                    fxBlock(*fx, l, r, settings);
                else
                    e->render(l, r, 512, p);
                sum += l[block % 512] + r[(block + 1) % 512];
            }
            timings.push_back(
                std::chrono::duration<double, std::micro>(Clock::now() - start).count() / 128);
        }
        std::sort(timings.begin(), timings.end());
        const char *names[] = {
            "all_fx_bypassed",        "silent_enabled_reverb", "inactive_zero_depth_fm_source",
            "active_muted_fm_source", "silent_engine",         "idle_then_free_lfo"};
        file << names[fixture] << ',' << timings[timings.size() / 2] << ',' << std::setprecision(17)
             << sum << '\n';
    }
    std::cout << "CPU fixtures exported\n";
}
