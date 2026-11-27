#pragma once
#include <algorithm>
#include <array>
#include <cmath>
#include <vector>

namespace logisth {
struct FlangerControls {
    bool enabled = false;
    float delayMs = 2.5f, depth = .75f, rate = .22f, feedback = .35f, width = .8f,
          damping = 7000.f, mix = .5f;
};

class Flanger {
    static constexpr double cycle = 6.283185307179586;
    double sampleRate = 48000, phase = 0, smoothing = 0;
    size_t writeIndex = 0;
    std::array<std::vector<float>, 2> lines;
    std::array<float, 2> filtered{};
    std::array<float, 7> current{};
    bool primed = false;

    float read(const std::vector<float> &line, double delay) const noexcept {
        const auto whole = size_t(delay);
        const float t = float(delay - whole);
        const size_t n = line.size();
        const size_t b = (writeIndex + n - whole) % n;
        const float a = line[(b + 1) % n], x = line[b];
        const float y = line[(b + n - 1) % n], d = line[(b + n - 2) % n];
        return x + .5f * t * (y - a + t * (2 * a - 5 * x + 4 * y - d +
                                            t * (3 * (x - y) + d - a)));
    }

  public:
    void prepare(double rate) {
        sampleRate = rate;
        smoothing = -std::expm1(-1.0 / (.012 * rate));
        for (auto &line : lines)
            line.assign(size_t(std::ceil(rate * .025)) + 4, 0);
        reset();
    }

    void reset() noexcept {
        for (auto &line : lines)
            std::fill(line.begin(), line.end(), 0.f);
        phase = 0;
        writeIndex = 0;
        filtered = {};
        current = {};
        primed = false;
    }

    bool active(const FlangerControls &p) const noexcept {
        return (p.enabled && p.mix > 0) || (primed && current[6] > 1e-5f);
    }

    void advance(int count, float rate) noexcept {
        phase += count * double(rate) / sampleRate;
        phase -= std::floor(phase);
    }

    void process(float *left, float *right, int count, const FlangerControls &p) noexcept {
        if (lines[0].empty())
            return;
        const std::array<float, 7> target{
            std::clamp(p.delayMs, .3f, 10.f), std::clamp(p.depth, 0.f, 1.f),
            std::clamp(p.rate, .01f, 40.f), std::clamp(p.feedback, -.9f, .9f),
            std::clamp(p.width, 0.f, 1.f),
            float(-std::expm1(-cycle * std::clamp(double(p.damping), 200., sampleRate * .45) /
                             sampleRate)),
            p.enabled ? std::clamp(p.mix, 0.f, 1.f) : 0.f};
        if (!primed) {
            current = target;
            primed = true;
        }
        float *channels[]{left, right};
        for (int i = 0; i < count; ++i) {
            for (size_t j = 0; j < current.size(); ++j)
                current[j] += float(smoothing) * (target[j] - current[j]);
            const auto [delay, depth, rate, feedback, width, damp, mix] = current;
            for (size_t ch = 0; ch < 2; ++ch) {
                const double lfo = std::sin(phase * cycle + (double(ch) - .5) * width * cycle * .5);
                const double samples = std::max(2., delay * .001 * sampleRate * (1. + .93 * depth * lfo));
                const float dry = channels[ch][i], wet = read(lines[ch], samples);
                filtered[ch] += damp * (wet - filtered[ch]);
                // Limit regeneration gently while leaving the feed-forward tap clean.
                const float regen = feedback * filtered[ch];
                lines[ch][writeIndex] = dry + regen / (1 + .12f * std::abs(regen));
                channels[ch][i] = dry + mix * (wet - dry);
            }
            if (++writeIndex == lines[0].size())
                writeIndex = 0;
            advance(1, rate);
        }
    }
};
} // namespace logisth
