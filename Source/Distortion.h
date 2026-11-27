#pragma once
#include "EffectOrder.h"
#include "Modulation.h"
namespace logisth {
struct DistortionOrder {
    std::array<unsigned, 4> slots{0, 1, 2, 3};
    bool valid() const noexcept {
        unsigned seen = 0;
        for (auto slot : slots) {
            if (slot > 3 || (seen & (1u << slot)))
                return false;
            seen |= 1u << slot;
        }
        return true;
    }
    unsigned packed() const noexcept {
        unsigned result = 0;
        for (unsigned i = 0; i < 4; ++i)
            result |= slots[i] << (i * 2);
        return result;
    }
    static bool unpack(unsigned bits, DistortionOrder &result) noexcept {
        if (bits > 255)
            return false;
        DistortionOrder next;
        unsigned seen = 0;
        for (unsigned i = 0; i < 4; ++i) {
            next.slots[i] = (bits >> (i * 2)) & 3;
            if (seen & (1u << next.slots[i]))
                return false;
            seen |= 1u << next.slots[i];
        }
        result = next;
        return true;
    }
    bool move(int from, int to) noexcept {
        if (from < 0 || from >= 4 || to < 0 || to >= 4 || from == to)
            return false;
        auto value = slots[size_t(from)];
        if (from < to)
            std::move(slots.begin() + from + 1, slots.begin() + to + 1, slots.begin() + from);
        else
            std::move_backward(slots.begin() + to, slots.begin() + from, slots.begin() + from + 1);
        slots[size_t(to)] = value;
        return true;
    }
    EffectOrder expanded() const noexcept {
        EffectOrder result;
        if (!valid())
            return result;
        for (size_t i = 0; i < 4; ++i)
            result.stages[i] = EffectStage(slots[i]);
        result.stages[4] = EffectStage::Flanger;
        return result;
    }
};
struct DistortionSettings {
    bool enabled = false;
    int type = 0;
    float drive = 0, mix = 1;
};
class DistortionRack {
    struct Stage {
        float drive = 0, mix = 1, countdown = 0;
        std::array<float, 2> hold{};
        void process(float *l, float *r, int n, DistortionSettings p, const float *mod,
                     float smooth) noexcept {
            for (int i = 0; i < n; ++i) {
                drive += smooth * (clamp(p.drive + (mod ? mod[i] : 0), 0, 1) - drive);
                mix += smooth * (p.mix - mix);
                const float gain = 1 + drive * 30;
                const bool capture = countdown <= 0;
                if (capture)
                    countdown = float(1 + int(drive * 63));
                --countdown;
                for (int ch = 0; ch < 2; ++ch) {
                    auto &sample = ch == 0 ? l[i] : r[i];
                    const auto x = sample * gain;
                    float wet = x;
                    switch (p.type) {
                    case 0:
                        wet = std::tanh(x);
                        break;
                    case 1:
                        wet = std::fmod(x + 1, 4.f);
                        if (wet < 0)
                            wet += 4;
                        wet = 1 - std::abs(wet - 2);
                        break;
                    case 2:
                        wet = clamp(x, -1, 1);
                        break;
                    case 3:
                        if (capture)
                            hold[size_t(ch)] = sample;
                        wet = hold[size_t(ch)];
                        break;
                    case 4: {
                        const auto levels = std::pow(2.f, 16 - 14 * drive);
                        wet = std::round(sample * levels) / levels;
                        break;
                    }
                    default:
                        break;
                    }
                    sample += (wet - sample) * mix;
                }
            }
        }
    };
    std::array<Stage, 4> stages;
    RackTransition transition;
    double rate = 44100;
    bool initial = true;

  public:
    void prepare(double sampleRate) noexcept {
        rate = sampleRate;
        reset();
    }
    void reset() noexcept {
        stages = {};
        initial = true;
        transition.reset(rate, {});
    }
    // Reuse the original stage kernel for an isolated crossover band.
    void processIsolatedStage(float *l, float *r, int n, DistortionSettings settings,
                              const float *driveMod, float smooth) noexcept {
        stages[0].process(l, r, n, settings, driveMod, smooth);
    }
    void process(float *l, float *r, int n, const std::array<DistortionSettings, 4> &settings,
                 DistortionOrder order, const float *driveMod, float smooth) noexcept {
        const auto expanded = order.expanded();
        if (initial) {
            transition.reset(rate, expanded);
            initial = false;
        }
        transition.request(expanded);
        for (int at = 0; at < n;) {
            const int size = transition.blockLimit(n - at);
            const auto current = transition.order();
            for (int position = 0; position < 4; ++position) {
                const auto slot = size_t(current.stages[size_t(position)]);
                if (settings[slot].enabled && settings[slot].mix > 0)
                    stages[slot].process(l + at, r + at, size, settings[slot],
                                         slot == 0 && driveMod ? driveMod + at : nullptr, smooth);
            }
            for (int i = 0; i < size; ++i) {
                const auto gain = transition.tick();
                l[at + i] *= gain;
                r[at + i] *= gain;
            }
            at += size;
        }
    }
};
} // namespace logisth
