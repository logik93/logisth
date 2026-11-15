#include "Engine.h"
#include <iostream>
#include <stdexcept>
using namespace logisth;
static void require(bool x, const char *m) {
    if (!x)
        throw std::runtime_error(m);
}
static std::vector<float> render(Params p, int solo = 0) {
    auto e = std::make_unique<Engine>();
    e->prepare(48000);
    p.solo = solo;
    e->noteOn(69, 1, 1, p);
    std::vector<float> out(8192), r(8192);
    e->render(out.data(), r.data(), 8192, p);
    return out;
}
static double energy(const std::vector<float> &x) {
    double v = 0;
    for (float a : x) {
        require(std::isfinite(a), "part C stays finite");
        v += a * a;
    }
    return v / x.size();
}
int main() {
    try {
        Params p;
        for (auto &o : p.osc) {
            o.level = 0;
            o.unison = 1;
            o.retrigger = true;
            o.stereo = 0;
            o.wave = 0;
            o.semitone = 0;
            o.detune = 0;
            o.data = SourceData::factory().get();
        }
        for (auto &part : p.part) {
            part.amp = {0, 0, 1, 0};
            part.filterType = 3;
            part.level = 1;
        }
        p.osc[4].level = .5;
        require(energy(render(p, 3)) > .08, "C1 is independently audible in solo C");
        require(energy(render(p, 1)) < 1e-18, "solo A excludes C output");
        auto a = p;
        a.osc[4].level = 0;
        a.osc[0].level = .5;
        auto ya = render(a);
        auto yc = render(p);
        double error = 0;
        for (size_t i = 0; i < ya.size(); ++i)
            error = std::max(error, double(std::abs(ya[i] - yc[i])));
        require(error < 1e-6, "C uses the same gain and envelope path as A");
        p.osc[4].level = 0;
        p.osc[5].level = .5;
        require(energy(render(p, 3)) > .08, "C2 is independently audible");
        p.osc[5].level = 0;
        p.osc[0].level = .1;
        p.osc[2].level = .2;
        p.osc[4].level = .3;
        const int masks[]{1, 2, 3, 0, 4, 5, 6, 7};
        for (int input = 0; input < 8; ++input) {
            auto q = p;
            q.part[2].input = input;
            auto y = render(q, 3);
            float level = 0;
            for (int bus = 0; bus < 3; ++bus)
                if (masks[input] & (1 << bus))
                    level += float(bus + 1) * .1f;
            require(std::abs(std::sqrt(energy(y)) - level / std::sqrt(2.)) < .004,
                    "C filter accepts every A/B/C input combination");
        }
        auto q = p;
        q.part[2].input = 4;
        q.part[2].filterType = 0;
        q.part[2].cutoff = 8000;
        double open = energy(render(q, 3));
        q.filterCutoff = -72;
        auto closed = render(q, 3);
        require(energy(std::vector<float>(closed.begin() + 4096, closed.end())) < open * .02,
                "shared filter control also closes filter C");
        ModValues mod;
        mod.values[size_t(Destination::CutoffAB)] = .3f;
        mod.values[size_t(Destination::CutoffC)] = .7f;
        mod.values[size_t(Destination::CutoffABC)] = .1f;
        require(std::abs(mod.pair(Destination::CutoffA, 0) - .4) < 1e-6 &&
                    std::abs(mod.pair(Destination::CutoffA, 2) - .8) < 1e-6,
                "A+B targets retain their meaning and A+B+C includes C");
        mod = {};
        mod.values[size_t(Destination::PositionC1)] = .2f;
        mod.values[size_t(Destination::PositionC2)] = .4f;
        require(mod.position(4) == .2f && mod.position(5) == .4f && mod.position(0) == 0,
                "C wavetable positions route independently");
        for (int mode = 0; mode < 4; ++mode) {
            auto z = p;
            z.osc[0].level = z.osc[2].level = 0;
            z.osc[4].engine = mode;
            z.osc[4].level = .4f;
            require(energy(render(z, 3)) > 1e-7,
                    "C supports Classic, Wavetable, Sampler and Granular");
        }
        std::cout << "PASS part C: C1/C2, gain parity, solo, eight filter inputs, shared cutoff, "
                     "new routing and four engines\n";
    } catch (const std::exception &e) {
        std::cerr << e.what() << '\n';
        return 1;
    }
}
