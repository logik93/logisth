#pragma once
#include "Modulation.h"
#include "PhaseWidth.h"
#include "Spectral.h"
#include <cstdint>
namespace logisth {
template <class Data>
float resample(const Data &data, double at, int channel, int quality, double speed, double first,
               double last, bool wrap) {
    auto &samples = data.samples[size_t(channel)];
    if (samples.size() < 2)
        return 0;
    const double span = std::max(1., last - first);
    auto read = [&](int64_t index) {
        double x = double(index);
        if (wrap) {
            x = first + std::fmod(x - first, span);
            if (x < first)
                x += span;
        }
        x = std::clamp(x, first, last - 1);
        return samples[size_t(std::clamp(int64_t(x), int64_t(0), int64_t(samples.size() - 1)))];
    };
    int64_t i = int64_t(std::floor(at));
    float f = float(at - i);
    if (quality == 0)
        return read(i) * (1 - f) + read(i + 1) * f;
    if (quality == 1) {
        float a = read(i - 1), b = read(i), c = read(i + 1), d = read(i + 2);
        return b + .5f * f * (c - a + f * (2 * a - 5 * b + 4 * c - d + f * (3 * (b - c) + d - a)));
    }
    int half = quality >= 3 ? 16 : 8;
    double sum = 0, weight = 0, cutoff = std::min(1., 1 / std::max(1., std::abs(speed)));
    for (int k = -half + 1; k <= half; ++k) {
        double distance = double(k) - f;
        double x = pi * distance * cutoff;
        double sinc = std::abs(x) < 1e-12 ? 1 : std::sin(x) / x;
        double window =
            .42 + .5 * std::cos(pi * distance / half) + .08 * std::cos(2 * pi * distance / half);
        double w = sinc * window * cutoff;
        sum += read(i + k) * w;
        weight += w;
    }
    return std::abs(weight) > 1e-12 ? float(sum / weight) : 0;
}
struct TailLoopSettings {
    bool enabled = false;
    float start = .5f, end = 1, fadeMs = 5;
    bool operator==(const TailLoopSettings &other) const {
        return enabled == other.enabled && start == other.start && end == other.end &&
               fadeMs == other.fadeMs;
    }
    bool operator!=(const TailLoopSettings &other) const {
        return !(*this == other);
    }
};
struct TailLoopBounds {
    double first = 0, last = 2, span = 2, low = 0, high = 2, fade = 0;
    bool reverse = false;
    static TailLoopBounds make(double length, double begin, double end, bool backwards,
                               double sampleRate, const TailLoopSettings &settings) {
        TailLoopBounds b;
        b.first = begin * (length - 2);
        b.last = std::min(length - 1, std::max(b.first + 2., end * (length - 1)));
        b.span = std::max(1., b.last - b.first);
        b.reverse = backwards;
        const double minimum = std::min(2., b.span);
        const double a =
            std::isfinite(settings.start) ? std::clamp(double(settings.start), 0., 1.) : .5;
        const double z =
            std::isfinite(settings.end) ? std::clamp(double(settings.end), 0., 1.) : 1.;
        b.low = std::clamp(a * (length - 1) - b.first, 0., std::max(0., b.span - minimum));
        b.high = std::clamp(z * (length - 1) - b.first, b.low + minimum, b.span);
        if (backwards) {
            const double oldLow = b.low;
            b.low = b.span - b.high;
            b.high = b.span - oldLow;
        }
        const double ms =
            std::isfinite(settings.fadeMs) ? std::clamp(double(settings.fadeMs), 0., 200.) : 5.;
        b.fade = std::min((b.high - b.low) * .25, sampleRate * ms * .001);
        return b;
    }
    double mapped(double cursor) const {
        if (cursor < span)
            return cursor;
        return low + fade + std::fmod(cursor - span, std::max(1e-9, high - low - fade));
    }
    template <class Data>
    float read(const Data &data, double cursor, int channel, int quality, double speed) const {
        if (cursor < 0)
            return 0;
        auto physical = [&](double distance, bool tail) {
            double lo = first, hi = last;
            if (tail) {
                lo += reverse ? span - high : low;
                hi = first + (reverse ? span - low : high);
            }
            return resample(data, reverse ? last - 1 - distance : first + distance, channel,
                            quality, speed, lo, hi, false);
        };
        const bool entered = cursor >= span;
        const double position = mapped(cursor), boundary = entered ? high : span;
        const float a = physical(position, entered);
        if (fade <= 0 || position < boundary - fade)
            return a;
        const double into = std::clamp(position - (boundary - fade), 0., fade);
        const float mix = float(into / fade);
        return a * (1 - mix) + physical(low + into, true) * mix;
    }
};
struct SamplerPlayer {
    struct Grain {
        double position = 0;
        int age = 0;
        bool active = false;
    };
    std::array<Grain, 16> grains{};
    double rateCache = 0, smoothing = 0;
    double cursor = 0, untilGrain = 0, lastStart = 0, stretch = 1;
    double elapsedSamples = 0, displayCursor = 0;
    float rateControl = 1, pitchControl = 0, tonalityControl = 0, formantControl = 0;
    bool controlsReady = false;
    SpectralStream *spectral = nullptr;
    uint64_t token = 0;
    bool spectralReset = true, spectralWasReady = false;
    float spectralBlend = 1;
    int algorithm = -1;
    bool finished = false, started = false;
    void reset() {
        auto *saved = spectral;
        auto serial = token;
        *this = SamplerPlayer{};
        spectral = saved;
        token = serial;
    }
    template <class Data>
    std::array<float, 2> tick(const Data &data, double ratio, double sr, float begin, float end,
                              bool loop, bool reverse, int mode, float factor, int interpolation,
                              float width = .5f, double fmOffset = 0, float kickAttack = 25,
                              const StretchControls &controls = {},
                              const TailLoopSettings &tail = {}) {
        if (algorithm != mode) {
            reset();
            algorithm = mode;
            stretch =
                std::clamp(double(factor), .125, 8.) /
                (std::isfinite(controls.rate) ? std::clamp(double(controls.rate), .125, 8.) : 1.);
        }
        double length = double(data.samples[0].size());
        if (length < 2)
            return {};
        double first = begin * (length - 2),
               last = std::min(length - 1, std::max(first + 2., end * (length - 1))),
               span = std::max(1., last - first);
        if (rateCache != sr) {
            rateCache = sr;
            smoothing = 1 - std::exp(-1 / (.02 * sr));
        }
        const bool tailOn = tail.enabled;
        const auto tailBounds =
            TailLoopBounds::make(length, begin, end, reverse, data.sampleRate, tail);
        displayCursor = tailOn ? tailBounds.mapped(cursor) : cursor;
        if (tailOn && finished) {
            finished = false;
            cursor = std::max(cursor, span);
        }
        const float rateTarget =
            std::isfinite(controls.rate) ? std::clamp(controls.rate, .125f, 8.f) : 1;
        const float pitchTarget =
            std::isfinite(controls.semitones) ? std::clamp(controls.semitones, -48.f, 48.f) : 0;
        const float tonalityTarget = std::isfinite(controls.tonalityHz)
                                         ? std::clamp(controls.tonalityHz, 0.f, float(sr * .49))
                                         : 0;
        const float formantTarget =
            std::isfinite(controls.formant) ? std::clamp(controls.formant, -24.f, 24.f) : 0;
        if (!controlsReady) {
            rateControl = rateTarget;
            pitchControl = pitchTarget;
            tonalityControl = tonalityTarget;
            formantControl = formantTarget;
            controlsReady = true;
        }
        rateControl += float(smoothing) * (rateTarget - rateControl);
        pitchControl += float(smoothing) * (pitchTarget - pitchControl);
        tonalityControl += float(smoothing) * (tonalityTarget - tonalityControl);
        formantControl += float(smoothing) * (formantTarget - formantControl);
        stretch += smoothing * (std::clamp(double(factor) / rateTarget, 1. / 64, 64.) - stretch);
        ratio *= std::exp2(double(pitchControl) / 12);
        double pitch = ratio * data.sampleRate / sr;
        auto read = [&](double offset, int ch) {
            offset = warpSamplePosition(offset, data.sampleRate * .04, width) + fmOffset;
            if (tailOn)
                return tailBounds.read(data, offset, ch, interpolation,
                                       mode >= 6   ? data.sampleRate / sr
                                       : mode == 1 ? pitch / stretch
                                       : mode == 0 ? pitch * rateControl
                                                   : pitch);
            double at = reverse ? last - 1 - offset : first + offset;
            if (!loop && (at < first || at >= last))
                return 0.f;
            return resample(data, at, ch, interpolation,
                            mode >= 6   ? data.sampleRate / sr
                            : mode == 1 ? pitch / stretch
                            : mode == 0 ? pitch * rateControl
                                        : pitch,
                            first, last, loop);
        };
        if (mode >= 6 && spectral && spectral->matches(sr, controls)) {
            if (finished)
                return {};
            double step = data.sampleRate / sr, attack = kickAttack * .001 * sr;
            float timeFactor = mode == 6 && elapsedSamples < attack ? 1.f : float(stretch);
            auto live = controls;
            live.tonalityHz = tonalityControl;
            live.formant = formantControl;
            auto out = spectral->tick(read, cursor, step, float(ratio), timeFactor, token,
                                      data.revision, spectralReset, live);
            spectralReset = false;
            if (!spectralWasReady) {
                spectralWasReady = true;
                spectralBlend = started ? 0.f : 1.f;
            }
            if (spectralBlend < 1) {
                auto *savedStream = spectral;
                double savedCursor = cursor, savedElapsed = elapsedSamples, savedStretch = stretch;
                bool savedFinished = finished;
                spectral = nullptr;
                auto old = tick(data, ratio, sr, begin, end, loop, reverse, mode, factor,
                                interpolation, width, fmOffset, kickAttack, controls, tail);
                spectral = savedStream;
                cursor = savedCursor;
                elapsedSamples = savedElapsed;
                stretch = savedStretch;
                finished = savedFinished;
                spectralBlend = std::min(1.f, spectralBlend + float(1 / (sr * .02)));
                for (int ch = 0; ch < 2; ++ch)
                    out[size_t(ch)] =
                        old[size_t(ch)] + (out[size_t(ch)] - old[size_t(ch)]) * spectralBlend;
            }
            if (mode == 6) {
                float mix = float(std::clamp((elapsedSamples - attack) / (sr * .008), 0., 1.));
                for (int ch = 0; ch < 2; ++ch)
                    out[size_t(ch)] =
                        read(elapsedSamples * step, ch) * (1 - mix) + out[size_t(ch)] * mix;
            }
            if (!loop && !tailOn) {
                float fade = float(std::clamp((span - cursor) / (step * sr * .005), 0., 1.));
                for (auto &x : out)
                    x *= fade;
            }
            cursor += step / timeFactor;
            started = true;
            ++elapsedSamples;
            if (cursor >= span && !tailOn) {
                if (loop)
                    cursor = std::fmod(cursor, span);
                else {
                    cursor = span;
                    finished = true;
                }
            }
            return out;
        }
        if (mode <= 1) {
            if (finished)
                return {};
            if (cursor >= span && !tailOn) {
                if (loop) {
                    double fade = std::min(span * .25, data.sampleRate * .005);
                    cursor = fade + std::fmod(cursor - span, span - fade);
                } else {
                    finished = true;
                    return {};
                }
            }
            std::array<float, 2> out{};
            double fade = std::min(span * .25, data.sampleRate * .005);
            for (int ch = 0; ch < 2; ++ch) {
                out[size_t(ch)] = read(cursor, ch);
                if (loop && !tailOn && span - cursor < fade) {
                    float mix = float((span - cursor) / fade);
                    out[size_t(ch)] =
                        out[size_t(ch)] * mix + read(cursor - (span - fade), ch) * (1 - mix);
                }
            }
            ++elapsedSamples;
            cursor += pitch * (mode == 0 ? rateControl : 1 / stretch);
            return out;
        }
        const double block = controls.blockMs > 0 && std::isfinite(controls.blockMs)
                                 ? std::clamp(double(controls.blockMs), .1, 200.) * .001
                             : mode == 5 ? .012
                             : mode == 4 ? .065
                                         : .04;
        const int window = std::max(32, int(sr * block));
        const int overlap = controls.overlap == 0 ? 2 : std::clamp(controls.overlap, 2, 16);
        const int hop = std::max(1, window / overlap);
        if (!started) {
            grains[0] = {cursor, hop, true};
            started = true;
        }
        if (untilGrain <= 0 && !finished) {
            double at = cursor;
            if (mode >= 3 && cursor > 0) {
                // WSOLA: align the next window to the previous window's continuation.
                double predicted = lastStart + pitch * hop, best = -1e30;
                int search = std::min(192, int(data.sampleRate * .006)),
                    step = std::max(1, search / 24);
                auto scoreAt = [&](int shift) {
                    double score = 0, a2 = 1e-12, b2 = 1e-12, candidate = cursor + shift;
                    if ((!loop && !tailOn && candidate >= span) ||
                        ((!loop || tailOn) && candidate < 0))
                        return -1e30;
                    for (int point = 0; point < 32; ++point) {
                        double delta = double(point) * hop / 32;
                        auto quick = [&](double o) {
                            if (tailOn)
                                return tailBounds.read(data, o, 0, 0, pitch);
                            double x = reverse ? last - 1 - o : first + o;
                            return resample(data, x, 0, 0, pitch, first, last, loop);
                        };
                        double a = quick(predicted + delta * pitch),
                               b = quick(candidate + delta * pitch);
                        score += a * b;
                        a2 += a * a;
                        b2 += b * b;
                    }
                    return score / std::sqrt(a2 * b2) -
                           double(std::abs(shift)) / std::max(1, search) * 1e-6;
                };
                int bestShift = 0;
                for (int shift = -search; shift <= search; shift += step) {
                    double score = scoreAt(shift);
                    if (score > best) {
                        best = score;
                        bestShift = shift;
                    }
                }
                const int coarse = bestShift;
                for (int shift = std::max(-search, coarse - step);
                     shift <= std::min(search, coarse + step); ++shift) {
                    double score = scoreAt(shift);
                    if (score > best) {
                        best = score;
                        bestShift = shift;
                    }
                }
                at = cursor + bestShift;
            }
            for (auto &g : grains)
                if (!g.active) {
                    g = {at, 0, true};
                    break;
                }
            lastStart = at;
            untilGrain += hop;
        }
        std::array<float, 2> out{};
        for (auto &g : grains)
            if (g.active) {
                double w = .5 - .5 * std::cos(2 * pi * g.age / window);
                for (int ch = 0; ch < 2; ++ch)
                    out[size_t(ch)] += read(g.position, ch) * float(w);
                g.position += pitch;
                if (++g.age >= window)
                    g.active = false;
            }
        for (auto &value : out)
            value *= 2.f / float(overlap);
        if (mode == 6) {
            double attack = kickAttack * .001 * sr;
            float mix = float(std::clamp((elapsedSamples - attack) / (sr * .008), 0., 1.));
            for (int ch = 0; ch < 2; ++ch)
                out[size_t(ch)] = read(elapsedSamples * data.sampleRate / sr, ch) * (1 - mix) +
                                  out[size_t(ch)] * mix;
        }
        ++elapsedSamples;
        cursor += data.sampleRate / sr /
                  (mode == 6 && elapsedSamples < kickAttack * .001 * sr ? 1 : stretch);
        if (cursor >= span && !tailOn) {
            if (loop)
                cursor = std::fmod(cursor, span);
            else {
                cursor = span;
                finished = true;
            }
        }
        --untilGrain;
        // A non-looping sample ends its windows smoothly rather than jumping to zero.
        return out;
    }
};
} // namespace logisth
