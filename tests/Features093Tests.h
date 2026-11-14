#pragma once
#include "ProcessorPhaseTests.h"
namespace features093_test {
inline void run() {
    using namespace juce;
    using namespace logisth;
    auto check = [](bool ok, const char *message) {
        if (!ok)
            throw std::runtime_error(message);
    };
    LogisthProcessor p;
    gain_test::set(p, "m1target1", 1);
    gain_test::set(p, "m1low1", .2f);
    gain_test::set(p, "m1high1", .8f);
    gain_test::set(p, "macro1", .5f);
    check(std::abs(p.effectiveValue(Param::filtercutoff)) < 1e-4,
          "macro routing reaches the DSP parameter snapshot");
    p.setMacroName(0, "Kick motion / Ω");
    p.setMutationLocks(5);
    TemporaryFile preset(".logisth");
    check(p.savePresetFile(preset.getFile()).wasOk(), "MORPH preset saves");
    LogisthProcessor restored;
    check(restored.loadPresetFile(preset.getFile()).wasOk(), "MORPH preset loads");
    check(restored.macroName(0) == p.macroName(0) && restored.mutationLocks() == 5,
          "macro names and mutation locks persist in presets");
    check(restored.state.getRawParameterValue("m1target1")->load() == 1 &&
              std::abs(restored.effectiveValue(Param::filtercutoff)) < 1e-4,
          "macro ranges and amounts persist");
    p.loadPreset(0);
    std::array<float, parameterCount> before{};
    for (size_t i = 0; i < parameterCount; ++i)
        before[i] = p.state.getParameter(parameterSpecs[i].id)->getValue();
    p.setMutationLocks(15);
    p.mutate(123);
    for (size_t i = 0; i < parameterCount; ++i)
        check(p.state.getParameter(parameterSpecs[i].id)->getValue() == before[i],
              "all section locks protect the entire patch");
    p.undoMutation();
    p.setMutationLocks(1);
    p.mutate(456);
    check(p.canUndoMutation() && p.state.getRawParameterValue("o1phase")->load() == 0,
          "oscillator lock protects oscillator phase");
    check(p.state.getParameter("filtercutoff")->getValue() != before[size_t(Param::filtercutoff)],
          "unlocked filter section mutates");
    p.undoMutation();
    for (size_t i = 0; i < parameterCount; ++i)
        check(std::abs(p.state.getParameter(parameterSpecs[i].id)->getValue() - before[i]) < 1e-6,
              "Undo restores every changed parameter");
    std::unique_ptr<AudioProcessorEditor> ui(p.createEditor());
    auto *editor = dynamic_cast<LogisthEditor *>(ui.get());
    auto *console = ui->getChildComponent(0);
    editor->snapshotPage(23);
    auto *name = dynamic_cast<TextEditor *>(console->findChildWithID("morph.name.0"));
    check(name && name->isVisible(), "MORPH name fields are reachable");
    name->setText("Hardstyle controls", false);
    name->onTextChange();
    check(p.macroName(0) == "Hardstyle controls", "name editing binds to preset metadata");
    for (int macro = 0; macro < 4; ++macro) {
        dynamic_cast<Button *>(console->findChildWithID("morph.select." + String(macro)))
            ->onClick();
        check(console->findChildWithID("m" + String(macro + 1) + "target4")->isVisible(),
              "every macro exposes four routings");
    }
    editor->snapshotPage(19);
    auto *view = console->findChildWithID("sample.waveform");
    check(view && view->isVisible(), "sample range editor is visible");
    auto event = [&](Point<float> at, bool right = false) {
        return MouseEvent(Desktop::getInstance().getMainMouseSource(), at,
                          ModifierKeys(right ? ModifierKeys::rightButtonModifier
                                             : ModifierKeys::leftButtonModifier),
                          1, 0, 0, 0, 0, view, view, Time::getCurrentTime(), at,
                          Time::getCurrentTime(), 1, false);
    };
    MouseWheelDetails wheel;
    wheel.deltaY = .1f;
    view->mouseWheelMove(event({float(view->getWidth()) * .5f, 90}), wheel);
    view->mouseDown(event({float(view->getWidth()) * .2f, 90}));
    view->mouseDrag(event({float(view->getWidth()) * .6f, 90}));
    view->mouseUp(event({float(view->getWidth()) * .6f, 90}));
    check(std::abs(p.state.getRawParameterValue("o1position")->load() - .3f) < 1e-5 &&
              std::abs(p.state.getRawParameterValue("o1end")->load() - float(1. / 6 + .4)) < 1e-5,
          "mouse selection maps START/END correctly after wheel zoom");
    view->mouseDoubleClick(event({200, 90}));
    view->mouseDown(event({0, 90}));
    view->mouseDrag(event({float(view->getWidth()) * .1f, 90}));
    view->mouseUp(event({float(view->getWidth()) * .1f, 90}));
    check(std::abs(p.state.getRawParameterValue("o1position")->load() - .3266667f) < 1e-5,
          "start handle trims inside a fitted selection");
    view->mouseDown(event({200, 90}, true));
    check(p.state.getRawParameterValue("o1position")->load() == 0 &&
              p.state.getRawParameterValue("o1end")->load() == 1,
          "right click resets full range");
    editor->snapshotPage(24);
    for (auto id : {"o1fmsource", "o1amsource", "o1pwmsync", "o1pwmdivision"})
        check(console->findChildWithID(id)->isVisible(),
              "new routing controls fit in their source page");
    auto error = editor->validateLayout();
    if (error.isNotEmpty())
        std::cerr << error << '\n';
    check(error.isEmpty(), "every 0.9.3 parameter is reachable without overlap");
    editor->setTheme(5);
    check(dynamic_cast<LogisthLook &>(ui->getLookAndFeel()).skin == "ap-flexin",
          "AP skin selects its supplied embedded white logo");
    ThemeDefinition decoded;
    check(ThemeSettings::decode(ThemeSettings::encode(ThemeSettings::theme(5)), decoded) &&
              decoded.skin == "ap-flexin",
          "AP skin property survives theme encoding");
    ui.reset();
    gain_test::cleanSine(p);
    gain_test::set(p, "o1engine", 2);
    gain_test::set(p, "o1stretchmode", 6);
    gain_test::set(p, "o1kickattack", 25);
    gain_test::set(p, "o1stretch", 1.5f);
    p.prepareToPlay(48000, 256);
    auto kick = processor_phase_test::render(p, 8192, true);
    check(kick.getMagnitude(0, 8192) > .01f, "Kick spectral pool reaches the full processor");
    p.reset();
    auto repeated = processor_phase_test::render(p, 8192, true);
    for (int i = 0; i < 1000; ++i)
        check(std::abs(kick.getSample(0, i) - repeated.getSample(0, i)) < 1e-6,
              "processor reset preserves repeatable protected attacks");
    for (int ch = 0; ch < 2; ++ch)
        for (int i = 0; i < 8192; ++i)
            check(std::isfinite(kick.getSample(ch, i)),
                  "pooled stereo spectral output stays finite");
    std::cout << "PASS 0.9.3 integration: MORPH names/ranges, section locks/Undo, zoomed range "
                 "selection/handles, routing and full layout\n";
}
} // namespace features093_test
