#pragma once
#include "vendor/signalsmith-stretch/signalsmith-stretch.h"
#include <array>
#include <memory>
namespace logisth {
// Preconfigured on a control thread. process()/seek()/reset() retain their buffers.
class SpectralStream {
    signalsmith::stretch::SignalsmithStretch<float> core{0x4c677374};
    std::array<std::vector<float>, 2> input;
    std::array<std::array<float, 64>, 2> output{};
    uint64_t token = 0, revision = 0;
    double inputPosition = 0, remainder = 0;
    int outputAt = 64;
    bool ready = false;

  public:
    explicit SpectralStream(double rate) {
        rate = std::isfinite(rate) ? std::clamp(rate, 8000., 384000.) : 48000.;
        int window = 1024;
        while (window < rate * .08)
            window *= 2;
        core.configure(2, window, window / 8);
        for (auto &channel : input)
            channel.resize(size_t(std::max(window, 512)));
    }
    template <class Read>
    std::array<float, 2> tick(Read read, double cursor, double step, float ratio, float stretch,
                              uint64_t newToken, uint64_t newRevision, bool forceReset = false) {
        core.setTransposeFactor(std::clamp(ratio, .00390625f, 256.f));
        double speed = 1 / std::clamp(double(stretch), .125, 8.);
        float *in[]{input[0].data(), input[1].data()}, *out[]{output[0].data(), output[1].data()};
        if (forceReset || !ready || token != newToken || revision != newRevision) {
            core.reset();
            ready = true;
            token = newToken;
            revision = newRevision;
            remainder = 0;
            inputPosition = cursor + (core.outputLatency() * speed - core.inputLatency()) * step;
            for (int i = 0; i < core.blockSamples(); ++i) {
                for (int ch = 0; ch < 2; ++ch)
                    input[size_t(ch)][size_t(i)] = read(inputPosition, ch);
                inputPosition += step;
            }
            core.seek(in, core.blockSamples(), speed);
            outputAt = 64;
        }
        if (outputAt == 64) {
            double wanted = 64 * speed + remainder;
            int count = int(std::floor(wanted));
            remainder = wanted - count;
            for (int i = 0; i < count; ++i) {
                for (int ch = 0; ch < 2; ++ch)
                    input[size_t(ch)][size_t(i)] = read(inputPosition, ch);
                inputPosition += step;
            }
            core.process(in, count, out, 64);
            outputAt = 0;
        }
        std::array<float, 2> result{output[0][size_t(outputAt)], output[1][size_t(outputAt)]};
        ++outputAt;
        return result;
    }
};
struct SpectralBank {
    double rate;
    unsigned mask;
    std::array<int, 4> capacity{};
    std::array<std::shared_ptr<SpectralStream>, 1024> cells{};
    static size_t index(int voice, int osc, int unison, int lane) {
        return size_t((((voice * 4) + osc) * 8 + unison) * 2 + lane);
    }
    SpectralBank(double sampleRate, unsigned wanted, std::array<int, 4> unisons,
                 const std::shared_ptr<SpectralBank> &old)
        : rate(sampleRate), mask(wanted), capacity(unisons) {
        for (int voice = 0; voice < 16; ++voice)
            for (int osc = 0; osc < 4; ++osc)
                if (mask & (1u << osc))
                    for (int u = 0; u < capacity[size_t(osc)]; ++u)
                        for (int lane = 0; lane < 2; ++lane) {
                            size_t i = index(voice, osc, u, lane);
                            if (old && old->rate == rate && old->cells[i])
                                cells[i] = old->cells[i];
                            else
                                cells[i] = std::make_shared<SpectralStream>(rate);
                        }
    }
    SpectralStream *get(int voice, int osc, int unison, int lane) const {
        return cells[index(voice, osc, unison, lane)].get();
    }
};
} // namespace logisth
