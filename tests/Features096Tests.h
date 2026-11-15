#pragma once
#include "ProcessorPhaseTests.h"
namespace features096_test {
inline void run() {
    using namespace juce;
    using namespace logisth;
    auto check = [](bool ok, const char *m) {
        if (!ok)
            throw std::runtime_error(m);
    };
    auto contract = JSON::parse(
        File(String(LOGISTH_SOURCE_DIR)).getChildFile("tests/fixtures/parameters-0.9.5.json"));
    check(contract.isArray() && contract.size() == 954, "complete 0.9.5 parameter contract exists");
    auto instance = std::make_unique<LogisthProcessor>();
    auto &p = *instance;
    for (int i = 0; i < contract.size(); ++i) {
        auto row = contract[i];
        const auto &s = parameterSpecs[size_t(i)];
        const String id(s.id);
        const bool conv = id == "reverbtype",
                   macro = id.startsWith("m") && id.contains("target") && id.endsWith("cx");
        const String extra = conv    ? "|Convolution"
                             : macro ? "|Cutoff A|Cutoff B|Resonance A|Resonance B|Mix A|Mix B"
                                     : "";
        check(row["id"].toString() == s.id && int(row["index"]) == i &&
                  row["choices"].toString() + extra == s.choices &&
                  std::abs(double(row["low"]) - s.low) < 1e-6 &&
                  std::abs(double(row["high"]) - (s.high - (conv    ? 1
                                                            : macro ? 6
                                                                    : 0))) < 1e-6 &&
                  std::abs(double(row["initial"]) - s.initial) < 1e-6 &&
                  std::abs(double(row["skew"]) - s.skew) < 1e-6 &&
                  p.getParameters()[i] == p.state.getParameter(s.id),
              "all original 954 ranges, choices, defaults and host indices remain intact");
    }
    check(parameterCount == 1055, "0.9.6 additions are append-only");
    std::unique_ptr<AudioProcessorEditor> ui(p.createEditor());
    auto *editor = dynamic_cast<LogisthEditor *>(ui.get());
    auto *console = ui->getChildComponent(0);
    auto component = [&](const String &id) {
        auto *c = console->findChildWithID(id);
        check(c != nullptr, "0.9.6 component exists");
        return c;
    };
    auto knob = [&](const String &id) {
        Slider *found = nullptr;
        for (auto *c : component(id)->getChildren())
            if (auto *s = dynamic_cast<Slider *>(c))
                found = s;
        check(found != nullptr, "0.9.6 knob exists");
        return found;
    };
    auto menu = [&](const String &id) {
        auto *found = dynamic_cast<ComboBox *>(component(id));
        if (!found)
            for (auto *c : component(id)->getChildren())
                if (auto *m = dynamic_cast<ComboBox *>(c))
                    found = m;
        check(found != nullptr, "0.9.6 menu exists");
        return found;
    };
    auto raw = [&](const String &id) { return p.state.getRawParameterValue(id)->load(); };
    for (int part = 0; part < partCount; ++part) {
        dynamic_cast<Button *>(component("part." + String(part)))->onClick();
        for (int side = 0; side < 2; ++side) {
            int slot = part * 2 + side;
            String id = "o" + String(slot + 1);
            auto *detune = knob(id + "detune");
            check(detune->getMaximum() == 400 &&
                      std::abs(detune->proportionOfLengthToValue(.5) - 100) < .01,
                  "400 ct knob has precise musical low-range control");
            detune->setValue(375, sendNotificationSync);
            check(raw(id + "detune") == 100 && raw(id + "detuneextra") == 100 &&
                      raw(id + "detunewide") == 175,
                  "wide detune combines three compatible ranges");
            detune->setValue(120, sendNotificationSync);
            check(raw(id + "detuneextra") == 20 && raw(id + "detunewide") == 0,
                  "lowering detune clears unused extensions");
            auto *blend = knob(id + "blend");
            check(blend->isVisible() && raw(id + "blend") == 1,
                  "blend defaults to full legacy unison");
            blend->setValue(.3, sendNotificationSync);
            check(std::abs(raw(id + "blend") - .3) < 1e-5,
                  "every blend reaches its independent host parameter");
        }
    }
    auto *route = menu("lfo1dest1");
    for (int target = 0; target < int(Destination::Count); ++target) {
        route->setSelectedId(target + 1, sendNotificationSync);
        check(
            route->getSelectedId() == target + 1 && route->getText().isNotEmpty() &&
                p.effectiveValue(Param::lfo1dest1) == float(target),
            "every grouped-menu leaf maps to the correct destination across all companion ranges");
    }
    route->setSelectedId(int(Destination::CutoffAC) + 1, sendNotificationSync);
    check(p.effectiveValue(Param::lfo1dest1) == float(Destination::CutoffAC) &&
              route->getText().contains("A+C") && route->getText().contains("Cutoff"),
          "grouped menu retains a clear selected family and A+C scope");
    route->setSelectedId(int(Destination::PositionBC) + 1, sendNotificationSync);
    check(p.effectiveValue(Param::lfo1dest1) == float(Destination::PositionBC),
          "B+C position is routable");
    route->setSelectedId(int(Destination::PositionC1) + 1, sendNotificationSync);
    check(raw("lfo1dest1cx2") == 0 &&
              p.effectiveValue(Param::lfo1dest1) == float(Destination::PositionC1),
          "return to 0.9.5 destinations clears paired extension");
    route->setSelectedId(int(Destination::CutoffAB) + 1, sendNotificationSync);
    check(raw("lfo1dest1cx") == 0 && raw("lfo1dest1cx2") == 0,
          "legacy menu choices clear both overrides");
    auto *settings = dynamic_cast<Button *>(component("ui.settings"));
    settings->onClick();
    auto *panel = component("settings.panel");
    auto *fixed = dynamic_cast<ToggleButton *>(panel->findChildWithID("settings.fixedvelocity"));
    check(fixed && fixed->isVisible() && !fixed->getToggleState(),
          "Settings exposes optional fixed velocity, disabled by default");
    fixed->setToggleState(true, sendNotificationSync);
    check(raw("fixedvelocity") == 1, "fixed velocity setting binds to state");
    dynamic_cast<Button *>(panel->findChildWithID("settings.tab.3"))->onClick();
    auto *credits = dynamic_cast<TextEditor *>(panel->findChildWithID("settings.about.credits"));
    check(credits && credits->isVisible() && credits->isReadOnly(),
          "About is a scrollable read-only text area");
    for (auto text :
         {"Logik - Prompt engineer, tester, and everything in between.",
          "coldwave - No. 1 tester with a head full of ideas.",
          "Moander - Tester with a head full of ideas.",
          "bokkenmmb - Tester, idea guy, and resident theme wizard (yes, the FoRReSkin one)."})
        check(credits->getText().contains(text), "all requested credits are included exactly");
    credits->moveCaretToEnd();
    bool scrolled = false;
    for (auto *c : credits->getChildren())
        if (auto *v = dynamic_cast<Viewport *>(c))
            scrolled |= v->getViewPositionY() > 0;
    check(scrolled, "credits can scroll to their final lines");
    settings->onClick();
    editor->snapshotPage(22);
    auto *band = menu("eq.band.select");
    check(band->getNumItems() == 16, "graphic EQ offers sixteen band slots");
    dynamic_cast<Button *>(component("eq.band.add"))->onClick();
    check(raw("eqb10on") == 1 && band->getSelectedId() == 11,
          "Add enables and selects the first extra band");
    auto *type = menu("eqb10type");
    check(type->getNumItems() == 8, "each band offers eight filter shapes");
    type->setSelectedId(4, sendNotificationSync);
    check(raw("eqb10type") == 3, "per-band filter choice updates audio state");
    knob("eqb10freq")->setValue(120, sendNotificationSync);
    knob("eqb10q")->setValue(.8, sendNotificationSync);
    dynamic_cast<Button *>(component("eq.band.remove"))->onClick();
    check(raw("eqb10on") == 0, "Remove bypasses the selected band");
    for (int i = 0; i < 16; ++i) {
        band->setSelectedId(i + 1, sendNotificationSync);
        check(component("eqb" + String(i) + "type")->isVisible() &&
                  component("eqband" + String(i))->isVisible(),
              "all bands share the same compact inspector");
    }
    auto layout = editor->validateLayout();
    if (layout.isNotEmpty())
        std::cerr << layout << '\n';
    check(layout.isEmpty(), "all 1053 controls are reachable without overlap");
    ui.reset();
    TemporaryFile preset(".logisth");
    check(p.savePresetFile(preset.getFile()).wasOk(), "0.9.6 preset saves");
    auto restored = std::make_unique<LogisthProcessor>();
    check(restored->loadPresetFile(preset.getFile()).wasOk(), "0.9.6 preset restores");
    for (const auto &s : parameterSpecs)
        check(std::abs(raw(s.id) - restored->state.getRawParameterValue(s.id)->load()) < 1e-5,
              "all new routing, blend, EQ and Settings values round-trip");
    // Fixed velocity applies to generated MIDI as well as the engine, including the arp.
    gain_test::set(p, "arpout", 1);
    gain_test::set(p, "arpon", 1);
    gain_test::set(p, "fixedvelocity", 1);
    gain_test::set(p, "arppattern", 0);
    p.prepareToPlay(48000, 512);
    MidiBuffer midi;
    AudioBuffer<float> audio(2, 512);
    midi.addEvent(MidiMessage::noteOn(1, 60, .1f), 0);
    bool note = false;
    for (int i = 0; i < 50; ++i) {
        p.processBlock(audio, midi);
        for (auto event : midi)
            if (event.getMessage().isNoteOn()) {
                note = true;
                check(event.getMessage().getVelocity() == 127,
                      "fixed mode sends maximum note velocity through arp MIDI out");
            }
        midi.clear();
    }
    check(note, "fixed MIDI fixture generated a note");
    gain_test::set(p, "o5engine", 2);
    gain_test::set(p, "o5stretchmode", 7);
    gain_test::set(p, "o5voices", 2);
    gain_test::set(p, "o5blend", 0);
    gain_test::set(p, "o5level", .2f);
    gain_test::set(p, "polyphony", 1);
    p.prepareToPlay(48000, 512);
    check(p.spectralReady(4), "even unison prepares its additional dry-center spectral stream");
    auto c = processor_phase_test::render(p, 8192, true);
    check(c.getMagnitude(0, 8192) > 1e-6, "spectral C player remains audible at zero blend");
    std::cout << "PASS 0.9.6 integration: frozen 954-parameter contract, six 400ct/blend controls, "
                 "paired menus, velocity/MIDI, scrollable credits, 16 EQ bands and presets\n";
}
} // namespace features096_test
