#pragma once
#include "ProcessorPhaseTests.h"
namespace features095_test {
inline void run() {
    using namespace juce;
    using namespace logisth;
    auto check = [](bool ok, const char *message) {
        if (!ok)
            throw std::runtime_error(message);
    };
    const auto contract = JSON::parse(
        File(String(LOGISTH_SOURCE_DIR)).getChildFile("tests/fixtures/parameters-0.9.4.1.json"));
    check(contract.isArray() && contract.size() == 736, "frozen legacy parameter contract exists");
    auto instance = std::make_unique<LogisthProcessor>();
    auto &p = *instance;
    for (int i = 0; i < contract.size(); ++i) {
        auto row = contract[i];
        const auto &spec = parameterSpecs[size_t(i)];
        const double expectedDefault = i == int(Param::bendrange) ? 12. : double(row["initial"]);
        const bool convolution = String(spec.id) == "reverbtype";
        auto equal = [&](const char *key, float value) {
            return std::abs(double(row[key]) - double(value)) < 1e-6;
        };
        check(int(row["index"]) == i && row["id"].toString() == spec.id &&
                  row["label"].toString() == spec.label && row["group"].toString() == spec.group &&
                  int(row["page"]) == spec.page && equal("low", spec.low) &&
                  equal("high", spec.high - (convolution ? 1 : 0)) &&
                  std::abs(expectedDefault - spec.initial) < 1e-6 && equal("skew", spec.skew) &&
                  row["choices"].toString() + (convolution ? "|Convolution" : "") == spec.choices &&
                  bool(row["integer"]) == spec.integer,
              "all 736 original parameter IDs, indices, ranges and choices stay unchanged");
        check(p.getParameters()[i] == p.state.getParameter(spec.id),
              "legacy host parameter ordering is retained");
    }
    check(parameterCount >= 954, "new parameters are appended after the legacy contract");
    auto raw = [&](const String &id) { return p.state.getRawParameterValue(id)->load(); };
    for (int slot = 0; slot < playerCount; ++slot) {
        String id = "o" + String(slot + 1);
        auto *detune = p.state.getParameter(id + "detune");
        detune->setValueNotifyingHost(.5f);
        check(raw(id + "detune") == 50 && raw(id + "detuneextra") == 0,
              "old normalized detune automation still means the same cents");
        check(raw(id + "tailon") == 0, "tail loops are opt-in for all sources");
        if (slot >= 4)
            check(raw(id + "level") == 0, "new C players start silent");
    }
    std::unique_ptr<AudioProcessorEditor> ui(p.createEditor());
    auto *editor = dynamic_cast<LogisthEditor *>(ui.get());
    auto *console = ui->getChildComponent(0);
    auto widget = [&](const String &id) -> Component * {
        auto *component = console->findChildWithID(id);
        check(component != nullptr, "0.9.5 component exists");
        return component;
    };
    auto menu = [&](const String &id) -> ComboBox * {
        auto *m = dynamic_cast<ComboBox *>(widget(id));
        if (!m)
            for (auto *child : widget(id)->getChildren())
                if (auto *candidate = dynamic_cast<ComboBox *>(child))
                    m = candidate;
        check(m != nullptr, "parameter has a menu");
        return m;
    };
    for (int part = 0; part < partCount; ++part) {
        dynamic_cast<Button *>(widget("part." + String(part)))->onClick();
        for (int side = 0; side < 2; ++side) {
            int slot = part * 2 + side;
            String id = "o" + String(slot + 1);
            Slider *knob = nullptr;
            for (auto *child : widget(id + "detune")->getChildren())
                if (auto *slider = dynamic_cast<Slider *>(child))
                    knob = slider;
            check(knob && knob->isVisible() && knob->getMaximum() == 400,
                  "all six main detune knobs reach 200 cents");
            knob->setValue(175, sendNotificationSync);
            check(raw(id + "detune") == 100 && raw(id + "detuneextra") == 75,
                  "wide detune splits into compatible base and additive parameters");
            knob->setValue(35, sendNotificationSync);
            check(raw(id + "detune") == 35 && raw(id + "detuneextra") == 0,
                  "returning to the original range clears the additive offset");
            menu(id + "engine")->setSelectedId(3, sendNotificationSync);
            menu("source.view." + String(slot))->setSelectedId(11, sendNotificationSync);
            for (auto suffix : {"tailon", "tailstart", "tailend", "tailfade"})
                check(widget(id + suffix)->isVisible(), "every sampler exposes its own tail page");
        }
    }
    auto *dest = menu("lfo1dest1");
    int c1;
    // Match the public enum order, independent of presentation wording.
    c1 = int(Destination::PositionC1) - int(parameterSpecs[size_t(Param::lfo1dest1)].high);
    dest->setSelectedId(int(Destination::PositionC1) + 1, sendNotificationSync);
    check(raw("lfo1dest1cx") == c1 &&
              p.effectiveValue(Param::lfo1dest1) == float(Destination::PositionC1),
          "extended modulation menus select an independent C1 target");
    dest->setSelectedId(int(Destination::PositionB2) + 1, sendNotificationSync);
    check(raw("lfo1dest1cx") == 0 && raw("lfo1dest1") == float(Destination::PositionB2),
          "selecting a legacy destination clears the extension");
    menu("p1input")->setSelectedId(8, sendNotificationSync);
    check(p.effectiveValue(Param::p1input) == 7 && raw("p1inputcx") == 4,
          "legacy filter menu also accepts all three parts");
    menu("o1fmsource")->setSelectedId(7, sendNotificationSync);
    check(p.effectiveValue(Param::o1fmsource) == 6 && raw("o1fmsourcecx") == 2,
          "C2 is available as an FM source in original A/B players");
    check(menu("sample.slot")->getNumItems() == playerCount,
          "waveform slot selector includes C1/C2");
    menu("sample.slot")->setSelectedId(5, sendNotificationSync);
    // Tail editing uses its own gestures and does not alter the primary selection.
    editor->snapshotPage(19);
    menu("sample.slot")->setSelectedId(5, sendNotificationSync);
    auto *view = widget("sample.waveform");
    gain_test::set(p, "o5position", .1f);
    gain_test::set(p, "o5end", .9f);
    auto event = [&](Point<float> at) {
        return MouseEvent(
            Desktop::getInstance().getMainMouseSource(), at,
            ModifierKeys(ModifierKeys::leftButtonModifier | ModifierKeys::altModifier), 1, 0, 0, 0,
            0, view, view, Time::getCurrentTime(), at, Time::getCurrentTime(), 1, false);
    };
    auto begin = Point<float>(float(view->getWidth()) * .6f, 120);
    auto end = Point<float>(float(view->getWidth()) * .8f, 120);
    view->mouseDown(event(begin));
    view->mouseDrag(event(end));
    view->mouseUp(event(end));
    check(raw("o5tailon") == 1 && std::abs(raw("o5tailstart") - .6f) < 1e-5 &&
              std::abs(raw("o5tailend") - .8f) < 1e-5 && raw("o5position") == .1f &&
              raw("o5end") == .9f,
          "Alt-drag creates a C tail loop independently of sample START/END");
    check(raw("o5loop") == 0, "enabling a tail replaces whole-selection looping");
    auto layout = editor->validateLayout();
    if (layout.isNotEmpty())
        std::cerr << layout << '\n';
    check(layout.isEmpty(), "all 954 controls fit their six-player pages");
    ui.reset();
    for (int slot = 0; slot < playerCount; ++slot) {
        auto id = "o" + String(slot + 1);
        gain_test::set(p, id + "tailon", 1);
        gain_test::set(p, id + "tailstart", .2f + float(slot) * .03f);
        gain_test::set(p, id + "tailend", .7f + float(slot) * .03f);
        gain_test::set(p, id + "tailfade", float(10 + slot));
        gain_test::set(p, id + "detuneextra", float(20 + slot));
    }
    gain_test::set(p, "o5level", .3f);
    gain_test::set(p, "o6level", .4f);
    gain_test::set(p, "mixC", .8f);
    p.state.state.setProperty("uiScope", 3, nullptr);
    TemporaryFile sourceFile(".wav");
    for (int slot : {4, 5}) {
        {
            WavAudioFormat wav;
            auto stream = sourceFile.getFile().createOutputStream();
            stream->setPosition(0);
            stream->truncate();
            std::unique_ptr<AudioFormatWriter> writer(
                wav.createWriterFor(stream.get(), 48000, 2, 24, {}, 0));
            check(bool(writer), "C sample writer exists");
            stream.release();
            AudioBuffer<float> material(2, 8192);
            for (int i = 0; i < 8192; ++i) {
                material.setSample(0, i, .4f * std::sin(float(2 * pi * i / (64 + slot * 16))));
                material.setSample(1, i, .2f * std::cos(float(2 * pi * i / (64 + slot * 16))));
            }
            check(writer->writeFromAudioSampleBuffer(material, 0, 8192), "C stereo sample writes");
        }
        check(p.loadSource(slot, sourceFile.getFile(), slot == 4 ? 128 : 256).wasOk(),
              "C players import independent stereo source material");
    }
    sourceFile.getFile().deleteFile();
    TemporaryFile preset(".logisth");
    check(p.savePresetFile(preset.getFile()).wasOk(), "six-player preset saves");
    auto restored = std::make_unique<LogisthProcessor>();
    check(restored->loadPresetFile(preset.getFile()).wasOk(), "six-player preset loads");
    for (const auto &spec : parameterSpecs)
        check(raw(spec.id) == restored->state.getRawParameterValue(spec.id)->load(),
              "all 954 parameter values survive preset round-trip");
    check(int(restored->state.state["uiScope"]) == 3, "C scope selection round-trips");
    for (int slot : {4, 5})
        check(p.sourceData(slot)->samples == restored->sourceData(slot)->samples &&
                  p.sourceData(slot)->frameSize == restored->sourceData(slot)->frameSize,
              "both C stereo sources and wavetable slicing are embedded losslessly");
    // C receives real streaming analysis pools and contributes actual scope audio.
    gain_test::set(p, "polyphony", 1);
    gain_test::set(p, "o1level", 0);
    for (int slot : {4, 5}) {
        auto id = "o" + String(slot + 1);
        gain_test::set(p, id + "stretchmode", 7);
        gain_test::set(p, id + "ssblock", 20);
        gain_test::set(p, id + "ssoverlap", 1);
    }
    p.prepareToPlay(48000, 512);
    check(p.spectralReady(4) && p.spectralReady(5),
          "both C players prepare independent spectral pools");
    auto audio = processor_phase_test::render(p, 8192, true);
    check(audio.getMagnitude(0, audio.getNumSamples()) > 1e-5f,
          "Part C reaches the processor output with spectral tail playback");
    std::array<float, 512> scope{};
    p.scope.read(3, scope.data(), 512);
    bool cAudio = false;
    for (float value : scope) {
        check(std::isfinite(value), "C scope remains finite");
        cAudio |= std::abs(value) > 1e-6f;
    }
    check(cAudio, "the dedicated C scope contains real C audio");
    // Strip additions from a saved state to reproduce an actual older session.
    MemoryBlock session;
    p.getStateInformation(session);
    auto xml = AudioProcessor::getXmlFromBinary(session.getData(), int(session.getSize()));
    auto old = ValueTree::fromXml(*xml);
    for (int i = old.getNumChildren(); --i >= 0;) {
        auto child = old.getChild(i);
        if (child.hasType("SOURCE") && int(child["slot"]) >= 4) {
            old.removeChild(i, nullptr);
            continue;
        }
        if (child.hasType("PARAM")) {
            bool legacy = false;
            for (int j = 0; j < contract.size(); ++j)
                legacy |= child["id"].toString() == contract[j]["id"].toString();
            if (!legacy)
                old.removeChild(i, nullptr);
        }
    }
    MemoryBlock legacyData;
    AudioProcessor::copyXmlToBinary(*old.createXml(), legacyData);
    check(restored->applyState(legacyData.getData(), int(legacyData.getSize())),
          "older session loads");
    for (int slot = 0; slot < playerCount; ++slot) {
        auto id = "o" + String(slot + 1);
        check(restored->state.getRawParameterValue(id + "tailon")->load() == 0 &&
                  restored->state.getRawParameterValue(id + "detuneextra")->load() == 0,
              "older sessions restore tail-off and zero additive detune");
    }
    check(restored->state.getRawParameterValue("o5level")->load() == 0 &&
              restored->state.getRawParameterValue("o6level")->load() == 0 &&
              restored->state.getRawParameterValue("lfo1dest1cx")->load() == 0,
          "older sessions restore silent C and original routing");
    std::cout << "PASS 0.9.5 integration: 736 frozen host parameters, six players, 200 ct, "
                 "extended choices, tail mouse editing, all-parameter presets and old sessions\n";
}
} // namespace features095_test
