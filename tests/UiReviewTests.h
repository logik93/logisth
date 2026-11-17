#pragma once
#include "PluginEditor.h"
#include "TextRendering.h"
namespace ui_review_test {
using namespace juce;
using namespace logisth;
inline void check(bool value, const char *why) {
    if (!value)
        throw std::runtime_error(why);
}
inline Button *buttonIn(Component *component) {
    if (auto *button = dynamic_cast<Button *>(component))
        return button;
    for (auto *child : component->getChildren())
        if (auto *button = dynamic_cast<Button *>(child); button && button->isVisible())
            return button;
    throw std::runtime_error("missing visible button");
}
inline void click(Button &button) {
    const auto at = button.getLocalBounds().getCentre().toFloat();
    auto event = [&](bool down) {
        return MouseEvent(Desktop::getInstance().getMainMouseSource(), at,
                          ModifierKeys(down ? ModifierKeys::leftButtonModifier : 0), 1, 0, 0, 0, 0,
                          &button, &button, Time::getCurrentTime(), at, Time::getCurrentTime(), 1,
                          false);
    };
    static_cast<Component &>(button).mouseDown(event(true));
    button.createComponentSnapshot(button.getLocalBounds());
    static_cast<Component &>(button).mouseUp(event(false));
}
inline void glyphMasks() {
    LogisthLook look;
    struct Glyphs : Component {
        LogisthLook &look;
        explicit Glyphs(LogisthLook &l) : look(l) {}
        void paint(Graphics &g) override {
            g.fillAll(Colours::black);
            g.setColour(Colours::white);
            g.setFont(look.font(16));
            g.drawText(String::fromUTF8("Zażółć gęślą Rate"), 5, 4, 220, 35,
                       Justification::centredLeft);
            g.drawEllipse(260.25f, 4.75f, 31.5f, 31.5f, 1.2f);
        }
    } glyphs(look);
    Component frame;
    frame.setSize(330, 45);
    frame.addAndMakeVisible(glyphs);
    glyphs.setBounds(frame.getLocalBounds());
    auto smooth = frame.createComponentSnapshot(frame.getLocalBounds());
    glyphs.setCachedComponentImage(new logisth_ui::TextImage(glyphs, true));
    auto aliased = frame.createComponentSnapshot(frame.getLocalBounds());
    auto levels = [](const Image &image, Rectangle<int> area) {
        std::set<int> result;
        for (int y = area.getY(); y < area.getBottom(); ++y)
            for (int x = area.getX(); x < area.getRight(); ++x)
                result.insert(image.getPixelAt(x, y).getRed());
        return result.size();
    };
    const auto smoothLevels = levels(smooth, {5, 4, 220, 35}),
               aliasedLevels = levels(aliased, {5, 4, 220, 35});
    if (smoothLevels <= 5 || aliasedLevels != 2)
        std::cerr << "Glyph gray levels smooth=" << smoothLevels << " off=" << aliasedLevels
                  << '\n';
    check(smoothLevels > 5 && aliasedLevels == 2,
          "font AA Off produces actual one-bit glyph coverage");
    check(levels(aliased, {255, 2, 45, 40}) > 5,
          "font AA Off preserves vector/knob curve antialiasing");
}
inline void folders() {
    TemporaryFile temporary;
    const auto root = temporary.getFile();
    root.createDirectory();
    auto p = std::make_unique<LogisthProcessor>();
    const auto save = root.getChildFile("save"), load = root.getChildFile("load"),
               samples = root.getChildFile("samples"), impulses = root.getChildFile("impulses");
    for (const auto &dir : {save, load, samples, impulses})
        dir.createDirectory();
    const auto preset = save.getChildFile("Test.logisth"),
               loaded = load.getChildFile("Imported.logisth");
    check(p->savePresetFile(preset).wasOk() && preset.copyFileTo(loaded) &&
              p->loadPresetFile(loaded).wasOk(),
          "folder fixture saves and reads presets in independent directories");
    const auto audio = samples.getChildFile("Test.wav"), ir = impulses.getChildFile("Test.wav");
    stereo_review_test::write(audio, stereo_review_test::source(44100, 0, 512), 44100);
    convolution_test::write(ir, convolution_test::impulse(44100));
    check(p->loadSource(0, audio).wasOk() && p->loadImpulse(ir).wasOk(),
          "folder fixture imports sample and IR");
    for (auto pair : {std::pair<UserSettings::Folder, File>{UserSettings::Folder::PresetSave, save},
                      {UserSettings::Folder::PresetOpen, load},
                      {UserSettings::Folder::Sample, samples},
                      {UserSettings::Folder::Impulse, impulses}})
        check(UserSettings::folder(pair.first) == pair.second,
              "each successful file operation remembers its own folder");
    check(p->loadPresetFile(root.getChildFile("missing.logisth")).failed() &&
              p->loadSource(0, root.getChildFile("missing.wav")).failed() &&
              UserSettings::folder(UserSettings::Folder::PresetOpen) == load &&
              UserSettings::folder(UserSettings::Folder::Sample) == samples,
          "failed loads leave previous chooser folders intact");
    const auto file = UserSettings::preferencesFile();
    UserSettings::useFileForTests(file);
    check(UserSettings::folder(UserSettings::Folder::PresetSave) == save &&
              UserSettings::folder(UserSettings::Folder::PresetOpen) == load &&
              UserSettings::folder(UserSettings::Folder::Sample) == samples &&
              UserSettings::folder(UserSettings::Folder::Impulse) == impulses,
          "dialog folders survive a process-level preference reload");
    root.deleteRecursively();
    check(UserSettings::folder(UserSettings::Folder::Sample).isDirectory(),
          "removed folders fall back to the nearest existing parent");
}
inline void run(const File &report = {}) {
    struct Preferences {
        File old = UserSettings::preferencesFile();
        TemporaryFile file{".json"};
        Preferences() {
            UserSettings::useFileForTests(file.getFile());
        }
        ~Preferences() {
            UserSettings::useFileForTests(old);
        }
    } profile;
    glyphMasks();
    folders();
    auto p = std::make_unique<LogisthProcessor>();
    std::unique_ptr<AudioProcessorEditor> ui(p->createEditor());
    auto *editor = dynamic_cast<LogisthEditor *>(ui.get());
    ui->setSize(1240, 860);
    auto *console = ui->getChildComponent(0);
    check(p->distortionOrder().slots == DistortionOrder{}.slots,
          "new instances start with distortion slots 1/2/3/4");
    const auto originalPoly = p->state.getParameter("polyphonyextra");
    check(parameterCount == 1084 && originalPoly == p->getParameters()[1070] &&
              p->state.getParameter("dist1on") == p->getParameters()[1071],
          "all previous 1071 host slots remain intact; distortion controls are appended");
    auto showRack = [&](int index) {
        buttonIn(console->findChildWithID("rack." + String(index)))->onClick();
    };
    auto writeImage = [&](const String &name) {
        if (report == File{})
            return;
        check(report.createDirectory().wasOk(), "UI report folder");
        FileOutputStream stream(report.getChildFile(name + ".png"));
        stream.setPosition(0);
        stream.truncate();
        PNGImageFormat png;
        check(png.writeImageToStream(ui->createComponentSnapshot(ui->getLocalBounds()), stream),
              "UI snapshot writes");
    };
    showRack(3);
    gain_test::set(*p, "sync", 1);
    Thread::sleep(55);
    Timer::callPendingTimersSynchronously();
    auto *rate = console->findChildWithID("chorusdivision"),
         *dual = console->findChildWithID("chorusdual");
    ComboBox *selector = nullptr;
    for (auto *c : rate->getChildren())
        if (auto *box = dynamic_cast<ComboBox *>(c))
            selector = box;
    check(selector && rate->isVisible() && dual->isVisible() && rate->getX() == dual->getX() &&
              std::abs(rate->getY() + selector->getBounds().getCentreY() -
                       (dual->getBounds().getCentreY() - 100)) <= 1,
          "chorus RATE and DUAL share their column and rotary-centre grid alignment");
    auto *feedback = console->findChildWithID("chorusfeedback");
    auto *mix = console->findChildWithID("chorus");
    check(feedback->getX() > mix->getX() && feedback->getY() == mix->getY() &&
              rate->getX() > feedback->getX(),
          "chorus feedback follows Dry/Wet and RATE/DUAL occupy the far-right column");
    writeImage("chorus-sync");
    gain_test::set(*p, "sync", 0);
    for (int attempt = 0; attempt < 10 && !console->findChildWithID("chorusrate")->isVisible();
         ++attempt) {
        Thread::sleep(55);
        Timer::callPendingTimersSynchronously();
    }
    if (!console->findChildWithID("chorusrate")->isVisible() || rate->isVisible())
        std::cerr << "Chorus sync raw=" << p->state.getRawParameterValue("sync")->load()
                  << " Hz visible=" << console->findChildWithID("chorusrate")->isVisible()
                  << " division visible=" << rate->isVisible() << '\n';
    check(console->findChildWithID("chorusrate")->isVisible() && !rate->isVisible(),
          "free chorus rate preserves the shared submenu grid");
    writeImage("chorus-free");
    showRack(1);
    for (int slot = 0; slot < 4; ++slot) {
        auto *field = console->findChildWithID("dist" + String(slot + 1) + "on");
        auto *toggle = buttonIn(field);
        for (int i = 0; i < 4; ++i) {
            const auto before =
                p->state.getRawParameterValue("dist" + String(slot + 1) + "on")->load();
            click(*toggle);
            check(p->state.getRawParameterValue("dist" + String(slot + 1) + "on")->load() != before,
                  "every distortion stage responds on the first pointer click");
        }
        buttonIn(console->findChildWithID("dist.slot." + String(slot)))->onClick();
        const auto type = slot == 0 ? String("disttype") : "dist" + String(slot + 1) + "type";
        check(console->findChildWithID(type)->isVisible(), "each slot has its own editable model");
    }
    auto *last = console->findChildWithID("dist.slot.3");
    check(last->keyPressed(KeyPress(KeyPress::leftKey, ModifierKeys::altModifier, 0)) &&
              p->distortionOrder().slots[2] == 3,
          "distortion keyboard move changes actual saved order");
    const auto start = Point<float>{float(last->getWidth() - 4), 12};
    auto drag = [&](Point<float> at) {
        return MouseEvent(Desktop::getInstance().getMainMouseSource(), at,
                          ModifierKeys::leftButtonModifier, 1, 0, 0, 0, 0, last, last,
                          Time::getCurrentTime(), start, Time::getCurrentTime(), 1, true);
    };
    last->mouseDown(drag(start));
    last->mouseDrag(drag({float(430 - last->getX()), 12}));
    last->mouseUp(drag(start));
    check(p->distortionOrder().slots[0] == 3 && last->getX() == 414,
          "distortion drag moves the slot and its real processing position");
    gain_test::set(*p, "dist2on", 1);
    gain_test::set(*p, "dist2type", 1);
    gain_test::set(*p, "dist2drive", .37f);
    TemporaryFile saved(".logisth");
    check(p->savePresetFile(saved.getFile()).wasOk(), "multi-distortion preset saves");
    auto restored = std::make_unique<LogisthProcessor>();
    check(restored->loadPresetFile(saved.getFile()).wasOk() &&
              restored->distortionOrder().slots == p->distortionOrder().slots &&
              std::abs(restored->state.getRawParameterValue("dist2drive")->load() - .37f) < 1e-6,
          "all independent stage settings/order survive portable presets");
    MemoryBlock state;
    p->getStateInformation(state);
    auto tree = ValueTree::fromXml(
        *AudioProcessor::getXmlFromBinary(state.getData(), int(state.getSize())));
    const auto before = restored->distortionOrder();
    tree.setProperty("distortionOrder", "not-an-order", nullptr);
    AudioProcessor::copyXmlToBinary(*tree.createXml(), state);
    check(!restored->applyState(state.getData(), int(state.getSize())) &&
              restored->distortionOrder().slots == before.slots,
          "invalid stage order rejects transactionally");
    tree.removeProperty("distortionOrder", nullptr);
    AudioProcessor::copyXmlToBinary(*tree.createXml(), state);
    check(restored->applyState(state.getData(), int(state.getSize())) &&
              restored->distortionOrder().slots == DistortionOrder{}.slots,
          "older presets use original stage order");
    for (int i = tree.getNumChildren() - 1; i >= 0; --i) {
        const auto id = tree.getChild(i)["id"].toString();
        if (id == "dist1on" || id.startsWith("dist2") || id.startsWith("dist3") ||
            id.startsWith("dist4"))
            tree.removeChild(i, nullptr);
    }
    AudioProcessor::copyXmlToBinary(*tree.createXml(), state);
    check(restored->applyState(state.getData(), int(state.getSize())) &&
              restored->state.getRawParameterValue("dist1on")->load() == 1 &&
              restored->state.getRawParameterValue("dist2on")->load() == 0 &&
              restored->state.getRawParameterValue("dist3on")->load() == 0 &&
              restored->state.getRawParameterValue("dist4on")->load() == 0,
          "genuine older states reset added stages rather than retaining the previous patch");
    writeImage("distortion-rack-reordered");
    p->loadPreset(0);
    check(p->distortionOrder().slots == DistortionOrder{}.slots,
          "Init resets reordered distortion slots to 1/2/3/4");
    gain_test::set(*p, "diston", 1);
    gain_test::set(*p, "dist2on", 1);
    gain_test::set(*p, "dist2type", 1);
    gain_test::set(*p, "dist2drive", .37f);
    buttonIn(console->findChildWithID("dist.slot.0"))->onClick();
    writeImage("distortion-rack");
    showRack(4);
    auto *analysis = dynamic_cast<ComboBox *>(console->findChildWithID("eq.analyzer.mode"));
    auto *tilt = dynamic_cast<ComboBox *>(console->findChildWithID("eq.analyzer.tilt"));
    check(analysis && tilt && analysis->getNumItems() == 4 && tilt->getNumItems() == 5,
          "EQ exposes Off/Pre/Post/Both and all five tilt values");
    analysis->setSelectedId(1, sendNotificationSync);
    check(!p->eqSpectrumEnabled.load() && UserSettings::analyzer().mode == 0,
          "EQ analyzer Off stops capture and persists immediately");
    analysis->setSelectedId(4, sendNotificationSync);
    tilt->setSelectedId(5, sendNotificationSync);
    check(UserSettings::analyzer().mode == 3 && UserSettings::analyzer().tilt == 4,
          "analyzer mode and slope persist");
    gain_test::set(*p, "eqon", 1);
    gain_test::set(*p, "eqmode", 1);
    gain_test::set(*p, "eqband3", -9);
    gain_test::set(*p, "eqband7", 6);
    gain_test::set(*p, "limiton", 1);
    p->prepareToPlay(48000, 512);
    p->eqSpectrumEnabled = true;
    processor_phase_test::render(*p, 8192, true);
    Thread::sleep(55);
    Timer::callPendingTimersSynchronously();
    writeImage("eq-analyzer");
    buttonIn(console->findChildWithID("ui.settings"))->onClick();
    auto *panel = console->findChildWithID("settings.panel");
    for (int tab = 0; tab < 7; ++tab) {
        buttonIn(panel->findChildWithID("settings.tab." + String(tab)))->onClick();
        for (auto *a : panel->getChildren())
            if (a->isVisible()) {
                check(panel->getLocalBounds().contains(a->getBounds()),
                      "categorized settings stay inside the central window");
                for (auto *b : panel->getChildren())
                    if (b != a && b->isVisible())
                        check(!a->getBounds().intersects(b->getBounds()),
                              "settings categories have no overlapping controls");
            }
        writeImage("settings-" + String(tab));
    }
    buttonIn(panel->findChildWithID("settings.tab.4"))->onClick();
    auto *size = dynamic_cast<ComboBox *>(panel->findChildWithID("settings.text.size"));
    auto *weight = dynamic_cast<ComboBox *>(panel->findChildWithID("settings.text.style"));
    auto *aa = dynamic_cast<ToggleButton *>(panel->findChildWithID("settings.text.antialias"));
    auto &look = dynamic_cast<LogisthLook &>(ui->getLookAndFeel());
    auto *notes = dynamic_cast<TextEditor *>(console->findChildWithID("preset.comment"));
    const auto originalHeight = notes->getFont().getHeight();
    size->setText("125%", sendNotificationSync);
    check(UserSettings::text().size == 125 &&
              std::abs(notes->getFont().getHeight() / originalHeight - 1.25) < .001,
          "typed font size updates existing editable text independently of window size");
    weight->setSelectedId(5, sendNotificationSync);
    check(look.font(12).isItalic() && console->getCachedComponentImage(),
          "bold italic text uses the glyph renderer");
    click(*aa);
    check(!UserSettings::text().antialias && console->getCachedComponentImage(),
          "AA can be disabled from Settings");
    writeImage("settings-text-large-aliased");
    weight->setSelectedId(1, sendNotificationSync);
    size->setText("100%", sendNotificationSync);
    click(*aa);
    check(std::abs(notes->getFont().getHeight() - originalHeight) < .001 &&
              console->getCachedComponentImage() && UserSettings::text().quality == 2,
          "resetting text preferences restores original size and default 2x glyph renderer");
    size->setText("999%", sendNotificationSync);
    check(UserSettings::text().size == 100, "out-of-range font size is rejected");
    const auto profileFile = profile.file.getFile();
    UserSettings::setText({110, 2, false});
    UserSettings::useFileForTests(profileFile);
    check(UserSettings::text().size == 110 && UserSettings::text().style == 2 &&
              !UserSettings::text().antialias && UserSettings::analyzer().mode == 3 &&
              UserSettings::analyzer().tilt == 4,
          "font and analyzer preferences reload across application instances");
    UserSettings::setText({});
    Thread::sleep(55);
    Timer::callPendingTimersSynchronously();
    check(editor->validateLayout().isEmpty(),
          "all 1084 parameters remain reachable after the additions");
    ui.reset();
    auto recreated = std::unique_ptr<AudioProcessorEditor>(p->createEditor());
    auto *newPanel = recreated->getChildComponent(0)->findChildWithID("settings.panel");
    check(newPanel->findChildWithID("settings.about.logo") != nullptr,
          "provided SVG is installed in About");
    std::cout
        << "PASS UI review: chorus grid, four pointer-responsive distortion slots/order/state, EQ "
           "controls, seven settings categories and persistent typography\n";
}
} // namespace ui_review_test
