#include "Engine.h"
#include <iostream>
#include <stdexcept>
using namespace logisth;
static void require(bool ok, const char *m) {
    if (!ok)
        throw std::runtime_error(m);
}
static Params clean(int slot = 0) {
    Params p;
    for (auto &o : p.osc) {
        o.level = 0;
        o.semitone = 0;
        o.wave = 0;
        o.detune = 400;
        o.stereo = 0;
        o.retrigger = true;
        o.unison = 1;
        o.data = SourceData::factory().get();
        o.root = 69;
    }
    for (auto &part : p.part) {
        part.filterType = 3;
        part.amp = {0, 0, 1, 0};
        part.level = 1;
    }
    p.osc[size_t(slot)].level = .3f;
    return p;
}
static std::vector<float> render(Params p, float velocity = 1) {
    auto e = std::make_unique<Engine>();
    e->prepare(48000);
    e->noteOn(69, velocity, 1, p);
    std::vector<float> l(24000), r(l.size());
    e->render(l.data(), r.data(), int(l.size()), p);
    for (float x : l)
        require(std::isfinite(x), "unison stays finite");
    return l;
}
static double power(const std::vector<float> &a, double hz) {
    double re = 0, im = 0;
    for (size_t i = 4096; i < a.size(); ++i) {
        double angle = 2 * pi * hz * double(i) / 48000;
        re += a[i] * std::cos(angle);
        im += a[i] * std::sin(angle);
    }
    return re * re + im * im;
}
int main() {
    try {
        for (int slot = 0; slot < playerCount; ++slot) {
            auto p = clean(slot);
            auto reference = render(p);
            for (int n : {2, 3, 4, 5, 6, 7, 8}) {
                p.osc[size_t(slot)].unison = n;
                p.osc[size_t(slot)].blend = 0;
                auto dry = render(p);
                for (size_t i = 0; i < dry.size(); ++i)
                    require(std::abs(reference[i] - dry[i]) < 1e-6,
                            "zero blend retains one correctly tuned center at every unison count");
                p.osc[size_t(slot)].blend = 1;
                auto wet = render(p);
                require(power(wet, 440) < power(reference, 440) * .2 || n % 2,
                        "even unison full blend preserves wide voices");
            }
            p.osc[size_t(slot)].unison = 2;
            p.osc[size_t(slot)].blend = 1;
            auto wide = render(p);
            for (double hz : {440 * std::exp2(-4. / 12), 440 * std::exp2(4. / 12)})
                require(power(wide, hz) > power(wide, 440) * 100,
                        "400-cent outer voices reach their musical frequencies");
        }
        for (int mode = 0; mode < 4; ++mode) {
            auto p = clean();
            p.osc[0].engine = mode;
            auto single = render(p);
            p.osc[0].unison = 2;
            p.osc[0].blend = 0;
            auto dry = render(p);
            double error = 0;
            for (size_t i = 0; i < dry.size(); ++i)
                error = std::max(error, double(std::abs(dry[i] - single[i])));
            require(error < 1e-6, "blend center works in every player engine");
        }
        for (auto first : {Destination::VolumeA, Destination::PitchA, Destination::PhaseA,
                           Destination::PanA, Destination::CutoffA, Destination::ResonanceA,
                           Destination::MixA, Destination::PositionA, Destination::GrainSizeA}) {
            int family = first == Destination::VolumeA      ? 0
                         : first == Destination::PitchA     ? 1
                         : first == Destination::PhaseA     ? 2
                         : first == Destination::PanA       ? 3
                         : first == Destination::CutoffA    ? 4
                         : first == Destination::ResonanceA ? 5
                         : first == Destination::MixA       ? 6
                         : first == Destination::PositionA  ? 7
                                                            : 8;
            ModValues m;
            m.values[size_t(Destination::VolumeAC) + size_t(family) * 2] = .2f;
            m.values[size_t(Destination::VolumeBC) + size_t(family) * 2] = .4f;
            require(std::abs(m.pair(first, 0) - .2f) < 1e-6 &&
                        std::abs(m.pair(first, 1) - .4f) < 1e-6 &&
                        std::abs(m.pair(first, 2) - .6f) < 1e-6,
                    "A+C and B+C routes apply symmetrically to all shared destinations");
        }
        for (auto first : {Destination::FmA1, Destination::AmA1, Destination::WidthA1}) {
            ModValues m;
            auto pair = first == Destination::FmA1   ? Destination::FmAC
                        : first == Destination::AmA1 ? Destination::AmAC
                                                     : Destination::WidthAC;
            m.values[size_t(pair)] = .2f;
            m.values[size_t(pair) + 1] = .4f;
            for (int slot = 0; slot < playerCount; ++slot)
                require(std::abs(m.osc(first, slot) - (slot < 2   ? .2f
                                                       : slot < 4 ? .4f
                                                                  : .6f)) < 1e-6,
                        "paired FM/AM/PWM scopes reach every appropriate player");
        }
        for (auto target : {Destination::VolumeAC, Destination::VolumeBC}) {
            auto p = clean();
            p.solo = 3;
            p.misc[0].source = Source::Velocity;
            p.misc[0].routes[0] = {target, 2};
            require(power(render(p), 440) > 1,
                    "paired volume modulation wakes an initially silent C part");
        }
        auto p = clean();
        p.fixedVelocity = true;
        require(render(p, .1f) == render(p, 1), "fixed velocity ignores input dynamics exactly");
        p.fixedVelocity = false;
        require(power(render(p, .1f), 440) < power(render(p, 1), 440) * .02,
                "normal velocity remains the default");
        std::cout << "PASS unison: six 400 ct players, dry center at 2-8 voices, four engines, "
                     "symmetric paired C routes and fixed velocity\n";
    } catch (const std::exception &e) {
        std::cerr << e.what() << '\n';
        return 1;
    }
}
