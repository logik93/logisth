#pragma once
#include "Telemetry.h"
#include <cmath>
namespace logisth {
inline float amplitude(float db) {
    return std::pow(10.f, db * .05f);
}
inline float decibels(float x) {
    return 20 * std::log10(std::max(1e-10f, x));
}
inline float compressionReduction(float input, float threshold, float ratio, float knee) {
    const float x = input - threshold, slope = 1 - 1 / std::max(1.f, ratio);
    if (knee <= 0)
        return std::max(0.f, x) * slope;
    if (x <= -knee * .5f)
        return 0;
    if (x >= knee * .5f)
        return x * slope;
    return slope * (x + knee * .5f) * (x + knee * .5f) / (2 * knee);
}
struct DynamicsMeters {
    std::atomic<float> input{-200}, output{-200}, reduction{0};
    void write(float in, float out, float gr) noexcept {
        input.store(decibels(in), std::memory_order_relaxed);
        output.store(decibels(out), std::memory_order_relaxed);
        reduction.store(gr, std::memory_order_relaxed);
    }
};
struct CompressorSettings {
    float threshold = -18, ratio = 4, knee = 6, attack = 10, release = 150, makeup = 0, mix = 1;
};
class Compressor {
    float gainReduction = 0, attack = 0, release = 0, makeup = 1;
    CompressorSettings p;

  public:
    float maximumReduction = 0, inputPeak = 0, outputPeak = 0;
    void reset() noexcept {
        gainReduction = 0;
    }
    void configure(const CompressorSettings &s, double rate) noexcept {
        p = s;
        attack = float(std::exp(-1 / (std::max(.1f, p.attack) * .001 * rate)));
        release = float(std::exp(-1 / (std::max(1.f, p.release) * .001 * rate)));
        makeup = amplitude(p.makeup);
        maximumReduction = inputPeak = outputPeak = 0;
    }
    float advanceSilence(int n) noexcept {
        gainReduction *= std::pow(release, float(n));
        if (gainReduction < 1e-8f)
            gainReduction = 0;
        return gainReduction;
    }
    void tick(float &l, float &r) noexcept {
        const float level = std::max(std::abs(l), std::abs(r));
        const float target = compressionReduction(decibels(level), p.threshold, p.ratio, p.knee);
        const float c = target > gainReduction ? attack : release;
        gainReduction = target + c * (gainReduction - target);
        if (gainReduction < 1e-8f)
            gainReduction = 0;
        const float g = 1 + p.mix * (amplitude(-gainReduction) * makeup - 1);
        l *= g;
        r *= g;
        inputPeak = std::max(inputPeak, level);
        outputPeak = std::max(outputPeak, std::max(std::abs(l), std::abs(r)));
        maximumReduction = std::max(maximumReduction, gainReduction);
    }
};
struct ClipperSettings {
    bool enabled = false, antialias = true;
    int mode = 1;
    float drive = 0, ceiling = -1, knee = .5f, mix = 1;
};
// Analytic curve/integral are shared by the audio engine and the transfer plot.
inline double clipCurve(double x, double ceiling, double knee, bool soft) {
    const double a = std::abs(x), w = soft ? ceiling * std::clamp(knee, 0., 1.) : 0;
    double y;
    if (w < 1e-9)
        y = std::min(a, ceiling);
    else if (a < ceiling - w)
        y = a;
    else if (a >= ceiling + w)
        y = ceiling;
    else {
        const double d = a - (ceiling - w);
        y = a - d * d / (4 * w);
    }
    return std::copysign(y, x);
}
inline double clipIntegral(double x, double ceiling, double knee, bool soft) {
    const double a = std::abs(x), w = soft ? ceiling * std::clamp(knee, 0., 1.) : 0;
    if (w < 1e-9)
        return a <= ceiling ? .5 * a * a : ceiling * a - .5 * ceiling * ceiling;
    const double start = ceiling - w, end = ceiling + w;
    if (a <= start)
        return .5 * a * a;
    auto middle = [&](double v) {
        double d = v - start;
        return .5 * v * v - d * d * d / (12 * w);
    };
    return a < end ? middle(a) : middle(end) + ceiling * (a - end);
}
class Clipper {
    double previous[2]{};
    bool started = false, active = false;

  public:
    void reset() noexcept {
        previous[0] = previous[1] = 0;
        started = false;
        active = false;
    }
    void process(float *l, float *r, int n, const ClipperSettings &p, DynamicsMeters &meter,
                 ScopeBuffer *trace = nullptr) noexcept {
        if (!p.enabled || p.mix <= 0) {
            reset();
            meter.write(0, 0, 0);
            return;
        }
        if (previous[0] == 0 && previous[1] == 0 &&
            std::all_of(l, l + n, [](float x) { return x == 0; }) &&
            std::all_of(r, r + n, [](float x) { return x == 0; })) {
            meter.write(0, 0, 0);
            if (trace)
                trace->clear();
            return;
        }
        const double drive = amplitude(p.drive), ceiling = amplitude(p.ceiling);
        float inPeak = 0, outPeak = 0;
        if (!active)
            started = false;
        active = true;
        for (int i = 0; i < n; ++i) {
            const float input = std::abs(l[i]) >= std::abs(r[i]) ? l[i] : r[i];
            for (int ch = 0; ch < 2; ++ch) {
                float &s = ch == 0 ? l[i] : r[i];
                if (!std::isfinite(s)) {
                    s = 0;
                    previous[ch] = 0;
                }
                const double x = double(s) * drive;
                double wet;
                const double dx = x - previous[ch];
                if (p.antialias && started &&
                    std::abs(dx) > 1e-5 * (1 + std::abs(x) + std::abs(previous[ch])))
                    wet = (clipIntegral(x, ceiling, p.knee, p.mode > 0) -
                           clipIntegral(previous[ch], ceiling, p.knee, p.mode > 0)) /
                          dx;
                else
                    wet = clipCurve(p.antialias && started ? (x + previous[ch]) * .5 : x, ceiling,
                                    p.knee, p.mode > 0);
                previous[ch] = x;
                const double dry = double(s);
                // ADAA wet has approximately half a sample of phase lag. The dry
                // mix is deliberately the original signal; no host latency is added.
                inPeak = std::max(inPeak, std::abs(s));
                s = float(dry + (wet - dry) * p.mix);
                outPeak = std::max(outPeak, std::abs(s));
            }
            started = true;
            if (trace && i % 4 == 0)
                trace->push(input, std::abs(l[i]) >= std::abs(r[i]) ? l[i] : r[i], 0);
        }
        meter.write(inPeak, outPeak,
                    std::max(0.f, decibels(inPeak * float(drive)) - decibels(outPeak)));
        if (trace)
            trace->commit();
    }
};
struct LimiterSettings {
    bool enabled = false;
    float ceiling = -1, release = 100, drive = 0;
};
class OutputLimiter {
    float gain = 1;
    bool active = false;

  public:
    void reset() noexcept {
        gain = 1;
        active = false;
    }
    void process(float *l, float *r, int n, const LimiterSettings &p, double rate,
                 DynamicsMeters &meter, ScopeBuffer *trace = nullptr) noexcept {
        if (!p.enabled) {
            reset();
            meter.write(0, 0, 0);
            return;
        }
        if (!active)
            gain = 1;
        active = true;
        if (gain == 1 && std::all_of(l, l + n, [](float x) { return x == 0; }) &&
            std::all_of(r, r + n, [](float x) { return x == 0; })) {
            meter.write(0, 0, 0);
            if (trace)
                trace->clear();
            return;
        }
        const float ceiling = amplitude(p.ceiling), drive = amplitude(p.drive);
        const float recovery = float(std::exp(-1 / (std::max(1.f, p.release) * .001 * rate)));
        float inPeak = 0, outPeak = 0, gr = 0;
        for (int i = 0; i < n; ++i) {
            float a = l[i] * drive, b = r[i] * drive;
            if (!std::isfinite(a) || !std::isfinite(b)) {
                a = b = 0;
            }
            const float peak = std::max(std::abs(a), std::abs(b));
            const float required = peak > ceiling ? ceiling / peak : 1;
            gain = std::min(required, 1 + recovery * (gain - 1));
            if (1 - gain < 1e-5f && required == 1)
                gain = 1;
            l[i] = std::clamp(a * gain, -ceiling, ceiling);
            r[i] = std::clamp(b * gain, -ceiling, ceiling);
            inPeak = std::max(inPeak, peak);
            outPeak = std::max(outPeak, std::max(std::abs(l[i]), std::abs(r[i])));
            const float reduction = -decibels(gain);
            gr = std::max(gr, reduction);
            if (trace && i % 32 == 0)
                trace->push(peak, std::max(std::abs(l[i]), std::abs(r[i])), reduction);
        }
        meter.write(inPeak, outPeak, gr);
        if (trace)
            trace->commit();
    }
};
} // namespace logisth
