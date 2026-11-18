#pragma once
#include "PluginEditor.h"
namespace features0983_test {
using namespace juce;
using namespace logisth;
inline void check(bool ok, const char *message) {
    if (!ok)
        throw std::runtime_error(message);
}
inline void set(LogisthProcessor &p, const String &id, float value) {
    auto *parameter = p.state.getParameter(id);
    check(parameter != nullptr, "multiband parameter exists");
    parameter->setValueNotifyingHost(parameter->convertTo0to1(value));
}
inline Button &button(Component &c, const String &id) {
    auto *b = dynamic_cast<Button *>(c.findChildWithID(id));
    check(b != nullptr, "0983 button exists");
    return *b;
}
inline void selectChoice(Component &console, const String &id, int selected) {
    auto *field = console.findChildWithID(id);
    check(field != nullptr, "choice parameter widget exists");
    for (auto *child : field->getChildren())
        if (auto *choice = dynamic_cast<ComboBox *>(child)) {
            choice->setSelectedId(selected, sendNotificationSync);
            return;
        }
    check(false, "parameter choice component exists");
}
inline void tick() {
    Thread::sleep(55);
    Timer::callPendingTimersSynchronously();
}
inline MouseEvent event(Component &c, Point<float> at, bool down) {
    return {Desktop::getInstance().getMainMouseSource(),
            at,
            ModifierKeys(down ? ModifierKeys::leftButtonModifier : 0),
            1,
            0,
            0,
            0,
            0,
            &c,
            &c,
            Time::getCurrentTime(),
            at,
            Time::getCurrentTime(),
            1,
            false};
}
inline double pixelDistance(const Image &a, const Image &b, Point<int> at) {
    const auto x = a.getPixelAt(at.x, at.y), y = b.getPixelAt(at.x, at.y);
    return std::abs(int(x.getRed()) - int(y.getRed())) +
           std::abs(int(x.getGreen()) - int(y.getGreen())) +
           std::abs(int(x.getBlue()) - int(y.getBlue()));
}
inline void run(const File &report = {}) {
    struct Profile {
        File theme = ThemeSettings::preferencesFile(), user = UserSettings::preferencesFile();
        TemporaryFile themeFile{".json"}, userFile{".json"};
        File folder;
        Profile() {
            ThemeSettings::useStorageFileForTests(themeFile.getFile());
            UserSettings::useFileForTests(userFile.getFile());
            folder = ThemeSettings::folder();
        }
        ~Profile() {
            ThemeSettings::useStorageFileForTests(theme);
            UserSettings::useFileForTests(user);
            folder.deleteRecursively();
        }
    } profile;
    auto p = std::make_unique<LogisthProcessor>();
    std::unique_ptr<LogisthEditor> ui(dynamic_cast<LogisthEditor *>(p->createEditor()));
    ui->setUIScale(100);
    ui->setModulationCollapsed(false);
    auto *console = ui->getChildComponent(0);
    auto snap = [&] { return ui->createComponentSnapshot(ui->getLocalBounds()); };
    auto write = [&](const String &name) {
        if (report == File{})
            return;
        check(report.createDirectory().wasOk(), "0983 report folder");
        FileOutputStream stream(report.getChildFile(name + ".png"));
        stream.setPosition(0);
        stream.truncate();
        check(PNGImageFormat{}.writeImageToStream(snap(), stream), "0983 screenshot");
    };
    button(*console, "ui.settings").onClick();
    auto *panel = console->findChildWithID("settings.panel");
    button(*panel, "settings.tab.2").onClick();
    auto *style = dynamic_cast<ComboBox *>(panel->findChildWithID("settings.theme.background"));
    auto *gradient = dynamic_cast<Slider *>(panel->findChildWithID("settings.background.gradient"));
    auto *grid = dynamic_cast<Slider *>(panel->findChildWithID("settings.background.grid"));
    check(style && gradient && grid, "new gradient/grid controls exist");
    for (int theme = 0; theme < 10; ++theme) {
        ui->setTheme(theme);
        style->setSelectedId(2, sendNotificationSync);
        gradient->setValue(0, sendNotificationSync);
        const auto zero = snap();
        gradient->setValue(100, sendNotificationSync);
        const auto full = snap();
        // Empty oscillator panel corner; no glyph/knob/focus pixels in either image.
        check(pixelDistance(zero, full, {464, 255}) > 12,
              "gradient visibly changes pixels inside every theme's panels");
        check(pixelDistance(zero, full, {1228, 540}) > 12,
              "gradient visibly changes background in every theme");
        if (theme == 0 || theme == 1 || theme == 5)
            write("gradient-theme-" + String(theme));
    }
    ui->setTheme(1);
    style->setSelectedId(3, sendNotificationSync);
    grid->setValue(0, sendNotificationSync);
    const auto zero = snap();
    write("grid-0");
    grid->setValue(50, sendNotificationSync);
    const auto half = snap();
    write("grid-50");
    grid->setValue(100, sendNotificationSync);
    const auto full = snap();
    write("grid-100");
    const auto halfDistance = pixelDistance(zero, half, {24, 200}),
               fullDistance = pixelDistance(zero, full, {24, 200});
    check(halfDistance > 0 && fullDistance > halfDistance,
          "grid opacity changes monotonically from zero to full strength");
    struct Visibility : ComponentListener {
        int changes = 0;
        void componentVisibilityChanged(Component &) override {
            ++changes;
        }
    } probe;
    grid->addComponentListener(&probe);
    grid->mouseDown(event(*grid, {15, 13}, true));
    grid->mouseDrag(event(*grid, {70, 13}, true));
    tick();
    grid->mouseDrag(event(*grid, {100, 13}, true));
    grid->mouseUp(event(*grid, {100, 13}, false));
    grid->removeComponentListener(&probe);
    check(probe.changes == 0, "grid drag never hides/rebuilds its held slider");
    gradient->setValue(37, sendNotificationSync);
    grid->setValue(61, sendNotificationSync);
    check(p->appearanceDraft && p->appearanceDraft->edited.gradientAmount == .37 &&
              p->appearanceDraft->edited.gridOpacity == .61,
          "temporary strength settings are retained as an appearance draft");
    const auto chosen = p->appearanceDraft->edited;
    auto file = profile.folder.getChildFile("custom-strengths.json");
    check(ThemeSettings::saveTo(chosen, file).wasOk(), "gradient/grid strengths save to theme");
    ThemeDefinition restored;
    check(ThemeSettings::decode(JSON::parse(file.loadFileAsString()), restored) &&
              restored.gradientAmount == .37 && restored.gridOpacity == .61,
          "gradient/grid strengths reload from a real theme file");
    auto old = ThemeSettings::encode(chosen);
    old.getDynamicObject()->removeProperty("gradientAmount");
    old.getDynamicObject()->removeProperty("gridOpacity");
    check(ThemeSettings::decode(old, restored) && restored.gradientAmount == .75 &&
              restored.gridOpacity == .75,
          "older theme files retain compatible default strengths");
    button(*panel, "settings.tab.3").onClick();
    auto *credits = dynamic_cast<TextEditor *>(panel->findChildWithID("settings.about.credits"));
    check(credits &&
              credits->getText().contains("mixedbytrzy - Tester, suggested hiding the modulation "
                                          "menu and contributed many other ideas."),
          "About includes the exact tester credit");
    write("credits");
    button(*console, "ui.settings").onClick();
    button(*console, "rack.1").onClick();
    check(ui->validateLayout().isEmpty(), "all 1089 parameters remain reachable without overlaps");
    selectChoice(*console, "distbandmode", 2);
    set(*p, "diston", 1);
    for (int count = 1; count <= 4; ++count) {
        selectChoice(*console, "distbands", count);
        tick();
        auto *graph = console->findChildWithID("distortion.graph");
        check(graph && graph->isVisible(), "multiband graph is visible");
        for (int cut = 0; cut < 3; ++cut) {
            auto *field = console->findChildWithID("distcross" + String(cut + 1));
            if (field->isVisible() != (cut < count - 1)) {
                std::cerr << "Count " << count << " raw "
                          << p->state.getRawParameterValue("distbands")->load() << " cut " << cut
                          << " visible " << field->isVisible() << "\n";
                write("debug-count-" + String(count));
            }
            check(field->isVisible() == (cut < count - 1),
                  "only active crossover controls are shown");
        }
        auto *analysis =
            dynamic_cast<ComboBox *>(console->findChildWithID("distortion.analyzer.mode"));
        check(analysis && analysis->isVisible(), "multiband pre/post selector is visible");
        for (auto *widget : console->getChildren())
            if (widget->isVisible())
                check(console->getLocalBounds().contains(widget->getBounds()),
                      "multiband controls stay within the window");
        write("multiband-" + String(count));
    }
    auto *graph = console->findChildWithID("distortion.graph");
    set(*p, "distcross1", 200);
    set(*p, "distcross2", 2000);
    set(*p, "distcross3", 8000);
    tick();
    const float start = 6 + float(std::log(200. / 20) / std::log(1000.)) * (graph->getWidth() - 12);
    graph->mouseDown(event(*graph, {start, 15}, true));
    graph->mouseDrag(event(*graph, {start + 20, 15}, true));
    tick();
    graph->mouseUp(event(*graph, {start + 20, 15}, false));
    check(p->state.getRawParameterValue("distcross1")->load() > 200,
          "dragging a crossover changes its host parameter on the first gesture");
    ui->setModulationCollapsed(true);
    ui->setUIScale(125);
    write("multiband-compact-125");
    ui->setUIScale(100);
    ui->setModulationCollapsed(false);
    auto *analysis = dynamic_cast<ComboBox *>(console->findChildWithID("distortion.analyzer.mode"));
    auto *tilt = dynamic_cast<ComboBox *>(console->findChildWithID("distortion.analyzer.tilt"));
    const auto eqMode = UserSettings::analyzer();
    analysis->setSelectedId(1, sendNotificationSync);
    check(UserSettings::distortionAnalyzer().mode == 0 &&
              UserSettings::analyzer().mode == eqMode.mode,
          "distortion spectrum has an independent off preference");
    tilt->setSelectedId(5, sendNotificationSync);
    analysis->setSelectedId(4, sendNotificationSync);
    p->prepareToPlay(48000, 512);
    p->distortionSpectrumEnabled = true;
    set(*p, "drive", .7f);
    set(*p, "dist2on", 1);
    set(*p, "dist3on", 1);
    set(*p, "dist4on", 1);
    AudioBuffer<float> audio(2, 512);
    MidiBuffer midi;
    midi.ensureSize(4096);
    midi.addEvent(MidiMessage::noteOn(1, 57, .7f), 0);
    for (int block = 0; block < 32; ++block) {
        p->processBlock(audio, midi);
        midi.clear();
    }
    check(p->distortionSpectrum.count() > 2048,
          "pre/post telemetry captures actual distortion-stage audio");
    std::array<float, 2048> pre{}, post{};
    p->distortionSpectrum.read(0, pre.data(), 2048);
    p->distortionSpectrum.read(2, post.data(), 2048);
    double difference = 0;
    for (size_t i = 0; i < pre.size(); ++i)
        difference += std::abs(pre[i] - post[i]);
    check(difference > 1, "pre/post streams show the processed signal difference");
    tick();
    write("multiband-spectrum");
    audio_allocation_probe::begin();
    for (int block = 0; block < 16; ++block)
        p->processBlock(audio, midi);
    const auto allocations = audio_allocation_probe::end();
    check(allocations == 0, "multiband and spectrum capture never allocate on the audio thread");
    analysis->setSelectedId(1, sendNotificationSync);
    tick();
    check(!p->distortionSpectrumEnabled.load(), "spectrum off disables audio capture");
    MemoryBlock saved;
    p->getStateInformation(saved);
    auto copy = std::make_unique<LogisthProcessor>();
    copy->setStateInformation(saved.getData(), int(saved.getSize()));
    check(copy->state.getRawParameterValue("distbandmode")->load() == 1 &&
              copy->state.getRawParameterValue("distbands")->load() == 3 &&
              copy->state.getRawParameterValue("distcross1")->load() ==
                  p->state.getRawParameterValue("distcross1")->load(),
          "presets/project state retain multiband routing and frequencies");
    auto legacy = ValueTree::fromXml(
        *AudioProcessor::getXmlFromBinary(saved.getData(), int(saved.getSize())));
    for (int i = legacy.getNumChildren() - 1; i >= 0; --i)
        if (legacy.getChild(i)["id"].toString() == "distbandmode" ||
            legacy.getChild(i)["id"].toString() == "distbands" ||
            legacy.getChild(i)["id"].toString().startsWith("distcross"))
            legacy.removeChild(i, nullptr);
    MemoryBlock oldState;
    AudioProcessor::copyXmlToBinary(*legacy.createXml(), oldState);
    check(copy->applyState(oldState.getData(), int(oldState.getSize())),
          "old preset fixture is valid and loads successfully");
    check(copy->state.getRawParameterValue("distbandmode")->load() == 0 &&
              copy->state.getRawParameterValue("distbands")->load() == 3 &&
              std::abs(copy->state.getRawParameterValue("distcross1")->load() - 200) < .01f,
          "loading older presets resets added routing parameters to Legacy defaults");
    selectChoice(*console, "distbandmode", 1);
    tick();
    check(!graph->isVisible() && !p->distortionSpectrumEnabled.load(),
          "Legacy restores the original serial editor and stops multiband analysis");
    write("legacy");
    ui.reset();
    check(!p->distortionSpectrumEnabled.load(), "closing the editor stops distortion telemetry");
    std::cout << "PASS 0.9.8.3: visible gradients in ten themes, grid intensity/drag/draft/save, "
                 "credits, multiband UI/crossover gesture, pre/post/off, allocation-free "
                 "processing and legacy preset compatibility\n";
}
} // namespace features0983_test
