#include "Effects.h"
#define Effects FrozenEffects0962
#define EffectsParams FrozenEffectsParams0962
#include "fixtures/Effects-0.9.6.2.h"
#undef EffectsParams
#undef Effects
#include <iostream>
#include <memory>
#include <stdexcept>
using namespace logisth;
static void check(bool ok, const char *message) {
    if (!ok)
        throw std::runtime_error(message);
}
class SparseImpulse : public ImpulseProcessor {
    std::vector<std::array<float, 2>> history = std::vector<std::array<float, 2>>(7200);
    size_t position = 0;

  public:
    void process(const float *l, const float *r, float *outL, float *outR, int n,
                 bool) noexcept override {
        for (int i = 0; i < n; ++i) {
            const auto old = history[position];
            history[position] = {l[i], r[i]};
            outL[i] = old[0];
            outR[i] = old[1];
            position = (position + 1) % history.size();
        }
    }
    void reset() noexcept override {
        std::fill(history.begin(), history.end(), std::array<float, 2>{});
        position = 0;
    }
};
int main() {
    try {
        EffectOrder order;
        int permutations = 0;
        do {
            EffectOrder decoded;
            check(order.valid() && EffectOrder::unpack(order.packed(), decoded) && decoded == order,
                  "all permutations round-trip as atomic order values");
            ++permutations;
        } while (std::next_permutation(order.stages.begin(), order.stages.end()));
        check(permutations == 40320, "all eight stages can move independently");
        auto invalid = order;
        invalid.stages[1] = invalid.stages[0];
        check(!invalid.valid() && !EffectOrder::unpack(0, invalid), "duplicate stages rejected");
        check(!EffectOrder::unpack(0x1000000, invalid), "unknown packed bits rejected");
        check(!order.move(-1, 0) && !order.move(0, 8), "move bounds validated");
        for (double rate : {44100., 48000., 96000.}) {
            auto current = std::make_unique<Effects>();
            auto frozen = std::make_unique<FrozenEffects0962>();
            current->prepare(rate);
            frozen->prepare(rate);
            EffectsParams p;
            FrozenEffectsParams0962 old;
            auto configure = [](auto &s) {
                s.distortion = s.phaser = s.chorus = s.eq = s.reverb = s.delay = s.compressor =
                    true;
                s.sync = false;
                s.drive = .2f;
                s.distMix = .3f;
                s.dual = true;
                s.delayL = .011f;
                s.delayR = .017f;
                s.smear = .4f;
                s.threshold = -24;
                s.eqMode = 1;
                s.eqBands[3] = 3;
                s.eqBands[7] = -2;
                s.reverbType = 3;
            };
            configure(p);
            configure(old);
            float l[257], r[257], a[257], b[257], drive[257], phase[257];
            double error = 0;
            for (int block = 0; block < 70; ++block) {
                for (int i = 0; i < 257; ++i) {
                    const auto t = block * 257 + i;
                    l[i] = a[i] = float(std::sin(t * .03) * .3);
                    r[i] = b[i] = float(std::cos(t * .017) * .15);
                    drive[i] = float(std::sin(t * .003) * .02);
                    phase[i] = float(std::cos(t * .001) * .01);
                }
                current->process(l, r, 257, drive, phase, p);
                frozen->process(a, b, 257, drive, phase, old);
                for (int i = 0; i < 257; ++i)
                    error = std::max(
                        {error, double(std::abs(l[i] - a[i])), double(std::abs(r[i] - b[i]))});
            }
            check(error < 2e-5, "default seven-effect audio remains consistent with 0.9.6.2");
        }
        auto render = [](bool clipFirst, bool distortion) {
            auto fx = std::make_unique<Effects>();
            fx->prepare(48000);
            EffectsParams p;
            p.reverb = false;
            p.sync = false;
            p.eq = true;
            p.bass = 12;
            p.distortion = distortion;
            if (clipFirst)
                p.order.move(distortion ? 3 : 7, 0);
            Clipper clipper;
            DynamicsMeters meter;
            ClipperSettings cp{!distortion, false, 0, 0, -1, .5f, 1};
            ClipperStage clip{clipper, cp, meter};
            float l[128], r[128], zero[128]{};
            for (int block = 0; block < 100; ++block) {
                std::fill_n(l, 128, .5f);
                std::fill_n(r, 128, .5f);
                fx->process(l, r, 128, zero, zero, p, nullptr, nullptr, &clip);
            }
            return l[127];
        };
        check(std::abs(render(false, true) - render(true, true)) > .2,
              "moving EQ across nonlinear distortion changes real audio");
        check(render(true, false) > render(false, false) + .5f,
              "Clipper participates in the movable pre-Main effect chain");
        for (bool delayFirst : {false, true}) {
            auto fx = std::make_unique<Effects>();
            fx->prepare(8000);
            SparseImpulse impulse;
            EffectsParams p;
            p.sync = false;
            p.reverb = true;
            p.reverbType = 5;
            p.reverbMix = 1;
            p.impulseSeconds = 1;
            p.convolution = &impulse;
            p.delay = true;
            p.delayL = p.delayR = 1.5f;
            p.delayMix = 1;
            p.feedback = 0;
            p.highCut = 3000;
            if (delayFirst)
                p.order.move(5, 4);
            float l[128]{}, r[128]{}, mod[128]{};
            bool late = false;
            for (int offset = 0; offset < 28000; offset += 128) {
                std::fill_n(l, 128, 0);
                std::fill_n(r, 128, 0);
                if (offset == 0)
                    l[0] = r[0] = 1;
                fx->process(l, r, 128, mod, mod, p);
                if (offset >= 18800 && offset < 20000)
                    for (float x : l)
                        late |= std::abs(x) > 1e-5;
            }
            check(
                late,
                "idle horizon preserves the SUM of serial IR gap and first delay in either order");
        }
        RackTransition ramp;
        ramp.reset(48000, EffectOrder{});
        auto moved = EffectOrder{};
        moved.move(7, 0);
        ramp.request(moved);
        float previous = 1;
        bool switched = false;
        for (int i = 0; i < 480; ++i) {
            check(ramp.blockLimit(512) > 0 && ramp.blockLimit(512) <= 240,
                  "bounded transition chunks");
            const auto before = ramp.order();
            const float value = ramp.tick();
            check(std::abs(value - previous) <= 1.f / 240 + 1e-6,
                  "live reorder has bounded gain steps");
            if (ramp.order() != before) {
                check(value == 0, "order switches at zero gain");
                switched = true;
            }
            previous = value;
        }
        check(switched && previous == 1 && ramp.order() == moved && ramp.blockLimit(512) == 512,
              "live order settles after ten milliseconds");
        std::cout << "PASS rack: all 40320 orders, default DSP parity, actual reordered audio, "
                     "movable clipper and causal transition\n";
    } catch (const std::exception &error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
