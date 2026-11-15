#pragma once
#include "Topology.h"
#include "vendor/signalsmith-stretch/signalsmith-stretch.h"
#include <array>
#include <atomic>
#include <cmath>
#include <memory>
#include <stdexcept>
#include <type_traits>
namespace logisth {
struct StretchControls {
    float rate = 1, semitones = 0, tonalityHz = 0, formant = 0, blockMs = 0;
    int overlap = 0; // Auto, otherwise the actual overlap ratio (2..16).
};
struct SpectralConfig {
    float blockMs = 0;
    int overlap = 0;
    static SpectralConfig from(const StretchControls &controls) {
        return {std::isfinite(controls.blockMs) ? std::clamp(controls.blockMs, 0.f, 200.f) : 0.f,
                controls.overlap == 0 ? 0 : std::clamp(controls.overlap, 2, 16)};
    }
    bool operator==(const SpectralConfig &other) const {
        return blockMs == other.blockMs && overlap == other.overlap;
    }
    bool operator!=(const SpectralConfig &other) const {
        return !(*this == other);
    }
    int window(double rate) const {
        if (blockMs > 0)
            return std::max(32, int(std::round(rate * blockMs * .001)));
        int samples = 1024;
        while (samples < rate * .08)
            samples *= 2;
        return samples;
    }
    int hop(double rate) const {
        return std::max(1, window(rate) / (overlap ? overlap : 8));
    }
    size_t estimatedBytes(double rate) const {
        size_t fft = 1;
        while (fft < size_t(window(rate)))
            fft *= 2;
        return fft * 160 + size_t(window(rate)) * 32 + 65536;
    }
};
// configure() and its allocations belong exclusively to preparation threads.
class SpectralStream {
    inline static std::atomic<uint64_t> identities{0};
    signalsmith::stretch::SignalsmithStretch<float> core{0x4c677374};
    std::array<std::vector<float>, 2> input;
    std::array<std::array<float, 64>, 2> output{};
    uint64_t token = 0, revision = 0;
    double inputPosition = 0, remainder = 0;
    int outputAt = 64;
    bool ready = false;
    void tuning(float ratio, const StretchControls &controls) {
        const float limit = std::isfinite(controls.tonalityHz)
                                ? std::clamp(controls.tonalityHz, 0.f, float(rate * .49))
                                : 0;
        core.setTransposeFactor(std::clamp(ratio, .00390625f, 256.f), limit / float(rate));
        core.setFormantSemitones(
            std::isfinite(controls.formant) ? std::clamp(controls.formant, -24.f, 24.f) : 0.f);
    }

  public:
    const uint64_t identity = ++identities;
    const double rate;
    const SpectralConfig config;
    explicit SpectralStream(double hostRate, SpectralConfig settings = {})
        : rate(std::isfinite(hostRate) ? std::clamp(hostRate, 8000., 384000.) : 48000.),
          config(settings) {
        core.configure(2, config.window(rate), config.hop(rate));
        for (auto &channel : input)
            channel.resize(size_t(std::max(config.window(rate), 4096)));
    }
    bool matches(double hostRate, const StretchControls &controls) const {
        return rate == hostRate && config == SpectralConfig::from(controls);
    }
    template <class Read>
    std::array<float, 2> tick(Read read, double cursor, double step, float ratio, float stretch,
                              uint64_t newToken, uint64_t newRevision, bool forceReset = false,
                              const StretchControls &controls = {}) {
        double speed = 1 / std::clamp(double(stretch), 1. / 64, 64.);
        float *in[]{input[0].data(), input[1].data()}, *out[]{output[0].data(), output[1].data()};
        if (forceReset || !ready || token != newToken || revision != newRevision) {
            tuning(ratio, controls);
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
            tuning(ratio, controls);
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
    static constexpr size_t memoryBudget = 512ull * 1024 * 1024;
    double rate;
    unsigned mask;
    int voiceCapacity;
    std::array<int, playerCount> capacity{};
    std::array<SpectralConfig, playerCount> configs{};
    std::array<std::shared_ptr<SpectralStream>, 16 * playerCount * 8 * 2> cells{};
    static size_t index(int voice, int osc, int unison, int lane) {
        return size_t((((voice * playerCount) + osc) * 8 + unison) * 2 + lane);
    }
    static size_t estimatedBytes(double rate, unsigned mask,
                                 const std::array<int, playerCount> &unisons,
                                 const std::array<SpectralConfig, playerCount> &configs,
                                 int voices) {
        size_t bytes = 0;
        for (int osc = 0; osc < playerCount; ++osc)
            if (mask & (1u << osc))
                bytes += configs[size_t(osc)].estimatedBytes(rate) * size_t(voices) *
                         size_t(std::clamp(unisons[size_t(osc)], 1, 8)) * 2;
        return bytes;
    }
    SpectralBank(double sampleRate, unsigned wanted, std::array<int, playerCount> unisons,
                 const std::shared_ptr<SpectralBank> &old,
                 std::array<SpectralConfig, playerCount> settings = {}, int voices = 16)
        : rate(sampleRate), mask(wanted), voiceCapacity(std::clamp(voices, 1, 16)),
          capacity(unisons), configs(settings) {
        if (estimatedBytes(rate, mask, capacity, configs, voiceCapacity) > memoryBudget)
            throw std::length_error("Spectral memory budget exceeded");
        for (int voice = 0; voice < voiceCapacity; ++voice)
            for (int osc = 0; osc < playerCount; ++osc)
                if (mask & (1u << osc))
                    for (int u = 0; u < capacity[size_t(osc)]; ++u)
                        for (int lane = 0; lane < 2; ++lane) {
                            size_t i = index(voice, osc, u, lane);
                            if (old && old->rate == rate &&
                                old->configs[size_t(osc)] == configs[size_t(osc)] && old->cells[i])
                                cells[i] = old->cells[i];
                            else
                                cells[i] =
                                    std::make_shared<SpectralStream>(rate, configs[size_t(osc)]);
                        }
    }
    template <class T, size_t N>
    static std::array<T, playerCount> expanded(const std::array<T, N> &values) {
        static_assert(N <= size_t(playerCount));
        std::array<T, playerCount> result{};
        std::copy(values.begin(), values.end(), result.begin());
        return result;
    }
    template <size_t N, std::enable_if_t<N != size_t(playerCount), int> = 0>
    SpectralBank(double sampleRate, unsigned wanted, std::array<int, N> unisons,
                 const std::shared_ptr<SpectralBank> &old,
                 std::array<SpectralConfig, N> settings = {}, int voices = 16)
        : SpectralBank(sampleRate, wanted, expanded(unisons), old, expanded(settings), voices) {}
    SpectralStream *get(int voice, int osc, int unison, int lane) const {
        if (voice < 0 || voice >= voiceCapacity || osc < 0 || osc >= playerCount || unison < 0 ||
            unison >= capacity[size_t(osc)] || lane < 0 || lane > 1)
            return nullptr;
        return cells[index(voice, osc, unison, lane)].get();
    }
};
} // namespace logisth
