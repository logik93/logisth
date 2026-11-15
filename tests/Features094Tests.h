#pragma once
#include "ProcessorPhaseTests.h"
namespace features094_test {
inline void run() {
    using namespace juce;
    using namespace logisth;
    auto check = [](bool ok, const char *text) {
        if (!ok)
            throw std::runtime_error(text);
    };
    auto instance = std::make_unique<LogisthProcessor>();
    auto &p = *instance;
    gain_test::set(p, "polyphony", 1);
    for (int slot = 0; slot < 4; ++slot) {
        String id = "o" + String(slot + 1);
        gain_test::set(p, id + "engine", 2);
        gain_test::set(p, id + "voices", 1);
        gain_test::set(p, id + "stretchmode", 7);
        gain_test::set(p, id + "ssrate", 1 + slot * .25f);
        gain_test::set(p, id + "sssemitones", float(slot * 3));
        gain_test::set(p, id + "sstonality", float(1000 + slot * 500));
        gain_test::set(p, id + "ssformant", float(slot - 2));
        gain_test::set(p, id + "ssblock", float(32 + slot * 8));
        gain_test::set(p, id + "ssoverlap", float(3 + slot));
    }
    p.prepareToPlay(48000, 512);
    for (int slot = 0; slot < 4; ++slot)
        check(p.spectralReady(slot), "each source prepares its own analysis configuration");
    TemporaryFile preset(".logisth");
    check(p.savePresetFile(preset.getFile()).wasOk(), "Signalsmith preset saves");
    auto restored = std::make_unique<LogisthProcessor>();
    check(restored->loadPresetFile(preset.getFile()).wasOk(), "Signalsmith preset loads");
    for (int slot = 0; slot < 4; ++slot)
        for (auto suffix :
             {"ssrate", "sssemitones", "sstonality", "ssformant", "ssblock", "ssoverlap"}) {
            String id = "o" + String(slot + 1) + suffix;
            check(p.state.getRawParameterValue(id)->load() ==
                      restored->state.getRawParameterValue(id)->load(),
                  "all six controls round-trip independently in all four source slots");
        }
    std::unique_ptr<AudioProcessorEditor> ui(p.createEditor());
    auto *editor = dynamic_cast<LogisthEditor *>(ui.get());
    auto *console = ui->getChildComponent(0);
    for (int part = 0; part < 2; ++part) {
        dynamic_cast<Button *>(console->findChildWithID("part." + String(part)))->onClick();
        for (int side = 0; side < 2; ++side) {
            int slot = part * 2 + side;
            auto *view =
                dynamic_cast<ComboBox *>(console->findChildWithID("source.view." + String(slot)));
            view->setSelectedId(10, sendNotificationSync);
            String id = "o" + String(slot + 1);
            for (auto suffix :
                 {"ssrate", "sssemitones", "sstonality", "ssformant", "ssblock", "ssoverlap"})
                check(console->findChildWithID(id + suffix)->isVisible(),
                      "all Signalsmith controls are visible in their compact source page");
        }
    }
    check(editor->validateLayout().isEmpty(), "every 0.9.4 control fits without overlap");
    ui.reset();
    gain_test::cleanSine(p);
    gain_test::set(p, "polyphony", 1);
    gain_test::set(p, "o1engine", 2);
    gain_test::set(p, "o1stretchmode", 7);
    gain_test::set(p, "o1loop", 1);
    p.prepareToPlay(48000, 512);
    auto first = processor_phase_test::render(p, 2048, true);
    for (int index = 0; index < 6; ++index) {
        gain_test::set(p, "o1level", 0);
        processor_phase_test::render(p, 512);
        gain_test::set(p, "o1ssblock", float(30 + index * 12));
        gain_test::set(p, "o1ssoverlap", float(2 + index));
        p.ensureSpectral(true);
        p.serviceAssets(); // Retire old pools while the player is muted, not idle.
        check(p.spectralReady(0), "live window changes replace the selected source pool");
        gain_test::set(p, "o1level", .2f);
        auto audio = processor_phase_test::render(p, 2048);
        for (int ch = 0; ch < 2; ++ch)
            for (int i = 0; i < audio.getNumSamples(); ++i)
                check(std::isfinite(audio.getSample(ch, i)) && std::abs(audio.getSample(ch, i)) < 8,
                      "muted players resume safely after old analysis pools have been retired");
    }
    gain_test::set(p, "o1ssblock", 150);
    p.ensureSpectral(false);
    gain_test::set(p, "o1ssblock", 55);
    p.ensureSpectral(false);
    for (int retry = 0; retry < 200 && !p.spectralReady(0); ++retry) {
        p.ensureSpectral(false);
        Thread::sleep(10);
    }
    check(p.spectralReady(0), "the last window request wins over superseded preparation work");
    gain_test::set(p, "o1stretchmode", 0);
    p.serviceAssets();
    check(!p.spectralReady(0), "disabling spectral modes releases their current pool");
    processor_phase_test::render(p, 1024);
    gain_test::set(p, "o1stretchmode", 7);
    p.ensureSpectral(true);
    processor_phase_test::render(p, 1024);
    std::cout << "PASS 0.9.4 integration: six per-player controls, preset round-trips, compact UI, "
                 "live/muted pool changes and last-request preparation\n";
}
} // namespace features094_test
