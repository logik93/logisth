#pragma once
#include "ProcessorPhaseTests.h"
namespace oscillator_clipboard_test {
inline void run() {
    using namespace juce;
    using namespace logisth;
    auto check = [](bool ok, const char *m) {
        if (!ok)
            throw std::runtime_error(m);
    };
    auto instance = std::make_unique<LogisthProcessor>();
    auto &p = *instance;
    TemporaryFile file(".wav");
    {
        WavAudioFormat wav;
        auto stream = file.getFile().createOutputStream();
        std::unique_ptr<AudioFormatWriter> w(
            wav.createWriterFor(stream.get(), 48000, 2, 24, {}, 0));
        check(bool(w), "oscillator clipboard writer");
        stream.release();
        AudioBuffer<float> b(2, 4096);
        for (int ch = 0; ch < 2; ++ch)
            for (int i = 0; i < 4096; ++i)
                b.setSample(ch, i, float(std::sin(i * .03 + ch)) * .3f);
        check(w->writeFromAudioSampleBuffer(b, 0, 4096), "clipboard source writes");
    }
    check(p.loadSource(0, file.getFile(), 128).wasOk(), "clipboard material imports");
    auto source = p.sourceData(0);
    file.getFile().deleteFile();
    for (int sourceSlot = 0; sourceSlot < playerCount; ++sourceSlot) {
        if (sourceSlot > 0) {
            check(p.copyOscillator(0).wasOk() && p.pasteOscillator(sourceSlot).wasOk(),
                  "clipboard transfers source audio to each part");
        }
        String prefix = "o" + String(sourceSlot + 1);
        gain_test::set(p, prefix + "engine", float(sourceSlot % 4));
        gain_test::set(p, prefix + "detune", 70);
        gain_test::set(p, prefix + "detuneextra", 90);
        gain_test::set(p, prefix + "detunewide", 125);
        gain_test::set(p, prefix + "phase", 137);
        gain_test::set(p, prefix + "stereo", .87f);
        gain_test::set(p, prefix + "blend", .37f);
        gain_test::set(p, prefix + "tailon", 1);
        gain_test::set(p, prefix + "tailstart", .3f);
        gain_test::set(p, prefix + "tailend", .8f);
        gain_test::set(p, prefix + "stretchmode", 3);
        auto fm = Param(playerParamIndex(Param::o1fmsource, sourceSlot, 5));
        gain_test::set(p, parameterSpecs[size_t(fm)].id, sourceSlot < 4 ? 0.f : 6.f);
        if (sourceSlot < 4)
            gain_test::set(p, prefix + "fmsourcecx", 2);
        check(p.copyOscillator(sourceSlot).wasOk(), "all six players can copy settings");
        for (int target = 0; target < playerCount; ++target) {
            check(p.pasteOscillator(target).wasOk(), "all six players can paste settings");
            String to = "o" + String(target + 1);
            for (const auto &spec : parameterSpecs) {
                String id(spec.id);
                if (!id.startsWith(prefix))
                    continue;
                String suffix = id.substring(2);
                if (suffix == "fmsource" || suffix == "amsource" || suffix.endsWith("cx"))
                    continue;
                check(std::abs(p.state.getRawParameterValue(id)->load() -
                               p.state.getRawParameterValue(to + suffix)->load()) < 1e-5,
                      "every oscillator control is copied independently of slot layout");
            }
            check(p.effectiveValue(Param(playerParamIndex(Param::o1fmsource, target, 5))) == 6,
                  "C FM-source choices retain their meaning in A/B/C destinations");
            check(p.sourceData(target) == source && p.sourceData(target)->frameSize == 128,
                  "paste includes immutable stereo audio and wavetable slicing");
        }
    }
    auto peer = std::make_unique<LogisthProcessor>();
    check(peer->pasteOscillator(5).wasOk() && peer->sourceData(5) == source,
          "private clipboard works between instances without the original file");
    check(p.copyOscillator(-1).failed() && p.pasteOscillator(6).failed(),
          "clipboard rejects invalid slots");
    std::unique_ptr<AudioProcessorEditor> ui(p.createEditor());
    auto *console = ui->getChildComponent(0);
    for (int part = 0; part < partCount; ++part) {
        dynamic_cast<Button *>(console->findChildWithID("part." + String(part)))->onClick();
        for (int side = 0; side < 2; ++side) {
            int slot = part * 2 + side;
            for (auto action : {"copy", "paste"}) {
                auto *b = dynamic_cast<Button *>(
                    console->findChildWithID("osc." + String(action) + "." + String(slot)));
                check(b && b->isVisible() && b->isEnabled(),
                      "small copy/paste buttons exist in every oscillator panel");
            }
        }
    }
    std::cout
        << "PASS oscillator clipboard: all 36 source/destination pairs, complete controls, "
           "semantic FM routing, embedded media, cross-instance paste and six panel buttons\n";
}
} // namespace oscillator_clipboard_test
