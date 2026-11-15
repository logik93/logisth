#include "Engine.h"
#include <iostream>
#include <stdexcept>
using namespace logisth;
static void check(bool ok, const char *why) {
    if (!ok)
        throw std::runtime_error(why);
}
struct Result {
    double ll = 0, rr = 0, lr = 0, mid = 0, side = 0;
};
static Result render(int slot, int voices, float width, float pan = 0, float detune = 20) {
    auto e = std::make_unique<Engine>();
    e->prepare(48000);
    Params p;
    p.polyphony = 1;
    for (auto &o : p.osc) {
        o.level = 0;
        o.wave = 1;
        o.detune = detune;
        o.retrigger = true;
        o.semitone = 0;
        o.stereo = 0;
        o.pan = 0;
    }
    for (auto &part : p.part) {
        part.amp = {0, 0, 1, 0};
        part.filterType = 3;
        part.level = 1;
    }
    auto &o = p.osc[size_t(slot)];
    o.level = .1f;
    o.unison = voices;
    o.stereo = width;
    o.pan = pan;
    e->noteOn(69, 1, 1, p);
    float l[256], r[256];
    Result m;
    for (int offset = 0; offset < 48000 * 5; offset += 256) {
        int n = std::min(256, 48000 * 5 - offset);
        e->render(l, r, n, p);
        if (offset > 48000)
            for (int i = 0; i < n; ++i) {
                check(std::isfinite(l[i]) && std::isfinite(r[i]), "stereo output remains finite");
                m.ll += double(l[i]) * l[i];
                m.rr += double(r[i]) * r[i];
                m.lr += double(l[i]) * r[i];
                m.mid += double(l[i] + r[i]) * (l[i] + r[i]) * .5;
                m.side += double(l[i] - r[i]) * (l[i] - r[i]) * .5;
            }
    }
    return m;
}
int main() {
    try {
        for (int slot = 0; slot < playerCount; ++slot)
            for (int voices : {2, 4, 8}) {
                auto mono = render(slot, voices, 0), half = render(slot, voices, .5f),
                     full = render(slot, voices, 1);
                check(mono.side == 0, "zero width is exactly mono for every classic player");
                check(std::abs(full.lr / std::sqrt(full.ll * full.rr)) < .04,
                      "full width separates unison voice groups with near-zero correlation");
                check(std::abs(10 * std::log10(full.side / full.mid)) < .4,
                      "100 percent stereo has equal Mid/Side power");
                check(std::abs(10 * std::log10(half.side / half.mid) + 6.0205999) < .4,
                      "50 percent stereo is half the Side/Mid amplitude");
                check(std::abs(10 * std::log10((full.ll + full.rr) / (mono.ll + mono.rr))) < .3,
                      "width does not hide a large gain change");
            }
        for (int slot = 0; slot < playerCount; ++slot) {
            auto single = render(slot, 1, 1);
            check(single.side == 0, "single voices are not artificially phase widened");
            auto identical = render(slot, 8, 1, 0, 0);
            check(identical.side < identical.mid * 1e-12,
                  "identical retriggered voices remain mono without phase inversion");
            for (float pan : {-1.f, 1.f}) {
                auto m = render(slot, 8, 1, pan);
                check(pan < 0 ? m.rr == 0 : m.ll == 0,
                      "full pan places the complete wide oscillator on one side");
            }
        }
        std::cout << "PASS stereo: all six players, mono/half/full width, correlation, Mid/Side, "
                     "gain, full-pan and coherent-phase cases\n";
    } catch (const std::exception &e) {
        std::cerr << e.what() << '\n';
        return 1;
    }
}
