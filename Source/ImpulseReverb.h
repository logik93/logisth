#pragma once
#include "ConvolutionInterface.h"
#include <atomic>
#include <juce_dsp/juce_dsp.h>
#include <memory>
#include <vector>
namespace logisth {
inline constexpr uint64_t impulseMemoryBudget = 32ull * 1024 * 1024;
struct ImpulseData {
    static uint64_t nextRevision() {
        static std::atomic<uint64_t> sequence{0};
        return ++sequence;
    }
    uint64_t revision = nextRevision();
    double sampleRate = 44100;
    juce::String name;
    int channels = 2;
    std::array<std::vector<float>, 2> samples;
    static std::shared_ptr<const ImpulseData> factory();
};
class PreparedImpulse {
    juce::dsp::Convolution convolution{juce::dsp::Convolution::NonUniform{256}};
    juce::AudioBuffer<float> buffer{2, 512};
    float currentGain = 1;
    bool gainStarted = false;
    float smoothing = 0;

  public:
    const uint64_t revision;
    const double rate;
    float normalizationGain = 1;
    PreparedImpulse(const ImpulseData &, double hostRate);
    void process(const float *, const float *, float *, float *, int, bool) noexcept;
    void reset() noexcept;
    int latency() const {
        return convolution.getLatency();
    }
};
// Stable audio adapter. Prepared engines are published/retired outside processing.
class ImpulseRack final : public ImpulseProcessor {
    std::atomic<PreparedImpulse *> desired{nullptr}, active{nullptr}, previousMarker{nullptr};
    PreparedImpulse *current = nullptr, *previous = nullptr;
    std::array<float, 512> nextL{}, nextR{}, oldL{}, oldR{};
    int fadePosition = 0, fadeLength = 1;
    bool fading = false, started = false;

  public:
    void publish(PreparedImpulse *next) noexcept {
        desired.store(next);
    }
    bool referenced(PreparedImpulse *p) const noexcept {
        return p == desired.load() || p == active.load() || p == previousMarker.load();
    }
    PreparedImpulse *wanted() const noexcept {
        return desired.load();
    }
    void process(const float *, const float *, float *, float *, int, bool) noexcept override;
    void reset() noexcept override;
    void detach() noexcept;
};
} // namespace logisth
