#pragma once
#include "ConvolutionTests.h"
namespace features0962_test {
inline void run() {
    using namespace juce;
    using namespace logisth;
    auto check = [](bool ok, const char *why) {
        if (!ok)
            throw std::runtime_error(why);
    };
    auto owner = std::make_unique<LogisthProcessor>();
    auto &p = *owner;
    check(p.effectiveValue(Param::compmode) == 1, "new patches select Modern compressor");
    MemoryBlock original;
    p.getStateInformation(original);
    auto xml = AudioProcessor::getXmlFromBinary(original.getData(), int(original.getSize()));
    auto old = ValueTree::fromXml(*xml);
    for (int i = old.getNumChildren(); --i >= 0;) {
        auto child = old.getChild(i);
        for (size_t id = 1055; id < parameterCount; ++id)
            if (child["id"].toString() == parameterSpecs[id].id)
                old.removeChild(i, nullptr);
    }
    MemoryBlock legacy;
    AudioProcessor::copyXmlToBinary(*old.createXml(), legacy);
    check(p.applyState(legacy.getData(), int(legacy.getSize())) &&
              p.effectiveValue(Param::compmode) == 0,
          "older presets automatically retain Legacy compression");
    check(p.effectiveValue(Param::clipon) == 0 && p.effectiveValue(Param::limiton) == 0,
          "older sounds bypass added stages");
    gain_test::cleanSine(p);
    gain_test::set(p, "compon", 1);
    gain_test::set(p, "compmode", 1);
    gain_test::set(p, "compknee", 12);
    gain_test::set(p, "clipon", 1);
    gain_test::set(p, "clipdrive", 12);
    gain_test::set(p, "limiton", 1);
    gain_test::set(p, "limitceiling", -12);
    gain_test::set(p, "limitdrive", 12);
    p.dynamicsView = 2;
    p.prepareToPlay(48000, 512);
    auto audio = processor_phase_test::render(p, 48000, true);
    check(audio.getMagnitude(0, 48000) <= amplitude(-12) + 1e-6 &&
              p.dynamicsMeters[2].reduction.load() > 0,
          "last limiter constrains actual post-Main plug-in output");
    check(p.dynamicsScope.count() > 0, "limiter activity is drawn from actual processing");
    AudioBuffer<float> block(2, 512);
    MidiBuffer midi;
    audio_allocation_probe::begin();
    for (int i = 0; i < 20; ++i)
        p.processBlock(block, midi);
    auto allocations = audio_allocation_probe::end();
    check(allocations == 0, "all dynamics and telemetry allocate no C++ audio memory");
    TemporaryFile preset(".logisth");
    check(p.savePresetFile(preset.getFile()).wasOk(), "dynamics preset saves");
    auto restored = std::make_unique<LogisthProcessor>();
    check(restored->loadPresetFile(preset.getFile()).wasOk(), "dynamics preset restores");
    for (size_t id = 1055; id < parameterCount; ++id)
        check(p.effectiveValue(Param(id)) == restored->effectiveValue(Param(id)),
              "every added dynamics parameter round-trips");
    std::unique_ptr<AudioProcessorEditor> ui(p.createEditor());
    auto *editor = dynamic_cast<LogisthEditor *>(ui.get());
    auto *console = ui->getChildComponent(0);
    int y = -1;
    for (int rack : {8, 0, 1, 2, 3, 4, 5, 6, 7, 11, 12, 9, 10}) {
        auto *button = dynamic_cast<Button *>(console->findChildWithID("rack." + String(rack)));
        check(button && button->getY() > y, "tabs follow Chords/Arp/audio chain/tools order");
        y = button->getY();
        button->onClick();
        if (rack == 7 || rack == 11 || rack == 12)
            check(console->findChildWithID("dynamics.view")->isVisible(),
                  "each dynamics page exposes its graph");
    }
    auto knobEnabled = [&](const char *id) {
        for (auto *child : console->findChildWithID(id)->getChildren())
            if (auto *slider = dynamic_cast<Slider *>(child))
                return slider->isEnabled();
        throw std::runtime_error("missing knob");
    };
    auto waitForKnob = [&](const char *id, bool enabled) {
        for (int retry = 0; retry < 200 && knobEnabled(id) != enabled; ++retry) {
            Thread::sleep(10);
            Timer::callPendingTimersSynchronously();
        }
        return knobEnabled(id) == enabled;
    };
    editor->snapshotPage(35);
    gain_test::set(p, "compmode", 0);
    check(waitForKnob("compknee", false), "Legacy mode refreshes knee without changing pages");
    gain_test::set(p, "compmode", 1);
    check(waitForKnob("compknee", true), "Modern mode restores knee without changing pages");
    editor->snapshotPage(36);
    gain_test::set(p, "clipmode", 0);
    check(waitForKnob("clipknee", false), "Hard curve refreshes its unused knee control");
    editor->snapshotPage(34);
    auto *wave =
        dynamic_cast<FileDragAndDropTarget *>(console->findChildWithID("reverb.ir.waveform"));
    check(wave && console->findChildWithID("reverb.ir.waveform")->isVisible(),
          "IR waveform is a drop target in its existing rack");
    TemporaryFile irFile(".wav");
    auto ir = convolution_test::impulse(48000);
    convolution_test::write(irFile.getFile(), ir);
    const auto before = p.impulseData()->revision;
    check(wave->isInterestedInFileDrag({irFile.getFile().getFullPathName()}) &&
              !wave->isInterestedInFileDrag({"invalid.txt"}),
          "IR drag validates audio extensions");
    wave->filesDropped({irFile.getFile().getFullPathName()}, 5, 5);
    for (int i = 0; i < 1000 && p.impulseData()->revision == before; ++i)
        Thread::sleep(5);
    check(p.impulseData()->revision != before && p.impulseData()->peak > 0,
          "IR drop imports asynchronously and builds cached waveform extrema");
    auto error = editor->validateLayout();
    if (error.isNotEmpty())
        std::cerr << error << '\n';
    check(error.isEmpty(), "all 1070 controls fit and remain reachable");
    gain_test::set(p, "midionly", 1);
    gain_test::set(p, "arpout", 1);
    gain_test::set(p, "arpon", 1);
    p.prepareToPlay(48000, 512);
    midi.addEvent(MidiMessage::noteOn(1, 60, .8f), 0);
    bool emitted = false;
    for (int i = 0; i < 100; ++i) {
        p.processBlock(block, midi);
        for (auto event : midi)
            emitted |= event.getMessage().isNoteOn();
        midi.clear();
        check(block.getMagnitude(0, 512) == 0 && p.effectSamples.load() == 0,
              "MIDI-only mode skips audio synthesis and effects");
    }
    check(emitted && p.voices.load() == 0,
          "MIDI-only arp still emits notes without hidden audio voices");
    std::cout << "PASS 0.9.6.2 integration: Legacy recall, dynamic presets, post-Main ceiling, "
                 "live graphs, no audio allocations, ordered tabs, IR waveform/drop, 1070-control "
                 "layout and MIDI-only CPU\n";
}
} // namespace features0962_test
