#pragma once
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
namespace logisth {
enum class EffectStage : unsigned {
    Distortion,
    Phaser,
    Chorus,
    Equalizer,
    Reverb,
    Delay,
    Compressor,
    Clipper,
    Flanger
};
inline constexpr int effectStageCount = 9;
struct EffectOrder {
    std::array<EffectStage, effectStageCount> stages{
        EffectStage::Distortion, EffectStage::Phaser, EffectStage::Chorus, EffectStage::Flanger,
        EffectStage::Equalizer,  EffectStage::Reverb, EffectStage::Delay,
        EffectStage::Compressor, EffectStage::Clipper};
    bool valid() const noexcept {
        unsigned seen = 0;
        for (auto stage : stages) {
            const auto id = unsigned(stage);
            if (id >= effectStageCount || (seen & (1u << id)))
                return false;
            seen |= 1u << id;
        }
        return true;
    }
    uint64_t packed() const noexcept {
        uint64_t value = 0;
        for (unsigned i = 0; i < stages.size(); ++i)
            value |= uint64_t(stages[i]) << (i * 4);
        return value;
    }
    static bool unpack(uint64_t value, EffectOrder &result) noexcept {
        if (value >> (effectStageCount * 4))
            return false;
        EffectOrder order;
        if (value >> 24) {
            for (unsigned i = 0; i < order.stages.size(); ++i)
                order.stages[i] = EffectStage((value >> (4 * i)) & 15);
        } else {
            // The old encoding stored eight three-bit slots.
            size_t at = 0;
            for (unsigned i = 0; i < 8; ++i) {
                auto stage = EffectStage((value >> (3 * i)) & 7);
                order.stages[at++] = stage;
                if (stage == EffectStage::Chorus && at < order.stages.size())
                    order.stages[at++] = EffectStage::Flanger;
                if (at == order.stages.size() && i != 7)
                    return false;
            }
        }
        if (!order.valid())
            return false;
        result = order;
        return true;
    }
    bool operator==(const EffectOrder &other) const noexcept {
        return stages == other.stages;
    }
    bool operator!=(const EffectOrder &other) const noexcept {
        return !(*this == other);
    }
    bool move(int from, int to) noexcept {
        if (from < 0 || from >= effectStageCount || to < 0 || to >= effectStageCount || from == to)
            return false;
        auto stage = stages[size_t(from)];
        if (from < to)
            std::move(stages.begin() + from + 1, stages.begin() + to + 1, stages.begin() + from);
        else
            std::move_backward(stages.begin() + to, stages.begin() + from,
                               stages.begin() + from + 1);
        stages[size_t(to)] = stage;
        return true;
    }
};
// One state bank, no duplicate FX rendering: a live reorder fades down for 5 ms,
// switches only at zero, then fades back for 5 ms. Preparation snaps to the saved order.
class RackTransition {
    EffectOrder current, pending;
    int length = 1, remaining = 0;
    float gain = 1, initial = 1;
    enum class Phase { Stable, Down, Up } phase = Phase::Stable;

  public:
    void reset(double rate, EffectOrder order) noexcept {
        current = pending = order.valid() ? order : EffectOrder{};
        length = std::max(1, int(std::lround(rate * .005)));
        remaining = 0;
        gain = initial = 1;
        phase = Phase::Stable;
    }
    void request(EffectOrder order) noexcept {
        if (!order.valid() || pending == order)
            return;
        pending = order;
        if (phase != Phase::Down) {
            phase = Phase::Down;
            remaining = length;
            initial = gain;
        }
    }
    const EffectOrder &order() const noexcept {
        return current;
    }
    int blockLimit(int wanted) const noexcept {
        return phase == Phase::Stable ? wanted : std::min(wanted, remaining);
    }
    float tick() noexcept {
        if (phase == Phase::Stable)
            return 1;
        const float t = float(length - remaining + 1) / float(length);
        gain = phase == Phase::Down ? initial * (1 - t) : t;
        const float result = gain;
        if (--remaining == 0) {
            if (phase == Phase::Down) {
                current = pending;
                phase = Phase::Up;
                remaining = length;
            } else
                phase = Phase::Stable;
        }
        return result;
    }
};
} // namespace logisth
