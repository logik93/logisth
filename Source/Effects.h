#pragma once
#include "ConvolutionInterface.h"
#include "Dynamics.h"
#include "EffectOrder.h"
#include "Equalizer.h"
#include "Flanger.h"
#include "Modulation.h"
#include "MultibandDistortion.h"
#include "Reverb.h"
#include <vector>
namespace logisth {
struct EffectsParams : ReverbControls, EqualizerParams {
    EffectOrder order;
    FlangerControls flange;
    DistortionOrder distortionOrder;
    bool firstDistortionEnabled = true;
    std::array<DistortionSettings, 3> extraDistortions{};
    ScopeBuffer *eqTrace = nullptr, *distortionTrace = nullptr;
    bool distortionMultiband = false;
    int distortionBands = 4;
    std::array<float, 3> distortionCrossovers{200, 2000, 8000};
    std::array<DistortionSettings, 4> distortionSettings() const noexcept {
        std::array<DistortionSettings, 4> result;
        result[0] = {firstDistortionEnabled, distortionType, drive, distMix};
        std::copy(extraDistortions.begin(), extraDistortions.end(), result.begin() + 1);
        return result;
    }
    ImpulseProcessor *convolution = nullptr;
    bool irNormalize = true;
    float reverbOutput = 0, impulseSeconds = 1;
    bool modernCompressor = true;
    float compKnee = 6, compMakeup = 0, compMix = 1;
    bool distortion = false, phaser = false, chorus = false, eq = false, reverb = true,
         delay = false, compressor = false, sync = true, dual = false, pingpong = false;
    int distortionType = 0, phaserDivision = 12, chorusDivision = 12, delayDivisionL = 8,
        delayDivisionR = 7;
    double bpm = 120;
    float drive = 0, distMix = 1, phaserFreq = 800, phaserSpread = .5f, phaserOffset = .2f,
          phaserWidth = 1, phaserRate = .3f, phaserGain = .5f, phaserFeedback = .3f,
          phaserMix = .5f;
    float chorusDelay = 12, chorusDepth = .25f, chorusRate = .35f, chorusFeedback = .1f,
          chorusWidth = 1, chorusMix = .25f;
    float bass = 0, bassFreq = 200, treble = 0, trebleFreq = 6000, reverbSize = .65f,
          reverbDamp = .5f, reverbPredelay = 0, reverbWidth = 1, reverbMix = .12f;
    float delayL = .375f, delayR = .5f, feedback = .35f, delaySpread = 1, delayWidth = 1,
          lowCut = 20, highCut = 18000, smear = 0, delayMix = .2f;
    float ratio = 4, threshold = -18, attack = 10, release = 150;
    bool activeDistortion() const noexcept {
        if (!distortion)
            return false;
        if (distortionMultiband)
            return MultibandDistortionRack::active(distortionSettings(), distortionOrder,
                                                   distortionBands);
        if (firstDistortionEnabled && distMix > 0)
            return true;
        for (const auto &stage : extraDistortions)
            if (stage.enabled && stage.mix > 0)
                return true;
        return false;
    }
};
struct ClipperStage {
    Clipper &processor;
    const ClipperSettings &settings;
    DynamicsMeters &meter;
    ScopeBuffer *trace = nullptr;
    void process(float *l, float *r, int n) const noexcept {
        processor.process(l, r, n, settings, meter, trace);
    }
};
class Effects {
    double sr = 44100, phase = 0, chorusPhase = 0;
    size_t delayPos = 0, chorusPos = 0, prePos = 0;
    std::array<std::vector<float>, 2> delayLines, preLines;
    std::array<std::vector<float>, 4> chorusLines;
    std::array<std::array<float, 6>, 2> allpass{};
    std::array<float, 2> phaserLast{}, lowState{}, highState{};
    MultibandDistortionRack distortionRack;
    std::array<std::array<float, 3>, 2> smearState{};
    Equalizer equalizer;
    Flanger flanger;
    Compressor modern;
    bool compWasEnabled = false, compWasModern = true, sleeping = true;
    uint64_t quietInput = 0, quietOutput = 0;
    std::array<float, 512> wetL{}, wetR{}, distortionBeforeL{}, distortionBeforeR{};
    std::array<CharacterReverb, 5> reverbModels;
    std::array<float, 6> reverbWeights{};
    std::array<float, 512> convolutionL{}, convolutionR{};
    float reverbGain = 1;
    std::array<float, 512> reverbInputL{}, reverbInputR{};
    bool reverbStarted = false, initialProcess = true;
    float detector = 0, delayMix = 0, reverbMix = 0;
    static float read(const std::vector<float> &data, size_t pos, double samples) {
        double at = double(pos) - samples;
        while (at < 0)
            at += data.size();
        const auto a = size_t(at) % data.size();
        const auto b = (a + 1) % data.size();
        float frac = float(at - std::floor(at));
        return data[a] + frac * (data[b] - data[a]);
    }
    static void width(float &l, float &r, float width) {
        const float mid = (l + r) * .5f, side = (l - r) * .5f * width;
        l = mid + side;
        r = mid - side;
    }

    void distort(float *l, float *r, int n, const EffectsParams &p, const float *driveMod,
                 float smooth) {
        const auto settings = p.distortionSettings();
        if (p.distortionTrace) {
            std::copy_n(l, n, distortionBeforeL.begin());
            std::copy_n(r, n, distortionBeforeR.begin());
        }
        if (p.distortion && (p.activeDistortion() || distortionRack.hasTail()))
            distortionRack.process(l, r, n, settings, p.distortionOrder, driveMod, smooth,
                                   p.distortionMultiband, p.distortionBands,
                                   p.distortionCrossovers);
        if (p.distortionTrace) {
            for (int i = 0; i < n; ++i)
                p.distortionTrace->push(distortionBeforeL[size_t(i)], distortionBeforeR[size_t(i)],
                                        l[i], r[i]);
            p.distortionTrace->commit();
        }
    }
    void phaseShift(float *l, float *r, int n, const EffectsParams &p, const float *phaserMod) {
        const double phaserHz = p.sync ? p.bpm / (60 * beatLength(p.phaserDivision)) : p.phaserRate;
        for (int s = 0; s < n; ++s) {
            float x[2]{l[s], r[s]};

            float wet[2]{};
            for (int ch = 0; ch < 2; ++ch) {
                float sample = x[ch] + phaserLast[ch] * p.phaserFeedback;
                for (int stage = 0; stage < 6; ++stage) {
                    double exponent =
                        (stage - 2.5) * p.phaserSpread * .8 + (ch == 0 ? -1 : 1) * p.phaserOffset +
                        std::sin(2 * pi * phase + ch * p.phaserWidth * pi * .5) * p.phaserGain * 2 +
                        phaserMod[s] * 10;
                    float freq =
                        clamp(float(p.phaserFreq * std::exp2(exponent)), 20, float(sr * .43));
                    float g = std::tan(float(pi) * freq / float(sr)), a = (1 - g) / (1 + g);
                    float out = -a * sample + allpass[ch][stage];
                    allpass[ch][stage] = sample + a * out;
                    sample = out;
                }
                phaserLast[ch] = std::tanh(sample);
                wet[ch] = sample;
            }
            width(wet[0], wet[1], p.phaserWidth);
            for (int ch = 0; ch < 2; ++ch)
                x[ch] += (wet[ch] - x[ch]) * p.phaserMix;
            phase += phaserHz / sr;
            phase -= std::floor(phase);

            l[s] = x[0];
            r[s] = x[1];
        }
    }
    void chorus(float *l, float *r, int n, const EffectsParams &p) {
        const double chorusHz = p.sync ? p.bpm / (60 * beatLength(p.chorusDivision)) : p.chorusRate;
        for (int s = 0; s < n; ++s) {
            float x[2]{l[s], r[s]};

            float wet[2]{};
            for (int stage = 0; stage < (p.dual ? 2 : 1); ++stage)
                for (int ch = 0; ch < 2; ++ch) {
                    const int line = 2 * stage + ch;
                    const double angle = 2 * pi * chorusPhase + line * pi * .5;
                    const double time = std::clamp(double(p.chorusDelay) *
                                                       (1 + .8 * p.chorusDepth * std::sin(angle)),
                                                   .1, 60.);
                    float echo = read(chorusLines[line], chorusPos, time * .001 * sr);
                    chorusLines[line][chorusPos] = x[ch] + echo * p.chorusFeedback;
                    wet[ch] += echo / (p.dual ? 2 : 1);
                }
            width(wet[0], wet[1], p.chorusWidth);
            for (int ch = 0; ch < 2; ++ch)
                x[ch] += (wet[ch] - x[ch]) * p.chorusMix;
            chorusPos = (chorusPos + 1) % chorusLines[0].size();
            chorusPhase += chorusHz / sr;
            chorusPhase -= std::floor(chorusPhase);

            l[s] = x[0];
            r[s] = x[1];
        }
    }
    void equalize(float *l, float *r, int n, const EffectsParams &p) {
        if (p.eq)
            equalizer.configure(p, p.bass, p.bassFreq, p.treble, p.trebleFreq);
        for (int s = 0; s < n; ++s) {
            const auto beforeL = l[s], beforeR = r[s];
            auto filtered = p.eq ? equalizer.tick(l[s], r[s]) : std::array<float, 2>{l[s], r[s]};
            l[s] = filtered[0];
            r[s] = filtered[1];
            if (p.eqTrace)
                p.eqTrace->push(beforeL, beforeR, l[s], r[s]);
        }
        if (p.eqTrace)
            p.eqTrace->commit();
    }
    void reverberate(float *l, float *r, int n, const EffectsParams &p, float smooth) {
        for (int s = 0; s < n; ++s) {
            const float x[2]{l[s], r[s]};

            for (int ch = 0; ch < 2; ++ch) {
                preLines[ch][prePos] = x[ch];
                float pre = p.reverbPredelay <= 0
                                ? x[ch]
                                : read(preLines[ch], prePos, p.reverbPredelay * .001 * sr);
                (ch == 0 ? wetL : wetR)[s] = pre;
            }
            prePos = (prePos + 1) % preLines[0].size();
        }
        if (p.reverb) {
            std::copy_n(wetL.begin(), n, reverbInputL.begin());
            std::copy_n(wetR.begin(), n, reverbInputR.begin());
            int selected = std::clamp(p.reverbType, 0, 5);
            if (!reverbStarted) {
                reverbWeights[size_t(selected)] = 1;
                reverbStarted = true;
            }
            for (size_t i = 0; i < 5; ++i)
                if (int(i) == selected || reverbWeights[i] > 0)
                    reverbModels[i].configure(p, p.reverbSize, p.reverbDamp);
            if (selected == 5 || reverbWeights[5] > 0) {
                if (p.convolution)
                    p.convolution->process(reverbInputL.data(), reverbInputR.data(),
                                           convolutionL.data(), convolutionR.data(), n,
                                           p.irNormalize);
                else {
                    std::copy_n(reverbInputL.data(), n, convolutionL.data());
                    std::copy_n(reverbInputR.data(), n, convolutionR.data());
                }
            }
            // Calibrated outside the feedback loops: approximately -1 dB full-wet
            // relative to mono pink noise at the documented default settings.
            static constexpr std::array<float, 5> levels{3.589219346f, 1.819700859f, 3.273406949f,
                                                         3.311311215f, 4.365158322f};
            const float outputTarget = std::pow(10.f, clamp(p.reverbOutput, -24, 24) / 20);
            const float fade = float(1 / (.1 * sr));
            for (int s = 0; s < n; ++s) {
                float total = 0, wet[2]{};
                for (int i = 0; i < 6; ++i) {
                    float &weight = reverbWeights[size_t(i)];
                    float old = weight;
                    weight =
                        i == selected ? std::min(1.f, weight + fade) : std::max(0.f, weight - fade);
                    total += weight;
                    if (weight > 0) {
                        auto sample = i == 5
                                          ? std::array<float, 2>{convolutionL[size_t(s)],
                                                                 convolutionR[size_t(s)]}
                                          : reverbModels[size_t(i)].tick(reverbInputL[size_t(s)],
                                                                         reverbInputR[size_t(s)]);
                        if (i < 5) {
                            sample[0] *= levels[size_t(i)];
                            sample[1] *= levels[size_t(i)];
                        }
                        width(sample[0], sample[1], p.reverbWidth);
                        wet[0] += sample[0] * weight;
                        wet[1] += sample[1] * weight;
                    } else if (old > 0 && i < 5)
                        reverbModels[size_t(i)].reset();
                }
                reverbMix += smooth * (p.reverbMix - reverbMix);
                if (std::abs(p.reverbMix - reverbMix) < 1e-6f)
                    reverbMix = p.reverbMix;
                reverbGain += smooth * (outputTarget - reverbGain);
                const float dryGain = reverbMix >= 1 ? 0.f : float(std::cos(reverbMix * pi * .5));
                const float wetGain =
                    reverbMix <= 0 ? 0.f : float(std::sin(reverbMix * pi * .5)) * reverbGain;
                l[s] = l[s] * dryGain + wet[0] / std::max(.0001f, total) * wetGain;
                r[s] = r[s] * dryGain + wet[1] / std::max(.0001f, total) * wetGain;
            }
        }
    }
    void delay(float *l, float *r, int n, const EffectsParams &p, float smooth) {
        const double dl =
            std::clamp((p.sync ? 60 / p.bpm * beatLength(p.delayDivisionL) : p.delayL) * sr, 1.,
                       double(delayLines[0].size() - 2));
        const double dr =
            std::clamp((p.sync ? 60 / p.bpm * beatLength(p.delayDivisionR) : p.delayR) * sr, 1.,
                       double(delayLines[0].size() - 2));
        const float low = float(1 - std::exp(-2 * pi * p.lowCut / sr)),
                    high =
                        float(1 - std::exp(-2 * pi * std::min(double(p.highCut), sr * .43) / sr));

        for (int s = 0; s < n; ++s) {
            float x[2]{l[s], r[s]};

            float echo[2]{read(delayLines[0], delayPos, dl), read(delayLines[1], delayPos, dr)};
            for (int ch = 0; ch < 2; ++ch) {
                highState[ch] += high * (echo[ch] - highState[ch]);
                lowState[ch] += low * (highState[ch] - lowState[ch]);
                echo[ch] = highState[ch] - lowState[ch];
                for (int stage = 0; stage < 3; ++stage) {
                    float a = p.smear * .65f;
                    float y = -a * echo[ch] + smearState[ch][stage];
                    smearState[ch][stage] = echo[ch] + a * y;
                    echo[ch] = y;
                }
            }
            const float mono = (x[0] + x[1]) * .5f;
            for (int ch = 0; ch < 2; ++ch) {
                float input = p.pingpong ? mono * (ch == 0 ? 1.f : 1 - p.delaySpread) : x[ch];
                float feedback = p.pingpong ? echo[1 - ch] : echo[ch];
                delayLines[ch][delayPos] = input + feedback * p.feedback;
            }
            width(echo[0], echo[1], p.delayWidth);
            delayMix += smooth * (p.delayMix - delayMix);
            for (int ch = 0; ch < 2; ++ch)
                x[ch] += (echo[ch] - x[ch]) * delayMix;
            delayPos = (delayPos + 1) % delayLines[0].size();

            l[s] = x[0];
            r[s] = x[1];
        }
    }
    void compress(float *l, float *r, int n, const EffectsParams &p, DynamicsMeters *meter,
                  ScopeBuffer *trace) {
        float compIn = 0, compOut = 0, compReduction = 0;
        const float attack = float(std::exp(-1 / (p.attack * .001 * sr))),
                    release = float(std::exp(-1 / (p.release * .001 * sr)));
        for (int s = 0; s < n; ++s) {
            float x[2]{l[s], r[s]};
            const float before = std::abs(x[0]) >= std::abs(x[1]) ? x[0] : x[1];
            if (p.compressor && p.modernCompressor)
                modern.tick(x[0], x[1]);
            if (p.compressor && !p.modernCompressor) {
                float level = std::max(std::abs(x[0]), std::abs(x[1]));
                float c = level > detector ? attack : release;
                detector = level + c * (detector - level);
                float db = 20 * std::log10(std::max(1e-8f, detector));
                float reduction = std::max(0.f, db - p.threshold) * (1 - 1 / p.ratio);
                float gain = std::pow(10.f, -reduction / 20);
                gain *= amplitude(p.compMakeup);
                if (p.compMix != 1)
                    gain = 1 + p.compMix * (gain - 1);
                compReduction = std::max(compReduction, reduction);
                x[0] *= gain;
                x[1] *= gain;
            }
            compIn = std::max(compIn, std::abs(before));
            compOut = std::max(compOut, std::max(std::abs(x[0]), std::abs(x[1])));
            if (trace && s % 4 == 0)
                trace->push(before, std::abs(x[0]) >= std::abs(x[1]) ? x[0] : x[1],
                            p.modernCompressor ? modern.maximumReduction : compReduction);
            l[s] = x[0];
            r[s] = x[1];
        }
        if (meter && p.compressor)
            meter->write(compIn, compOut,
                         p.modernCompressor ? modern.maximumReduction : compReduction);
        if (trace)
            trace->commit();
    }

  public:
    uint64_t processedSamples = 0;
    void prepare(double sampleRate) {
        sr = sampleRate;
        for (auto &b : delayLines)
            b.assign(size_t(sr * 16) + 4, 0);
        for (auto &b : preLines)
            b.assign(size_t(sr * .21) + 4, 0);
        for (auto &b : chorusLines)
            b.assign(size_t(sr * .1) + 4, 0);
        for (int i = 0; i < 5; ++i)
            reverbModels[size_t(i)].prepare(sr, i);
        equalizer.prepare(sr);
        flanger.prepare(sr);
        distortionRack.prepare(sr);
        reset();
    }
    void reset() {
        for (auto *buffers : {&delayLines, &preLines})
            for (auto &b : *buffers)
                std::fill(b.begin(), b.end(), 0);
        for (auto &b : chorusLines)
            std::fill(b.begin(), b.end(), 0);
        phase = chorusPhase = 0;
        delayPos = chorusPos = prePos = 0;
        allpass = {};
        phaserLast = lowState = highState = {};
        distortionRack.reset();
        smearState = {};
        equalizer.reset();
        flanger.reset();
        modern.reset();
        compWasEnabled = false;
        sleeping = true;
        quietInput = quietOutput = 0;
        for (auto &model : reverbModels)
            model.reset();
        reverbWeights = {};
        reverbStarted = false;
        reverbGain = 1;
        initialProcess = true;
        detector = delayMix = reverbMix = 0;
    }
    void process(float *l, float *r, int n, const float *driveMod, const float *phaserMod,
                 const EffectsParams &p, DynamicsMeters *meter = nullptr,
                 ScopeBuffer *trace = nullptr, const ClipperStage *clip = nullptr) {
        processedSamples = 0;
        if (compWasEnabled != p.compressor || compWasModern != p.modernCompressor) {
            detector = 0;
            modern.reset();
            compWasEnabled = p.compressor;
            compWasModern = p.modernCompressor;
        }
        if (meter && !p.compressor)
            meter->write(0, 0, 0);
        if (initialProcess) {
            if (p.reverb) {
                reverbMix = clamp(p.reverbMix, 0, 1);
                reverbGain = std::pow(10.f, clamp(p.reverbOutput, -24, 24) / 20);
            }
            initialProcess = false;
        }
        if (!p.reverb && reverbStarted) {
            for (auto &line : preLines)
                std::fill(line.begin(), line.end(), 0);
            prePos = 0;
            for (auto &model : reverbModels)
                model.reset();
            if (p.convolution)
                p.convolution->reset();
            reverbWeights = {};
            reverbStarted = false;
            reverbMix = 0;
        }
        const bool clipEnabled = clip && clip->settings.enabled && clip->settings.mix > 0;
        if (clip && !clipEnabled)
            clip->process(l, r, n);
        if (!p.distortion)
            distortionRack.silence(p.distortionMultiband, p.distortionBands, p.distortionOrder,
                                   p.distortionSettings());
        else if (!p.activeDistortion() && !distortionRack.hasTail())
            distortionRack.synchronize(p.distortionMultiband, p.distortionBands, p.distortionOrder,
                                       p.distortionSettings());
        const bool any = clipEnabled || p.eqTrace || p.distortionTrace ||
                         distortionRack.hasTail() || p.activeDistortion() || p.phaser || p.chorus ||
                         flanger.active(p.flange) ||
                         p.eq || p.reverb || p.delay || p.compressor;
        if (!any)
            return;
        float inPeak = 0;
        for (int i = 0; i < n; ++i)
            inPeak = std::max(inPeak, std::max(std::abs(l[i]), std::abs(r[i])));
        if (p.compressor && p.modernCompressor)
            modern.configure(
                {p.threshold, p.ratio, p.compKnee, p.attack, p.release, p.compMakeup, p.compMix},
                sr);
        if (sleeping && inPeak < 1e-10f) {
            distortionRack.silence(p.distortionMultiband, p.distortionBands, p.distortionOrder,
                                   p.distortionSettings());
            if (p.distortionTrace) {
                for (int i = 0; i < n; ++i)
                    p.distortionTrace->push(0, 0, 0, 0);
                p.distortionTrace->commit();
            }
            if (p.eqTrace) {
                for (int i = 0; i < n; ++i)
                    p.eqTrace->push(0, 0, 0, 0);
                p.eqTrace->commit();
            }
            if (p.phaser) {
                double hz = p.sync ? p.bpm / (60 * beatLength(p.phaserDivision)) : p.phaserRate;
                phase += hz * n / sr;
                phase -= std::floor(phase);
            }
            if (p.chorus) {
                double hz = p.sync ? p.bpm / (60 * beatLength(p.chorusDivision)) : p.chorusRate;
                chorusPhase += hz * n / sr;
                chorusPhase -= std::floor(chorusPhase);
            }
            if (p.flange.enabled)
                flanger.advance(n, p.flange.rate);
            float reduction = 0;
            if (p.compressor) {
                if (p.modernCompressor)
                    reduction = modern.advanceSilence(n);
                else {
                    detector *= float(std::exp(-n / (std::max(1.f, p.release) * .001 * sr)));
                    reduction = compressionReduction(decibels(detector), p.threshold, p.ratio, 0);
                }
            }
            if (meter)
                meter->write(0, 0, reduction);
            if (trace) {
                for (int i = 0; i < n; i += 4)
                    trace->push(0, 0, reduction);
                trace->commit();
            }
            if (clipEnabled) {
                clip->process(l, r, n);
                processedSamples = uint64_t(n);
            }
            return;
        }
        sleeping = false;
        processedSamples = uint64_t(n);
        if (inPeak < 1e-10f)
            quietInput += uint64_t(n);
        else
            quietInput = 0;
        const float smooth = float(1 - std::exp(-1 / (.01 * sr)));
        const auto order = p.order.valid() ? p.order : EffectOrder{};
        for (auto stage : order.stages)
            switch (stage) {
            case EffectStage::Distortion:
                if (p.activeDistortion() || distortionRack.hasTail() || p.distortionTrace)
                    distort(l, r, n, p, driveMod, smooth);
                break;
            case EffectStage::Phaser:
                if (p.phaser)
                    phaseShift(l, r, n, p, phaserMod);
                break;
            case EffectStage::Chorus:
                if (p.chorus)
                    chorus(l, r, n, p);
                break;
            case EffectStage::Flanger:
                if (flanger.active(p.flange))
                    flanger.process(l, r, n, p.flange);
                break;
            case EffectStage::Equalizer:
                if (p.eq || p.eqTrace)
                    equalize(l, r, n, p);
                break;
            case EffectStage::Reverb:
                if (p.reverb)
                    reverberate(l, r, n, p, smooth);
                break;
            case EffectStage::Delay:
                if (p.delay)
                    delay(l, r, n, p, smooth);
                break;
            case EffectStage::Compressor:
                if (p.compressor)
                    compress(l, r, n, p, meter, trace);
                break;
            case EffectStage::Clipper:
                if (clipEnabled)
                    clip->process(l, r, n);
                break;
            }
        float outPeak = 0;
        for (int i = 0; i < n; ++i)
            outPeak = std::max(outPeak, std::max(std::abs(l[i]), std::abs(r[i])));
        if (outPeak < 1e-10f)
            quietOutput += uint64_t(n);
        else
            quietOutput = 0;
        double horizon = .2;
        if (p.reverb)
            horizon +=
                (p.reverbType == 5 ? double(p.impulseSeconds) : 2.) + p.reverbPredelay * .001;
        if (p.delay) {
            const double left = p.sync ? 60 / p.bpm * beatLength(p.delayDivisionL) : p.delayL;
            const double right = p.sync ? 60 / p.bpm * beatLength(p.delayDivisionR) : p.delayR;
            horizon += std::clamp(std::max(left, right), 0., double(delayLines[0].size() - 2) / sr);
        }
        if (p.chorus)
            horizon += .1;
        if (flanger.active(p.flange))
            horizon += .18;
        if (quietInput > uint64_t(horizon * sr) && quietOutput > uint64_t(.2 * sr))
            sleeping = true;
    }
};
} // namespace logisth
