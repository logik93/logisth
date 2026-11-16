#include "ImpulseReverb.h"
#include <algorithm>
#include <cmath>
#include <stdexcept>
namespace logisth {
void ImpulseData::buildPeaks() {
    peak = 0;
    for (size_t ch = 0; ch < 2; ++ch)
        for (size_t bin = 0; bin < 512; ++bin) {
            size_t start = bin * samples[ch].size() / 512,
                   end = std::max(start + 1, (bin + 1) * samples[ch].size() / 512);
            float lo = 0, hi = 0;
            for (size_t at = start; at < std::min(end, samples[ch].size()); ++at) {
                lo = std::min(lo, samples[ch][at]);
                hi = std::max(hi, samples[ch][at]);
            }
            minimum[ch][bin] = lo;
            maximum[ch][bin] = hi;
            peak = std::max(peak, std::max(-lo, hi));
        }
}

std::shared_ptr<const ImpulseData> ImpulseData::factory() {
    static const auto data = [] {
        auto result = std::make_shared<ImpulseData>();
        result->name = "Factory / chamber";
        result->sampleRate = 44100;
        result->channels = 2;
        const int length = 44100;
        uint32_t random = 0x82641413;
        for (auto &channel : result->samples)
            channel.resize(size_t(length));
        for (int ch = 0; ch < 2; ++ch) {
            float low = 0;
            for (int i = 0; i < length; ++i) {
                random ^= random << 13;
                random ^= random >> 17;
                random ^= random << 5;
                float white = float(double(random) / 2147483648. - 1);
                low += .4f * (white - low);
                double time = double(i) / 44100.;
                float onset = float(std::min(1., std::max(0., (time - .012) * 80)));
                result->samples[size_t(ch)][size_t(i)] =
                    .04f * low * onset * float(std::exp(-time * 7));
            }
            for (int tap = 0; tap < 12; ++tap) {
                int at = int((.012 + ch * .0017 + tap * .0063) * 44100);
                result->samples[size_t(ch)][size_t(at)] +=
                    .3f * float(std::exp(-tap * .28)) * (tap % 3 == 1 ? -.7f : 1.f);
            }
        }
        result->buildPeaks();
        return std::shared_ptr<const ImpulseData>(result);
    }();
    return data;
}
PreparedImpulse::PreparedImpulse(const ImpulseData &data, double hostRate)
    : revision(data.revision), rate(hostRate) {
    if (!std::isfinite(rate) || rate < 8000 || rate > 384000)
        throw std::invalid_argument("Unsupported convolution sample rate");
    const size_t frames = data.samples[0].size();
    if (!frames || data.samples[1].size() != frames || frames > impulseMemoryBudget / 8 ||
        double(frames) * rate / data.sampleRate > double(impulseMemoryBudget / 8))
        throw std::length_error("Impulse response exceeds the memory budget at this sample rate");
    juce::AudioBuffer<float> ir(2, int(frames));
    for (int ch = 0; ch < 2; ++ch)
        ir.copyFrom(ch, 0, data.samples[size_t(ch)].data(), int(frames));
    convolution.loadImpulseResponse(
        std::move(ir), data.sampleRate, juce::dsp::Convolution::Stereo::yes,
        juce::dsp::Convolution::Trim::no, juce::dsp::Convolution::Normalise::no);
    convolution.prepare({rate, 512, 2});
    if (convolution.getLatency() != 0)
        throw std::runtime_error("Convolution requires zero latency");
    // Measure the prepared, resampled IR rather than assuming a rate-dependent gain.
    double energies[2]{};
    const int length = convolution.getCurrentIRSize() + 512;
    for (int offset = 0; offset < length; offset += 512) {
        const int n = std::min(512, length - offset);
        buffer.clear();
        if (offset == 0) {
            buffer.setSample(0, 0, 1);
            buffer.setSample(1, 0, 1);
        }
        juce::dsp::AudioBlock<float> block(buffer);
        auto sub = block.getSubBlock(0, size_t(n));
        juce::dsp::ProcessContextReplacing<float> context(sub);
        convolution.process(context);
        for (int ch = 0; ch < 2; ++ch)
            for (int i = 0; i < n; ++i) {
                double x = buffer.getSample(ch, i);
                energies[ch] += x * x;
            }
    }
    double energy = std::max(energies[0], energies[1]);
    if (!std::isfinite(energy) || energy < 1e-24)
        throw std::invalid_argument("The impulse response contains no audible signal");
    normalizationGain = float(std::min(1e6, 1 / std::sqrt(energy)));
    smoothing = float(1 - std::exp(-1 / (rate * .02)));
    reset();
}
void PreparedImpulse::reset() noexcept {
    convolution.reset();
    buffer.clear();
    gainStarted = false;
}
void PreparedImpulse::process(const float *l, const float *r, float *outL, float *outR, int n,
                              bool normalize) noexcept {
    buffer.copyFrom(0, 0, l, n);
    buffer.copyFrom(1, 0, r, n);
    juce::dsp::AudioBlock<float> block(buffer);
    auto sub = block.getSubBlock(0, size_t(n));
    juce::dsp::ProcessContextReplacing<float> context(sub);
    convolution.process(context);
    float target = normalize ? normalizationGain : 1;
    if (!gainStarted) {
        currentGain = target;
        gainStarted = true;
    }
    for (int i = 0; i < n; ++i) {
        currentGain += smoothing * (target - currentGain);
        if (std::abs(target - currentGain) < 1e-6f)
            currentGain = target;
        outL[i] = buffer.getSample(0, i) * currentGain;
        outR[i] = buffer.getSample(1, i) * currentGain;
    }
}
void ImpulseRack::process(const float *l, const float *r, float *outL, float *outR, int n,
                          bool normalize) noexcept {
    auto *next = desired.load();
    if (!started && next && !current) {
        current = next;
        active.store(current);
    }
    started = true;
    if (!fading && next && next != current) {
        previous = current;
        previousMarker.store(previous);
        current = next;
        active.store(current);
        fadePosition = 0;
        fadeLength = std::max(1, int(current->rate * .1));
        fading = true;
    }
    if (current)
        current->process(l, r, nextL.data(), nextR.data(), n, normalize);
    else {
        std::copy_n(l, n, nextL.data());
        std::copy_n(r, n, nextR.data());
    }
    if (fading) {
        if (previous)
            previous->process(l, r, oldL.data(), oldR.data(), n, normalize);
        else {
            std::copy_n(l, n, oldL.data());
            std::copy_n(r, n, oldR.data());
        }
    }
    for (int i = 0; i < n; ++i) {
        float mix = fading ? std::min(1.f, float(fadePosition++) / fadeLength) : 1.f;
        outL[i] = fading ? oldL[size_t(i)] + (nextL[size_t(i)] - oldL[size_t(i)]) * mix
                         : nextL[size_t(i)];
        outR[i] = fading ? oldR[size_t(i)] + (nextR[size_t(i)] - oldR[size_t(i)]) * mix
                         : nextR[size_t(i)];
    }
    if (fading && fadePosition >= fadeLength) {
        previous = nullptr;
        previousMarker.store(nullptr);
        fading = false;
    }
}
void ImpulseRack::reset() noexcept {
    if (current)
        current->reset();
    if (previous && previous != current)
        previous->reset();
    previous = nullptr;
    previousMarker.store(nullptr);
    fading = false;
    started = false;
    fadePosition = 0;
}
void ImpulseRack::detach() noexcept {
    desired.store(nullptr);
    active.store(nullptr);
    previousMarker.store(nullptr);
    current = previous = nullptr;
    fading = false;
    started = false;
    fadePosition = 0;
}
} // namespace logisth
