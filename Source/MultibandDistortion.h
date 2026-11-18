#pragma once
#include "Distortion.h"
#include <complex>
namespace logisth {
// Fourth-order Linkwitz-Riley splits. Lower bands receive the later splits'
// second-order allpasses so the linear sum has unity magnitude at every frequency.
class DistortionCrossover {
    struct Coefficients {
        double b0 = 1, b1 = 0, b2 = 0, a1 = 0, a2 = 0;
        double tptA1 = 1, tptA2 = 0, tptA3 = 0;
        std::complex<double> response(double hz, double rate) const {
            const auto z = std::polar(1., -2 * pi * hz / rate);
            return (b0 + b1 * z + b2 * z * z) / (1. + a1 * z + a2 * z * z);
        }
    };
    struct Filter {
        double ic1 = 0, ic2 = 0;
        double tick(double x, const Coefficients &c, int type) noexcept {
            const double v3 = x - ic2;
            const double v1 = c.tptA1 * ic1 + c.tptA2 * v3;
            const double v2 = ic2 + c.tptA2 * ic1 + c.tptA3 * v3;
            ic1 = 2 * v1 - ic1;
            ic2 = 2 * v2 - ic2;
            return type == 0   ? v2
                   : type == 1 ? x - std::sqrt(2.) * v1 - v2
                               : x - 2 * std::sqrt(2.) * v1;
        }
    };
    struct Pair {
        std::array<Filter, 2> low, high;
    };
    std::array<std::array<Pair, 3>, 2> split{};
    std::array<std::array<std::array<Filter, 3>, 3>, 2> align{};
    std::array<Coefficients, 3> low{}, high{}, allpass{};
    std::array<double, 3> frequency{}, target{};
    double rate = 44100, smooth = 0;
    int bands = 4;
    uint32_t counter = 0;
    bool initial = true;
    static std::array<Coefficients, 3> coefficients(double hz, double sr) noexcept {
        const double w = 2 * pi * hz / sr, cosine = std::cos(w),
                     alpha = std::sin(w) / std::sqrt(2.);
        const double a0 = 1 + alpha, a1 = -2 * cosine / a0, a2 = (1 - alpha) / a0;
        const double l0 = (1 - cosine) / (2 * a0), h0 = (1 + cosine) / (2 * a0);
        std::array<Coefficients, 3> result{
            {{l0, 2 * l0, l0, a1, a2}, {h0, -2 * h0, h0, a1, a2}, {a2, a1, 1, a1, a2}}};
        const double g = std::tan(w * .5), tpt = 1 / (1 + g * (g + std::sqrt(2.)));
        for (auto &c : result) {
            c.tptA1 = tpt;
            c.tptA2 = g * tpt;
            c.tptA3 = g * g * tpt;
        }
        return result;
    }
    void update() noexcept {
        for (int i = 0; i < bands - 1; ++i) {
            frequency[size_t(i)] *=
                std::exp(smooth * std::log(target[size_t(i)] / frequency[size_t(i)]));
            const auto c = coefficients(frequency[size_t(i)], rate);
            low[size_t(i)] = c[0];
            high[size_t(i)] = c[1];
            allpass[size_t(i)] = c[2];
        }
    }

  public:
    static std::array<double, 3> cutoffs(const std::array<float, 3> &raw, int count,
                                         double sr) noexcept {
        const double top = std::min(20000., .45 * std::max(1000., sr));
        count = std::clamp(count, 1, 4);
        std::array<double, 3> result{};
        double bottom = 20;
        for (int i = 0; i < count - 1; ++i) {
            const double value = std::isfinite(raw[size_t(i)])
                                     ? raw[size_t(i)]
                                     : std::array<double, 3>{200, 2000, 8000}[size_t(i)];
            result[size_t(i)] = std::clamp(value, bottom, top / std::pow(1.03, count - 2 - i));
            bottom = result[size_t(i)] * 1.03;
        }
        return result;
    }
    static std::array<std::complex<double>, 4>
    response(double hz, int count, const std::array<float, 3> &raw, double sr) noexcept {
        count = std::clamp(count, 1, 4);
        const auto cut = cutoffs(raw, count, sr);
        std::array<std::complex<double>, 4> result{};
        std::complex<double> remainder = 1;
        for (int i = 0; i < count - 1; ++i) {
            const auto c = coefficients(cut[size_t(i)], sr);
            result[size_t(i)] = remainder * std::pow(c[0].response(hz, sr), 2);
            for (int j = i + 1; j < count - 1; ++j)
                result[size_t(i)] *= coefficients(cut[size_t(j)], sr)[2].response(hz, sr);
            remainder *= std::pow(c[1].response(hz, sr), 2);
        }
        result[size_t(count - 1)] = remainder;
        return result;
    }
    void prepare(double sampleRate) noexcept {
        rate = std::isfinite(sampleRate) && sampleRate >= 1000 ? sampleRate : 44100;
        smooth = 1 - std::exp(-32 / (.015 * rate));
        reset();
    }
    void reset() noexcept {
        split = {};
        align = {};
        initial = true;
        counter = 0;
    }
    void configure(int count, const std::array<float, 3> &cuts) noexcept {
        count = std::clamp(count, 1, 4);
        if (bands != count) {
            bands = count;
            reset();
        }
        target = cutoffs(cuts, bands, rate);
        if (initial) {
            frequency = target;
            initial = false;
            update();
        }
    }
    std::array<std::array<float, 4>, 2> tick(float l, float r) noexcept {
        if (counter++ % 32 == 0)
            update();
        std::array<std::array<float, 4>, 2> out{};
        for (int ch = 0; ch < 2; ++ch) {
            double remainder = ch == 0 ? l : r;
            for (int band = 0; band < bands - 1; ++band) {
                auto &s = split[size_t(ch)][size_t(band)];
                double value = remainder;
                for (auto &filter : s.low)
                    value = filter.tick(value, low[size_t(band)], 0);
                for (auto &filter : s.high)
                    remainder = filter.tick(remainder, high[size_t(band)], 1);
                for (int later = band + 1; later < bands - 1; ++later)
                    value = align[size_t(ch)][size_t(band)][size_t(later)].tick(
                        value, allpass[size_t(later)], 2);
                out[size_t(ch)][size_t(band)] = float(value);
            }
            out[size_t(ch)][size_t(bands - 1)] = float(remainder);
        }
        return out;
    }
};

// Legacy remains the original serial rack. Multiband reuses its nonlinear kernel
// independently for each band; no extra audio buffers, allocation or FFT are needed.
class MultibandDistortionRack {
    struct Routing {
        int bands = 0; // 0 = legacy, 1..4 = multiband.
        DistortionOrder order;
        std::array<int, 4> types{};
        bool operator==(const Routing &b) const noexcept {
            return bands == b.bands &&
                   (bands == 0 || (order.slots == b.order.slots && types == b.types));
        }
    } current, pending;
    DistortionRack legacy;
    std::array<DistortionRack, 4> parallel;
    DistortionCrossover crossover;
    std::array<float, 4> enabled{};
    double rate = 44100;
    float blend = 0, gain = 1, startGain = 1;
    int fadeLength = 1, remaining = 0;
    enum class Phase { Stable, Down, Up } phase = Phase::Stable;
    bool initial = true;
    void resetBands() noexcept {
        for (auto &rack : parallel)
            rack.reset();
        crossover.reset();
        enabled = {};
        blend = 0;
    }
    static Routing routing(bool multi, int count, DistortionOrder order,
                           const std::array<DistortionSettings, 4> &p) noexcept {
        Routing result;
        result.bands = multi ? std::clamp(count, 1, 4) : 0;
        result.order = order.valid() ? order : DistortionOrder{};
        for (int i = 0; i < 4; ++i)
            result.types[size_t(i)] = std::clamp(p[size_t(i)].type, 0, 4);
        return result;
    }
    float fade() noexcept {
        if (phase == Phase::Stable)
            return 1;
        const float t = float(fadeLength - remaining + 1) / fadeLength;
        gain = phase == Phase::Down ? startGain * (1 - t) : t;
        const float result = gain;
        if (--remaining == 0) {
            if (phase == Phase::Down) {
                const bool wasMulti = current.bands != 0;
                current = pending;
                resetBands();
                if (wasMulti && current.bands == 0)
                    legacy.reset();
                phase = Phase::Up;
                remaining = fadeLength;
            } else
                phase = Phase::Stable;
        }
        return result;
    }
    void request(Routing next) noexcept {
        if (initial) {
            current = pending = next;
            initial = false;
            return;
        }
        if (pending == next)
            return;
        pending = next;
        if (phase != Phase::Down) {
            phase = Phase::Down;
            remaining = fadeLength;
            startGain = gain;
        }
    }

  public:
    static bool active(const std::array<DistortionSettings, 4> &p, DistortionOrder order,
                       int count) noexcept {
        order = order.valid() ? order : DistortionOrder{};
        for (int i = 0; i < std::clamp(count, 1, 4); ++i) {
            const auto &s = p[order.slots[size_t(i)]];
            if (s.enabled && s.mix > 0)
                return true;
        }
        return false;
    }
    void prepare(double sampleRate) noexcept {
        rate = sampleRate;
        fadeLength = std::max(1, int(std::lround(rate * .005)));
        legacy.prepare(rate);
        for (auto &rack : parallel)
            rack.prepare(rate);
        crossover.prepare(rate);
        reset();
    }
    void reset() noexcept {
        legacy.reset();
        resetBands();
        current = {};
        pending = {};
        phase = Phase::Stable;
        gain = startGain = 1;
        remaining = 0;
        initial = true;
    }
    bool hasTail() const noexcept {
        return blend > 0 || phase != Phase::Stable;
    }
    void synchronize(bool multi, int count, DistortionOrder order,
                     const std::array<DistortionSettings, 4> &p) noexcept {
        const auto next = routing(multi, count, order, p);
        if (!(current == next))
            resetBands();
        current = pending = next;
        initial = false;
        phase = Phase::Stable;
        remaining = 0;
        gain = startGain = 1;
    }
    void silence(bool multi, int count, DistortionOrder order,
                 const std::array<DistortionSettings, 4> &p) noexcept {
        const auto next = routing(multi, count, order, p);
        if (!initial && current == next && blend == 0 && phase == Phase::Stable)
            return;
        synchronize(multi, count, order, p);
        resetBands();
    }
    void process(float *l, float *r, int n, const std::array<DistortionSettings, 4> &p,
                 DistortionOrder order, const float *driveMod, float smooth, bool multi, int count,
                 const std::array<float, 3> &cuts) noexcept {
        request(routing(multi, count, order, p));
        for (int at = 0; at < n;) {
            const int size = phase == Phase::Stable ? n - at : std::min(n - at, remaining);
            if (current.bands == 0) {
                legacy.process(l + at, r + at, size, p, order, driveMod ? driveMod + at : nullptr,
                               smooth);
            } else {
                crossover.configure(current.bands, cuts);
                const bool any = active(p, current.order, current.bands);
                for (int i = 0; i < size; ++i) {
                    blend += smooth * ((any ? 1.f : 0.f) - blend);
                    if (!any && blend < 1e-6f)
                        blend = 0;
                    if (blend == 0)
                        continue;
                    const float dryL = l[at + i], dryR = r[at + i];
                    const auto bands = crossover.tick(dryL, dryR);
                    float sumL = 0, sumR = 0;
                    for (int band = 0; band < current.bands; ++band) {
                        const auto index = size_t(band);
                        const auto slot = current.order.slots[index];
                        const auto &setting = p[slot];
                        const bool on = setting.enabled && setting.mix > 0;
                        enabled[index] += smooth * ((on ? 1.f : 0.f) - enabled[index]);
                        if (!on && enabled[index] < 1e-6f)
                            enabled[index] = 0;
                        float wetL = bands[0][index], wetR = bands[1][index];
                        if (enabled[index] > 0) {
                            auto isolated = setting;
                            isolated.enabled = true;
                            isolated.type = current.types[slot];
                            parallel[index].processIsolatedStage(
                                &wetL, &wetR, 1, isolated,
                                slot == 0 && driveMod ? driveMod + at + i : nullptr, smooth);
                        }
                        sumL += bands[0][index] + (wetL - bands[0][index]) * enabled[index];
                        sumR += bands[1][index] + (wetR - bands[1][index]) * enabled[index];
                    }
                    l[at + i] += (sumL - dryL) * blend;
                    r[at + i] += (sumR - dryR) * blend;
                }
            }
            if (phase != Phase::Stable)
                for (int i = 0; i < size; ++i) {
                    const auto f = fade();
                    l[at + i] *= f;
                    r[at + i] *= f;
                }
            at += size;
        }
    }
};
} // namespace logisth
