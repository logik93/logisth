#pragma once
#include "vendor/signalsmith-stretch/signalsmith-linear/fft.h"
#include <array>
#include <complex>
#include <cstdint>
#include <vector>

namespace logisth {
inline constexpr float minPhaseStretch = .1f, maxPhaseStretch = 1024.f;
// Magnitude-preserving, random-phase STFT with four-way overlap. Stereo shares
// each phase rotation so inter-channel phase survives the diffusion.
class PhaseDiffuseStretch {
    using Complex = std::complex<float>;
    using FFT = signalsmith::linear::RealFFT<float>;
    static constexpr float tau = 6.283185307179586f;
    FFT fft;
    std::vector<float> window, time;
    std::vector<Complex> analysis, synthesis, phases;
    std::array<std::vector<float>, 2> output;
    size_t at = 0, untilFrame = 0;
    uint32_t rng = 0x1c4471a1;
    double source = 0;

  public:
    explicit PhaseDiffuseStretch(size_t size)
        : fft(FFT::fastSizeAbove(size)), window(fft.size()), time(fft.size()),
          analysis(fft.size() / 2), synthesis(analysis.size()), phases(analysis.size()) {
        for (size_t i = 0; i < window.size(); ++i)
            window[i] = std::sqrt(.5f - .5f * std::cos(tau * float(i) / float(window.size())));
        for (auto &channel : output)
            channel.resize(window.size());
    }

    void reset(double cursor, uint64_t seed) noexcept {
        at = untilFrame = 0;
        source = cursor;
        rng = uint32_t(seed) ^ 0x1c4471a1;
        if (rng == 0)
            rng = 1;
        for (auto &channel : output)
            std::fill(channel.begin(), channel.end(), 0.f);
    }

    template <class Reader>
    std::array<float, 2> tick(const Reader &read, double step, float ratio, float stretch,
                              const std::array<float, 3> &gains) noexcept {
        const size_t size = window.size(), bins = analysis.size(), hop = size / 4;
        if (untilFrame == 0) {
            for (size_t bin = 1; bin < bins; ++bin) {
                rng ^= rng << 13;
                rng ^= rng >> 17;
                rng ^= rng << 5;
                phases[bin] = std::polar(1.f, float(double(rng) / 4294967296.) * tau);
            }
            const float norm = 1 / std::max(1.f, std::sqrt(gains[0] * gains[0] +
                                                        gains[1] * gains[1] + gains[2] * gains[2]));
            for (size_t ch = 0; ch < 2; ++ch) {
                for (size_t i = 0; i < size; ++i)
                    time[i] = read(source + double(i) * step, int(ch)) * window[i];
                fft.fft(time.data(), analysis.data());
                synthesis[0] = {};
                for (size_t bin = 1; bin < bins; ++bin) {
                    Complex value{};
                    for (int octave = 0; octave < 3; ++octave) {
                        if (gains[size_t(octave)] == 0)
                            continue;
                        const float index = std::ldexp(float(bin) / ratio, 1 - octave);
                        const auto lower = size_t(index);
                        if (lower < 1 || lower + 1 >= bins)
                            continue;
                        const auto from = analysis[lower];
                        const auto sample = from + (analysis[lower + 1] - from) * (index - float(lower));
                        value += sample * gains[size_t(octave)];
                    }
                    synthesis[bin] = value * phases[bin];
                }
                fft.ifft(synthesis.data(), time.data());
                auto &channel = output[ch];
                for (size_t i = 0; i < size; ++i)
                    channel[(at + i) % size] += time[i] * window[i] * (norm * .5f / float(size));
            }
            source += double(hop) * step / std::clamp(stretch, minPhaseStretch, maxPhaseStretch);
            untilFrame = hop;
        }
        std::array<float, 2> result{output[0][at], output[1][at]};
        output[0][at] = output[1][at] = 0;
        if (++at == size)
            at = 0;
        --untilFrame;
        return result;
    }
};
} // namespace logisth
