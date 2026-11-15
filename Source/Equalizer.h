#pragma once
#include "Modulation.h"
#include <complex>
#include <cstdint>
namespace logisth {
inline constexpr int eqMaxBands = 16;
inline constexpr std::array<double, eqMaxBands> eqInitialFrequencies{
    31.5, 63, 125, 250, 500, 1000, 2000, 4000, 8000, 16000, 80, 180, 700, 3000, 6000, 12000};
struct EqualizerParams {
    int eqMode = 0;
    bool eqAutoGain = false;
    float eqOutput = 0;
    std::array<float, eqMaxBands> eqBands{};
    std::array<int, eqMaxBands> eqTypes{};
    std::array<float, eqMaxBands> eqFrequencies = [] {
        std::array<float, eqMaxBands> values{};
        for (int i = 0; i < eqMaxBands; ++i)
            values[size_t(i)] = float(eqInitialFrequencies[size_t(i)]);
        return values;
    }();
    std::array<float, eqMaxBands> eqQ = [] {
        std::array<float, eqMaxBands> v{};
        v.fill(1.41421356237f);
        return v;
    }();
    std::array<bool, eqMaxBands> eqEnabled = [] {
        std::array<bool, eqMaxBands> v{};
        for (int i = 0; i < 10; ++i)
            v[size_t(i)] = true;
        return v;
    }();
};
class Equalizer {
    struct Filter {
        double b0 = 1, b1 = 0, b2 = 0, a1 = 0, a2 = 0, z1 = 0, z2 = 0;
        float tick(float x) {
            double y = b0 * x + z1;
            z1 = b1 * x - a1 * y + z2;
            z2 = b2 * x - a2 * y;
            return float(y);
        }
        void normalize(double a0) {
            b0 /= a0;
            b1 /= a0;
            b2 /= a0;
            a1 /= a0;
            a2 /= a0;
        }
        void set(double sr, double frequency, double db, bool shelf, bool high = false,
                 double q = 1.41421356237) {
            double A = std::pow(10., db / 40),
                   w = 2 * pi * std::clamp(frequency, 15., sr * .45) / sr, c = std::cos(w),
                   s = std::sin(w), a0;
            if (!shelf) {
                double alpha = s / (2 * q);
                b0 = 1 + alpha * A;
                b1 = -2 * c;
                b2 = 1 - alpha * A;
                a0 = 1 + alpha / A;
                a1 = -2 * c;
                a2 = 1 - alpha / A;
            } else {
                double beta = s * std::sqrt(2 * A) * (1.41421356237 / q);
                if (high) {
                    b0 = A * ((A + 1) + (A - 1) * c + beta);
                    b1 = -2 * A * ((A - 1) + (A + 1) * c);
                    b2 = A * ((A + 1) + (A - 1) * c - beta);
                    a0 = (A + 1) - (A - 1) * c + beta;
                    a1 = 2 * ((A - 1) - (A + 1) * c);
                    a2 = (A + 1) - (A - 1) * c - beta;
                } else {
                    b0 = A * ((A + 1) - (A - 1) * c + beta);
                    b1 = 2 * A * ((A - 1) - (A + 1) * c);
                    b2 = A * ((A + 1) - (A - 1) * c - beta);
                    a0 = (A + 1) + (A - 1) * c + beta;
                    a1 = -2 * ((A - 1) + (A + 1) * c);
                    a2 = (A + 1) + (A - 1) * c - beta;
                }
            }
            normalize(a0);
        }
        void graphic(double sr, int type, double frequency, double db, double q) {
            q = std::clamp(q, .1, 20.);
            if (std::abs(q - double(1.41421356237f)) < 1e-8)
                q = 1.41421356237;
            if (type <= 2) {
                set(sr, frequency, db, type != 0, type == 2, q);
                return;
            }
            double w = 2 * pi * std::clamp(frequency, 15., sr * .45) / sr, c = std::cos(w),
                   alpha = std::sin(w) / (2 * q);
            double a0 = 1 + alpha;
            a1 = -2 * c;
            a2 = 1 - alpha;
            switch (type) {
            case 3:
                b0 = (1 + c) * .5;
                b1 = -(1 + c);
                b2 = b0;
                break;
            case 4:
                b0 = (1 - c) * .5;
                b1 = 1 - c;
                b2 = b0;
                break;
            case 5:
                b0 = 1;
                b1 = -2 * c;
                b2 = 1;
                break;
            case 6:
                b0 = alpha;
                b1 = 0;
                b2 = -alpha;
                break;
            default:
                b0 = 1 - alpha;
                b1 = -2 * c;
                b2 = 1 + alpha;
                break;
            }
            normalize(a0);
        }
        double magnitude(double frequency, double sr) const {
            auto z = std::polar(1., -2 * pi * frequency / sr);
            return std::abs((b0 + b1 * z + b2 * z * z) / (1. + a1 * z + a2 * z * z));
        }
    };
    std::array<std::array<Filter, 2>, 2> legacy{};
    std::array<std::array<Filter, eqMaxBands>, 2> bands{}, previous{};
    std::array<float, 4 + eqMaxBands> current{}, target{};
    std::array<float, eqMaxBands> frequency{}, frequencyTarget{}, q{}, qTarget{}, wet{},
        wetTarget{}, typeMix{};
    std::array<int, eqMaxBands> types{}, oldTypes{};
    double sr = 44100;
    float coefficient = 0, sampleCoefficient = 0, mode = 0, modeTarget = 0, gain = 1,
          gainTarget = 1;
    uint32_t counter = 0;
    bool initial = false;
    void update() {
        for (int ch = 0; ch < 2; ++ch) {
            legacy[size_t(ch)][0].set(sr, current[2], current[0], true);
            legacy[size_t(ch)][1].set(sr, current[3], current[1], true, true);
            for (int i = 0; i < eqMaxBands; ++i) {
                auto n = size_t(i);
                bands[size_t(ch)][n].graphic(sr, types[n], frequency[n], current[n + 4], q[n]);
                if (typeMix[n] < 1)
                    previous[size_t(ch)][n].graphic(sr, oldTypes[n], frequency[n], current[n + 4],
                                                    q[n]);
            }
        }
    }

  public:
    inline static constexpr std::array<double, 10> frequencies{31.5, 63,   125,  250,  500,
                                                               1000, 2000, 4000, 8000, 16000};
    void prepare(double rate) {
        sr = rate;
        coefficient = float(1 - std::exp(-32 / (.015 * sr)));
        sampleCoefficient = float(1 - std::exp(-1 / (.015 * sr)));
        reset();
    }
    void reset() {
        legacy = {};
        bands = {};
        previous = {};
        current = {};
        target = {};
        frequency = {};
        frequencyTarget = {};
        q = {};
        qTarget = {};
        wet = {};
        wetTarget = {};
        types = {};
        oldTypes = {};
        typeMix.fill(1);
        counter = 0;
        initial = false;
        mode = modeTarget = 0;
        gain = gainTarget = 1;
    }
    void configure(const EqualizerParams &p, float bass, float bassFreq, float treble,
                   float trebleFreq) {
        target[0] = bass;
        target[1] = treble;
        target[2] = bassFreq;
        target[3] = trebleFreq;
        for (int i = 0; i < eqMaxBands; ++i) {
            auto n = size_t(i);
            target[n + 4] = std::clamp(p.eqBands[n], -18.f, 18.f);
            frequencyTarget[n] = std::clamp(p.eqFrequencies[n], 20.f, 20000.f);
            qTarget[n] = std::clamp(p.eqQ[n], .1f, 20.f);
            wetTarget[n] = p.eqEnabled[n] ? 1.f : 0.f;
            int type = std::clamp(p.eqTypes[n], 0, 7);
            if (initial && type != types[n] && typeMix[n] >= 1) {
                for (int ch = 0; ch < 2; ++ch) {
                    previous[size_t(ch)][n] = bands[size_t(ch)][n];
                    bands[size_t(ch)][n] = {};
                }
                oldTypes[n] = types[n];
                types[n] = type;
                typeMix[n] = 0;
            } else if (!initial)
                types[n] = oldTypes[n] = type;
        }
        modeTarget = p.eqMode == 0 ? 0.f : 1.f;
        double compensation = 0;
        if (p.eqAutoGain)
            for (int i = 0; i < 96; ++i) {
                double hz = 20 * std::pow(std::min(20000., sr * .45) / 20, i / 95.);
                compensation = std::max(
                    compensation, response(p, bass, bassFreq, treble, trebleFreq, hz, sr, false));
            }
        gainTarget = float(std::pow(10., (p.eqOutput - compensation) / 20));
        if (!initial) {
            current = target;
            frequency = frequencyTarget;
            q = qTarget;
            wet = wetTarget;
            mode = modeTarget;
            gain = gainTarget;
            initial = true;
        }
        update();
    }
    std::array<float, 2> tick(float l, float r) {
        if (counter++ % 32 == 0) {
            auto smooth = [&](float &a, float b) {
                float d = b - a;
                if (std::abs(d) > 1e-5f)
                    a += coefficient * d;
                else
                    a = b;
            };
            for (size_t i = 0; i < current.size(); ++i)
                smooth(current[i], target[i]);
            for (size_t i = 0; i < frequency.size(); ++i) {
                smooth(frequency[i], frequencyTarget[i]);
                smooth(q[i], qTarget[i]);
            }
            update();
        }
        mode += sampleCoefficient * (modeTarget - mode);
        gain += sampleCoefficient * (gainTarget - gain);
        for (size_t i = 0; i < wet.size(); ++i) {
            wet[i] += sampleCoefficient * (wetTarget[i] - wet[i]);
            if (std::abs(wetTarget[i] - wet[i]) < 1e-6f)
                wet[i] = wetTarget[i];
            typeMix[i] = std::min(1.f, typeMix[i] + float(1 / (sr * .02)));
        }
        std::array<float, 2> out{l, r};
        for (int ch = 0; ch < 2; ++ch) {
            auto c = size_t(ch);
            float a = legacy[c][1].tick(legacy[c][0].tick(out[c])), b = out[c];
            for (size_t i = 0; i < wet.size(); ++i) {
                if (wet[i] == 0 && wetTarget[i] == 0)
                    continue;
                float filtered = bands[c][i].tick(b);
                if (typeMix[i] < 1) {
                    float old = previous[c][i].tick(b);
                    filtered = old + (filtered - old) * typeMix[i];
                }
                b = wet[i] == 1 ? filtered : b + (filtered - b) * wet[i];
            }
            out[c] = (a + (b - a) * mode) * gain;
        }
        return out;
    }
    static double response(const EqualizerParams &p, float bass, float bassFreq, float treble,
                           float trebleFreq, double hz, double sr, bool output = true) {
        double mag = 1;
        Filter f;
        if (p.eqMode == 0) {
            f.set(sr, bassFreq, bass, true);
            mag *= f.magnitude(hz, sr);
            f.set(sr, trebleFreq, treble, true, true);
            mag *= f.magnitude(hz, sr);
        } else
            for (size_t i = 0; i < p.eqBands.size(); ++i)
                if (p.eqEnabled[i]) {
                    f.graphic(sr, std::clamp(p.eqTypes[i], 0, 7), p.eqFrequencies[i], p.eqBands[i],
                              p.eqQ[i]);
                    mag *= f.magnitude(hz, sr);
                }
        return 20 * std::log10(std::max(1e-12, mag)) + (output ? p.eqOutput : 0);
    }
};
} // namespace logisth
