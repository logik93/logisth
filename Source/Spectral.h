#pragma once
#include "Topology.h"
#include "PhaseDiffuseStretch.h"
#include "vendor/signalsmith-stretch/signalsmith-stretch.h"
#include <array>
#include <atomic>
#include <cmath>
#include <memory>
#include <stdexcept>
#include <type_traits>
namespace logisth {
inline constexpr int phaseDiffusionMode = 8;
inline constexpr int minPhaseFftOrder = 8, maxPhaseFftOrder = 18, defaultPhaseFftOrder = 15;
struct StretchControls {
    float rate = 1, semitones = 0, tonalityHz = 0, formant = 0, blockMs = 0;
    bool diffuse = false;
    std::array<float, 3> ratios{0, 1, 0};
    int overlap = 0; // Auto, otherwise the actual overlap ratio (2..16).
    int phaseFftOrder = defaultPhaseFftOrder;
};
struct SpectralConfig {
    float blockMs = 0;
    int overlap = 0;
    bool diffuse = false;
    int phaseFftOrder = defaultPhaseFftOrder;
    static SpectralConfig from(const StretchControls &controls) {
        if (controls.diffuse)
            return {0, 0, true, std::clamp(controls.phaseFftOrder, minPhaseFftOrder, maxPhaseFftOrder)};
        return {std::isfinite(controls.blockMs) ? std::clamp(controls.blockMs, 0.f, 200.f) : 0.f,
                controls.overlap == 0 ? 0 : std::clamp(controls.overlap, 2, 16), controls.diffuse};
    }
    bool operator==(const SpectralConfig &other) const {
        return blockMs == other.blockMs && overlap == other.overlap && diffuse == other.diffuse &&
               phaseFftOrder == other.phaseFftOrder;
    }
    bool operator!=(const SpectralConfig &other) const {
        return !(*this == other);
    }
    int window(double rate) const {
        if (diffuse)
            return 1 << phaseFftOrder;
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
        if (diffuse)
            return size_t(window(rate)) * 64 + 65536;
        size_t fft = 1;
        const size_t frames = size_t(window(rate));
        while (fft < frames)
            fft *= 2;
        return fft * 160 + frames * 32 + 65536;
    }
};
// configure() and its allocations belong exclusively to preparation threads.
class SpectralStream {
    inline static std::atomic<uint64_t> identities{0};
    signalsmith::stretch::SignalsmithStretch<float> core{0x4c677374};
    std::unique_ptr<PhaseDiffuseStretch> diffusion;
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
        if (config.diffuse) {
            diffusion = std::make_unique<PhaseDiffuseStretch>(config.window(rate));
            return;
        }
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
        if (diffusion) {
            if (forceReset || !ready || token != newToken || revision != newRevision) {
                diffusion->reset(cursor, newToken ^ newRevision);
                token = newToken;
                revision = newRevision;
                ready = true;
            }
            return diffusion->tick(read, step, std::clamp(ratio, .00390625f, 256.f), stretch,
                                   controls.ratios);
        }
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
    unsigned phaseMask;
    int voiceCapacity;
    std::array<int, playerCount> capacity{};
    std::array<SpectralConfig, playerCount> configs{};
    std::array<SpectralConfig, playerCount> phaseConfigs{};
    std::array<std::shared_ptr<SpectralStream>, maxPolyphony * playerCount * unisonCapacity * 2>
        cells{};
    std::vector<std::shared_ptr<SpectralStream>> phaseCells;
    static size_t index(int voice, int osc, int unison, int lane) {
        return size_t((((voice * playerCount) + osc) * unisonCapacity + unison) * 2 + lane);
    }
    static size_t estimatedBytes(double rate, unsigned mask,
                                 const std::array<int, playerCount> &unisons,
                                 const std::array<SpectralConfig, playerCount> &configs,
                                 int voices) {
        size_t bytes = 0;
        for (int osc = 0; osc < playerCount; ++osc)
            if (mask & (1u << osc))
                bytes += configs[size_t(osc)].estimatedBytes(rate) * size_t(voices) *
                         size_t(std::clamp(unisons[size_t(osc)], 1, unisonCapacity)) * 2;
        return bytes;
    }
    SpectralBank(double sampleRate, unsigned wanted, std::array<int, playerCount> unisons,
                 const std::shared_ptr<SpectralBank> &old,
                 std::array<SpectralConfig, playerCount> settings = {},
                 int voices = defaultPolyphony, unsigned phaseWanted = 0,
                 std::array<SpectralConfig, playerCount> phaseSettings = {})
        : rate(sampleRate), mask(wanted), phaseMask(phaseWanted),
          voiceCapacity(std::clamp(voices, 1, maxPolyphony)), capacity(unisons), configs(settings),
          phaseConfigs(phaseSettings) {
        if (estimatedBytes(rate, mask, capacity, configs, voiceCapacity) +
                estimatedBytes(rate, phaseMask, capacity, phaseConfigs, voiceCapacity) / 2 > memoryBudget)
            throw std::length_error("Spectral memory budget exceeded");
        if (phaseMask)
            phaseCells.resize(size_t(voiceCapacity) * playerCount * unisonCapacity);
        for (int voice = 0; voice < voiceCapacity; ++voice)
            for (int osc = 0; osc < playerCount; ++osc) {
                if (phaseMask & (1u << osc))
                    for (int u = 0; u < capacity[size_t(osc)]; ++u) {
                        const auto i = index(voice, osc, u, 0) / 2;
                        if (old && old->rate == rate && (old->phaseMask & (1u << osc)) &&
                            old->phaseConfigs[size_t(osc)] == phaseConfigs[size_t(osc)] &&
                            i < old->phaseCells.size() && old->phaseCells[i])
                            phaseCells[i] = old->phaseCells[i];
                        else
                            phaseCells[i] = std::make_shared<SpectralStream>(rate, phaseConfigs[size_t(osc)]);
                    }
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
                 std::array<SpectralConfig, N> settings = {}, int voices = defaultPolyphony)
        : SpectralBank(sampleRate, wanted, expanded(unisons), old, expanded(settings), voices) {}
    SpectralStream *get(int voice, int osc, int unison, int lane) const {
        if (voice < 0 || voice >= voiceCapacity || osc < 0 || osc >= playerCount || unison < 0 ||
            unison >= capacity[size_t(osc)] || lane < 0 || lane > 1)
            return nullptr;
        return cells[index(voice, osc, unison, lane)].get();
    }
    SpectralStream *getPhase(int voice, int osc, int unison) const {
        if (voice < 0 || voice >= voiceCapacity || osc < 0 || osc >= playerCount ||
            unison < 0 || unison >= capacity[size_t(osc)] || !(phaseMask & (1u << osc)))
            return nullptr;
        return phaseCells[index(voice, osc, unison, 0) / 2].get();
    }
};
} // namespace logisth
