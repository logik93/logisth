#pragma once
#include "ConvolutionTests.h"
namespace stereo_review_test {
using namespace juce;
using namespace logisth;
inline void check(bool value, const char *why) {
    if (!value)
        throw std::runtime_error(why);
}
inline AudioBuffer<float> source(double rate, int pattern, int n = 4096) {
    AudioBuffer<float> result(2, n);
    for (int i = 0; i < n; ++i) {
        float l = .2f * std::sin(float(2 * pi * 331 * i / rate));
        float r = .15f * std::sin(float(2 * pi * 991 * i / rate));
        if (i == 97)
            l += .25f;
        if (i == 233)
            r -= .2f;
        if (pattern == 1)
            r = 0;
        if (pattern == 2)
            l = 0;
        if (pattern == 3)
            r = -l;
        result.setSample(0, i, l);
        result.setSample(1, i, r);
    }
    return result;
}
inline void write(const File &file, const AudioBuffer<float> &data, double rate) {
    WavAudioFormat wav;
    auto stream = file.createOutputStream();
    stream->setPosition(0);
    stream->truncate();
    std::unique_ptr<AudioFormatWriter> writer(
        wav.createWriterFor(stream.get(), rate, 2, 24, {}, 0));
    check(bool(writer), "sample WAV writer");
    stream.release();
    check(writer->writeFromAudioSampleBuffer(data, 0, data.getNumSamples()), "sample WAV writes");
}
inline double compareFIR(const AudioBuffer<float> &input, const AudioBuffer<float> &actual,
                         const ImpulseData &ir, float gain, int predelay = 0) {
    double error = 0;
    for (int ch = 0; ch < 2; ++ch)
        for (int i = 0; i < actual.getNumSamples(); ++i) {
            double expected = 0;
            for (int tap = 0; tap < int(ir.samples[size_t(ch)].size()); ++tap) {
                const int from = i - tap - predelay;
                if (from >= 0 && from < input.getNumSamples())
                    expected += input.getSample(ch, from) * ir.samples[size_t(ch)][size_t(tap)];
            }
            error = std::max(error, std::abs(actual.getSample(ch, i) - expected * gain));
        }
    return error;
}
inline void run(const File &report = {}) {
    TemporaryFile temporary;
    const auto root = report == File{} ? temporary.getFile() : report;
    check(root.createDirectory().wasOk(), "stereo review fixture directory");
    String csv =
        "sample_rate,ir,source,normalized,predelay_ms,max_error,left_energy,right_energy\n";
    int cases = 0;
    for (double rate : {44100., 48000., 96000.})
        for (int stereo = 0; stereo < 2; ++stereo) {
            auto owner = std::make_unique<LogisthProcessor>();
            auto &p = *owner;
            auto ir = convolution_test::impulse(rate);
            ir.channels = stereo ? 2 : 1;
            if (!stereo)
                ir.samples[1] = ir.samples[0];
            const auto irFile = root.getChildFile("ir-" + String(int(rate)) +
                                                  (stereo ? "-stereo.wav" : "-mono.wav"));
            convolution_test::write(irFile, ir);
            check(p.loadImpulse(irFile).wasOk(), "actual mono/stereo IR file imports");
            auto decodedIR = p.impulseData();
            for (int pattern = 0; pattern < 4; ++pattern) {
                const auto audioFile = root.getChildFile("source-" + String(int(rate)) + "-" +
                                                         String(pattern) + ".wav");
                write(audioFile, source(rate, pattern), rate);
                check(p.loadSource(0, audioFile).wasOk(), "actual stereo source WAV imports");
                const auto decoded = p.sourceData(0);
                AudioBuffer<float> input(2, int(decoded->samples[0].size()));
                for (int ch = 0; ch < 2; ++ch)
                    input.copyFrom(ch, 0, decoded->samples[size_t(ch)].data(),
                                   input.getNumSamples());
                for (bool normalize : {false, true}) {
                    auto convolver = std::make_unique<PreparedImpulse>(*decodedIR, rate);
                    ImpulseRack rack;
                    rack.publish(convolver.get());
                    auto effects = std::make_unique<Effects>();
                    effects->prepare(rate);
                    EffectsParams params;
                    params.reverbType = 5;
                    params.reverbMix = params.reverbWidth = 1;
                    params.irNormalize = normalize;
                    params.convolution = &rack;
                    const int n = input.getNumSamples() + 256;
                    AudioBuffer<float> output(2, n);
                    audio_allocation_probe::begin();
                    for (int at = 0; at < n; at += 257) {
                        const auto count = std::min(257, n - at);
                        float l[257]{}, r[257]{}, zeros[257]{};
                        for (int i = 0; i < count && at + i < input.getNumSamples(); ++i) {
                            l[i] = input.getSample(0, at + i);
                            r[i] = input.getSample(1, at + i);
                        }
                        effects->process(l, r, count, zeros, zeros, params);
                        output.copyFrom(0, at, l, count);
                        output.copyFrom(1, at, r, count);
                    }
                    const auto allocations = audio_allocation_probe::end();
                    const auto error = compareFIR(input, output, *decodedIR,
                                                  normalize ? convolver->normalizationGain : 1);
                    if (error >= 2e-5)
                        std::cerr << "Stereo FIR error rate=" << rate << " IR=" << stereo
                                  << " pattern=" << pattern << " norm=" << normalize
                                  << " error=" << error << '\n';
                    check(error < 2e-5 && allocations == 0,
                          "real-file stereo convolution matches independent L/R FIR through the "
                          "complete reverb stage");
                    double energy[2]{};
                    for (int i = 0; i < n; ++i)
                        for (int ch = 0; ch < 2; ++ch)
                            energy[ch] += std::pow(output.getSample(ch, i), 2);
                    if (pattern == 1)
                        check(energy[1] < 1e-12,
                              "left-only source never leaks into right convolution lane");
                    if (pattern == 2)
                        check(energy[0] < 1e-12,
                              "right-only source never leaks into left convolution lane");
                    if (pattern == 3 && !stereo)
                        for (int i = 0; i < n; ++i)
                            check(std::abs(output.getSample(0, i) + output.getSample(1, i)) < 2e-6,
                                  "mono IR preserves a stereo source's opposite polarity instead "
                                  "of cancelling it");
                    csv += String(int(rate)) + "," + (stereo ? "stereo" : "mono") + "," +
                           String(pattern) + "," + String(int(normalize)) + ",0," +
                           String(error, 10) + "," + String(energy[0], 8) + "," +
                           String(energy[1], 8) + "\n";
                    ++cases;
                    if (report != File{} && rate == 48000 && pattern == 0 && !normalize)
                        write(root.getChildFile(stereo ? "output-stereo-ir.wav"
                                                       : "output-mono-ir.wav"),
                              output, rate);
                }
            }
        }
    // Exercise the real instrument/sampler, complete mixer and Main after the IR.
    for (int stereo = 0; stereo < 2; ++stereo) {
        auto dry = std::make_unique<LogisthProcessor>(), wet = std::make_unique<LogisthProcessor>();
        const auto file = root.getChildFile("instrument-stereo.wav");
        write(file, source(48000, 0), 48000);
        for (auto *p : {dry.get(), wet.get()}) {
            p->loadPreset(0);
            check(p->loadSource(0, file).wasOk(), "sampler integration loads the audio fixture");
            gain_test::set(*p, "o1engine", 2);
            gain_test::set(*p, "o1root", 69);
            gain_test::set(*p, "o1level", .5f);
            gain_test::set(*p, "reverbtype", 5);
            gain_test::set(*p, "reverb", 1);
            gain_test::set(*p, "irnormalize", 0);
            check(p->loadImpulse(
                       root.getChildFile(stereo ? "ir-48000-stereo.wav" : "ir-48000-mono.wav"))
                      .wasOk(),
                  "instrument integration imports the IR");
            gain_test::set(*p, "reverbon", p == wet.get() ? 1.f : 0.f);
            p->prepareToPlay(48000, 512);
        }
        const auto input = processor_phase_test::render(*dry, 4608, true);
        const auto actual = processor_phase_test::render(*wet, 4608, true);
        const auto error = compareFIR(input, actual, *wet->impulseData(), 1);
        if (error >= 2e-5)
            std::cerr << "Instrument stereo error=" << error << '\n';
        check(error < 2e-5 && actual.getMagnitude(0, actual.getNumSamples()) > .001,
              "sampler, convolution, mixer and Main preserve the expected stereo FIR result");
    }
    check(cases == 48, "all stereo/mono IR, lane, polarity, rate and normalization cases tested");
    if (report != File{}) {
        check(root.getChildFile("stereo-convolution.csv").replaceWithText(csv),
              "stereo review CSV writes");
        root.getChildFile("README.md")
            .replaceWithText(
                "# Stereo convolution validation\n\n"
                "48 real-WAV cases at 44.1/48/96 kHz, mono and asymmetric stereo IRs, "
                "independent/left-only/right-only/antiphase inputs, normalization on/off.\n"
                "The actual reverb stage is compared sample by sample with independent direct L/R "
                "FIR convolution; no C++ allocations are permitted during processing.\n"
                "Two additional tests render the actual sampler instrument and compare its output "
                "through convolution and Main with the dry instrument's FIR reference.\n"
                "Stereo IR routing is diagonal: L * IR-L and R * IR-R. A mono IR filters both "
                "input lanes independently; it does not sum the source to mono. This is not a "
                "four-channel cross-convolution matrix.\n"
                "Fixtures are original mathematical tones and impulses, not third-party "
                "samples.\n");
    } else
        root.deleteRecursively();
    std::cout << "PASS stereo convolution: 48 real-WAV lane/polarity/normalization/rate cases, "
                 "stereo sampler integration and audio allocation guards\n";
}
} // namespace stereo_review_test
