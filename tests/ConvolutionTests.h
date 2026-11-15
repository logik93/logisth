#pragma once
#include "AllocationProbe.h"
#include "ProcessorPhaseTests.h"
namespace convolution_test {
using namespace juce;
using namespace logisth;
inline void check(bool ok, const char *text) {
    if (!ok)
        throw std::runtime_error(text);
}
inline ImpulseData impulse(double rate) {
    ImpulseData data;
    data.sampleRate = rate;
    data.name = "Stereo FIR test";
    for (auto &c : data.samples)
        c.resize(65);
    data.samples[0][0] = .8f;
    data.samples[0][12] = .3f;
    data.samples[1][5] = .6f;
    data.samples[1][21] = -.2f;
    return data;
}
inline void write(const File &file, const ImpulseData &data) {
    WavAudioFormat wav;
    auto stream = file.createOutputStream();
    stream->setPosition(0);
    stream->truncate();
    std::unique_ptr<AudioFormatWriter> w(
        wav.createWriterFor(stream.get(), data.sampleRate, unsigned(data.channels), 24, {}, 0));
    check(bool(w), "IR test writer");
    stream.release();
    AudioBuffer<float> b(data.channels, int(data.samples[0].size()));
    for (int ch = 0; ch < data.channels; ++ch)
        b.copyFrom(ch, 0, data.samples[size_t(ch)].data(), b.getNumSamples());
    check(w->writeFromAudioSampleBuffer(b, 0, b.getNumSamples()), "IR fixture writes");
}
inline void run() {
    for (double rate : {44100., 48000., 96000.})
        for (int blockSize : {1, 17, 256, 511}) {
            auto data = impulse(rate);
            PreparedImpulse convolver(data, rate);
            check(convolver.latency() == 0, "partitioned convolution has zero added latency");
            std::array<std::array<float, 4096>, 2> input{}, output{};
            for (int ch = 0; ch < 2; ++ch)
                for (int i = 0; i < 4096; ++i)
                    input[size_t(ch)][size_t(i)] = .1f * std::sin(float(i) * (.11f + ch * .031f));
            audio_allocation_probe::begin();
            for (int at = 0; at < 4096; at += blockSize) {
                int n = std::min(blockSize, 4096 - at);
                convolver.process(input[0].data() + at, input[1].data() + at, output[0].data() + at,
                                  output[1].data() + at, n, false);
            }
            auto allocations = audio_allocation_probe::end();
            check(allocations == 0, "prepared convolution processing allocates no C++ memory");
            for (int ch = 0; ch < 2; ++ch)
                for (int i = 0; i < 4096; ++i) {
                    double ref = 0;
                    for (int tap = 0; tap < int(data.samples[size_t(ch)].size()) && tap <= i; ++tap)
                        ref += input[size_t(ch)][size_t(i - tap)] *
                               data.samples[size_t(ch)][size_t(tap)];
                    check(std::abs(ref - output[size_t(ch)][size_t(i)]) < 2e-5,
                          "FFT result matches direct stereo FIR at every block size/rate");
                }
            convolver.reset();
            float zeros[256]{}, l[256], r[256];
            convolver.process(zeros, zeros, l, r, 256, false);
            for (int i = 0; i < 256; ++i)
                check(std::abs(l[i]) + std::abs(r[i]) < 1e-7, "IR reset clears previous tails");
        }
    for (double rate : {44100., 48000., 96000.}) {
        auto data = impulse(44100);
        PreparedImpulse convolver(data, rate);
        double e[2]{};
        float l[512]{}, r[512]{}, ol[512], orr[512];
        for (int b = 0; b < 8; ++b) {
            std::fill_n(l, 512, 0);
            std::fill_n(r, 512, 0);
            if (b == 0)
                l[0] = r[0] = 1;
            convolver.process(l, r, ol, orr, 512, true);
            for (int i = 0; i < 512; ++i) {
                e[0] += ol[i] * ol[i];
                e[1] += orr[i] * orr[i];
            }
        }
        check(std::abs(std::max(e[0], e[1]) - 1) < 2e-4,
              "IR normalization is based on the prepared response at the actual host rate");
    }
    auto instance = std::make_unique<LogisthProcessor>();
    auto &p = *instance;
    gain_test::set(p, "reverbtype", 5);
    gain_test::set(p, "reverbon", 1);
    gain_test::set(p, "reverb", 1);
    gain_test::set(p, "irnormalize", 0);
    TemporaryFile file(".wav");
    auto data = impulse(48000);
    write(file.getFile(), data);
    check(p.loadImpulse(file.getFile()).wasOk(), "stereo IR imports");
    p.prepareToPlay(48000, 512);
    check(p.impulseReady(), "IR is fully ready after prepare");
    auto owned = p.impulseData();
    TemporaryFile preset(".logisth");
    check(p.savePresetFile(preset.getFile()).wasOk(), "IR preset saves");
    file.getFile().deleteFile();
    auto restored = std::make_unique<LogisthProcessor>();
    check(restored->loadPresetFile(preset.getFile()).wasOk(),
          "IR preset loads without its original file");
    check(owned->samples == restored->impulseData()->samples &&
              owned->sampleRate == restored->impulseData()->sampleRate,
          "preset embeds stereo IR PCM losslessly");
    restored->prepareToPlay(96000, 512);
    check(restored->impulseReady(), "embedded IR prepares at a different host rate");
    MemoryBlock project;
    p.getStateInformation(project);
    restored->setStateInformation(project.getData(), int(project.getSize()));
    check(restored->impulseData()->samples == owned->samples,
          "DAW state embeds the impulse response too");
    {
        TemporaryFile monoFile(".wav"), monoPreset(".logisth");
        auto mono = impulse(44100);
        mono.channels = 1;
        write(monoFile.getFile(), mono);
        auto monoProcessor = std::make_unique<LogisthProcessor>();
        check(monoProcessor->loadImpulse(monoFile.getFile()).wasOk(),
              "mono IR imports before selecting convolution");
        check(monoProcessor->impulseReady(),
              "inactive imported IR is already validated and prepared");
        check(monoProcessor->savePresetFile(monoPreset.getFile()).wasOk(),
              "inactive user IR embeds in preset");
        monoFile.getFile().deleteFile();
        check(monoProcessor->loadPresetFile(monoPreset.getFile()).wasOk() &&
                  monoProcessor->impulseData()->channels == 1 &&
                  monoProcessor->impulseData()->samples[0] ==
                      monoProcessor->impulseData()->samples[1],
              "mono embedded IR preserves channel metadata and independent L/R processing");
    }
    {
        auto longProcessor = std::make_unique<LogisthProcessor>();
        longProcessor->prepareToPlay(8000, 512);
        auto longIR = impulse(8000);
        longIR.channels = 1;
        for (auto &c : longIR.samples) {
            c.assign(8000 * 61, 0);
            c.front() = .8f;
            c.back() = .2f;
        }
        TemporaryFile longFile(".wav");
        write(longFile.getFile(), longIR);
        check(longProcessor->loadImpulse(longFile.getFile()).wasOk(),
              "long IR has no fixed duration cutoff");
        gain_test::set(*longProcessor, "reverbtype", 5);
        gain_test::set(*longProcessor, "reverbon", 1);
        check(longProcessor->getTailLengthSeconds() >= 121,
              "host tail duration includes long IR plus instrument/effects release");
    }
    // Reject malformed embedded impulses before applying any source/parameter state.
    auto xml = AudioProcessor::getXmlFromBinary(project.getData(), int(project.getSize()));
    auto bad = ValueTree::fromXml(*xml);
    for (auto node : bad)
        if (node.hasType("IMPULSE"))
            node.setProperty("format", 999, nullptr);
    MemoryBlock malformed;
    AudioProcessor::copyXmlToBinary(*bad.createXml(), malformed);
    auto before = restored->impulseData();
    check(!restored->applyState(malformed.getData(), int(malformed.getSize())) &&
              restored->impulseData() == before,
          "malformed IR presets leave the current sound intact");
    TemporaryFile silent(".wav");
    auto blank = impulse(48000);
    for (auto &c : blank.samples)
        std::fill(c.begin(), c.end(), 0);
    write(silent.getFile(), blank);
    check(p.loadImpulse(silent.getFile()).failed() && p.impulseData() == owned,
          "silent IR import fails without losing the previous response");
    gain_test::set(p, "o1level", .1f);
    gain_test::set(p, "polyphony", 1);
    gain_test::set(p, "arpon", 0);
    p.prepareToPlay(48000, 512);
    auto audio = processor_phase_test::render(p, 2048, true);
    check(audio.getMagnitude(0, 2048) > 1e-6, "convolution reaches the actual instrument output");
    AudioBuffer<float> block(2, 512);
    MidiBuffer midi;
    audio_allocation_probe::begin();
    for (int i = 0; i < 12; ++i) {
        p.processBlock(block, midi);
        midi.clear();
    }
    auto allocated = audio_allocation_probe::end();
    check(allocated == 0, "complete convolution instrument callback allocates no C++ memory");
    // Runtime replacement keeps old/new engines valid until the audio crossfade ends.
    TemporaryFile nextFile(".wav");
    auto next = impulse(48000);
    next.samples[0][0] = -.5f;
    write(nextFile.getFile(), next);
    auto revision = p.impulseData()->revision;
    p.loadImpulseAsync(nextFile.getFile());
    for (int retry = 0;
         retry < 1000 && (p.impulseData()->revision == revision || !p.impulseReady()); ++retry) {
        p.processBlock(block, midi);
        midi.clear();
        Thread::sleep(5);
    }
    check(p.impulseData()->revision != revision && p.impulseReady(),
          "live IR replacement prepares outside processing");
    for (int i = 0; i < 30; ++i) {
        p.processBlock(block, midi);
        midi.clear();
        for (int ch = 0; ch < 2; ++ch)
            for (int s = 0; s < 512; ++s)
                check(std::isfinite(block.getSample(ch, s)), "IR crossfades remain finite");
        p.serviceAssets();
    }
    p.loadImpulseAsync(nextFile.getFile());
    p.clearImpulse();
    for (int i = 0; i < 50; ++i) {
        p.serviceAssets();
        Thread::sleep(5);
        check(p.impulseData() == ImpulseData::factory(),
              "reset supersedes queued older impulse imports");
    }
    std::unique_ptr<AudioProcessorEditor> ui(p.createEditor());
    auto *editor = dynamic_cast<LogisthEditor *>(ui.get());
    auto *console = ui->getChildComponent(0);
    editor->snapshotPage(34);
    for (auto id :
         {"reverb.ir.load", "reverb.ir.reset", "reverb.ir.name", "irnormalize", "reverbout"})
        check(console->findChildWithID(id) && console->findChildWithID(id)->isVisible(),
              "convolution controls share the existing rack");
    for (auto id : {"reverbdecay", "reverbdamp", "reverbsize", "shimmeramount", "springdispersion"})
        check(!console->findChildWithID(id)->isVisible(),
              "convolution hides unrelated algorithm controls");
    auto error = editor->validateLayout();
    if (error.isNotEmpty())
        std::cerr << error << '\n';
    check(error.isEmpty(), "all new impulse controls fit without overlap");
    std::cout << "PASS convolution: direct FIR parity, three rates, arbitrary blocks, zero "
                 "latency, normalization, embedded preset/project PCM, invalid imports, live "
                 "replacement and audio allocation guards\n";
}
} // namespace convolution_test
