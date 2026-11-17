#pragma once
#include "Telemetry.h"
#include <cmath>
#include <complex>
namespace logisth {
// Full-rate stereo-power FFT on the GUI thread. No FFT runs in processBlock.
class EqSpectrum {
  public:
    static constexpr size_t size = 2048, bins = size / 2 + 1;
    static constexpr float floor = -120;
    std::array<std::array<float, bins>, 2> db{};
    static double tiltDb(double hz, int selected) {
        return std::clamp(selected, 0, 4) * 1.5 * std::log2(std::max(20., hz) / 1000.);
    }
    EqSpectrum() {
        for (size_t i = 0; i < size; ++i) {
            window[i] = float(.5 - .5 * std::cos(2 * 3.14159265358979323846 * i / size));
            windowSum += window[i];
        }
        for (auto &channel : db)
            channel.fill(floor);
    }
    bool update(const ScopeBuffer &capture, double rate, int mode) {
        const auto end = capture.count();
        if (mode == 0 || end == last)
            return false;
        const bool first = last == 0 || end < last || mode != lastMode;
        const double elapsed = first ? .05 : std::min(.5, double(end - last) / rate);
        const float release = float(std::exp(-elapsed / .18));
        for (int stream = 0; stream < 2; ++stream) {
            if (!(mode & (1 << stream)))
                continue;
            for (int channel = 0; channel < 2; ++channel) {
                capture.readAt(stream * 2 + channel, input.data(), int(size), end);
                for (size_t i = 0; i < size; ++i)
                    work[i] = {input[i] * window[i], 0};
                fft();
                for (size_t bin = 0; bin < bins; ++bin) {
                    const float scale = (bin == 0 || bin == size / 2 ? 1.f : 2.f) / windowSum;
                    const float energy = std::norm(work[bin]) * scale * scale * .5f;
                    if (channel == 0)
                        incoming[bin] = energy;
                    else
                        incoming[bin] += energy;
                }
            }
            for (size_t bin = 0; bin < bins; ++bin) {
                auto &value = power[size_t(stream)][bin];
                const auto target = incoming[bin];
                value =
                    first ? target : target + (value - target) * (target > value ? .15f : release);
                db[size_t(stream)][bin] = std::max(floor, 10 * std::log10(std::max(1e-12f, value)));
            }
        }
        last = end;
        lastMode = mode;
        return true;
    }

  private:
    std::array<float, size> input{}, window{};
    std::array<float, bins> incoming{};
    std::array<std::array<float, bins>, 2> power{};
    std::array<std::complex<float>, size> work{};
    float windowSum = 0;
    uint64_t last = 0;
    int lastMode = 0;
    void fft() {
        for (size_t i = 1, j = 0; i < size; ++i) {
            size_t bit = size >> 1;
            for (; j & bit; bit >>= 1)
                j ^= bit;
            j ^= bit;
            if (i < j)
                std::swap(work[i], work[j]);
        }
        for (size_t length = 2; length <= size; length <<= 1) {
            const auto angle = float(-2 * 3.14159265358979323846 / length);
            const std::complex<float> step(std::cos(angle), std::sin(angle));
            for (size_t start = 0; start < size; start += length) {
                std::complex<float> phase{1, 0};
                for (size_t i = 0; i < length / 2; ++i) {
                    const auto a = work[start + i], b = work[start + i + length / 2] * phase;
                    work[start + i] = a + b;
                    work[start + i + length / 2] = a - b;
                    phase *= step;
                }
            }
        }
    }
};
} // namespace logisth
