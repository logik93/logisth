#pragma once
#include "BackgroundLibrary.h"
#include "PluginEditor.h"
#include <functional>
#include <string_view>
namespace compact_layout_test {
inline void run(const juce::File &report = {}) {
    using namespace juce;
    using namespace logisth;
    auto check = [](bool ok, const char *message) {
        if (!ok)
            throw std::runtime_error(message);
    };
    struct Profile {
        File previous = ThemeSettings::preferencesFile();
        TemporaryFile file{".json"};
        File themes;
        Profile() {
            ThemeSettings::useStorageFileForTests(file.getFile());
            themes = ThemeSettings::folder();
        }
        ~Profile() {
            ThemeSettings::useStorageFileForTests(previous);
            themes.deleteRecursively();
        }
    } profile;
    auto p = std::make_unique<LogisthProcessor>();
    std::unique_ptr<LogisthEditor> ui(dynamic_cast<LogisthEditor *>(p->createEditor()));
    ui->setUIScale(100);
    auto *console = ui->getChildComponent(0);
    auto *toggle = dynamic_cast<Button *>(console->findChildWithID("ui.modulation"));
    check(toggle != nullptr && toggle->isVisible(), "modulation disclosure is reachable");
    auto write = [&](const String &name) {
        if (report == File{})
            return;
        report.createDirectory();
        auto file = report.getChildFile(name + ".png");
        FileOutputStream stream(file);
        stream.setPosition(0);
        stream.truncate();
        PNGImageFormat{}.writeImageToStream(ui->createComponentSnapshot(ui->getLocalBounds()),
                                            stream);
    };
    const auto initial = ui->createComponentSnapshot(ui->getLocalBounds());
    const auto &look = dynamic_cast<LogisthLook &>(ui->getLookAndFeel());
    check(initial.getPixelAt(320, 0) == look.body, "no highlight line remains at the top edge");
    write("expanded");
    auto event = [](Component &c, bool down) {
        const auto at = c.getLocalBounds().getCentre().toFloat();
        return MouseEvent(Desktop::getInstance().getMainMouseSource(), at,
                          ModifierKeys(down ? ModifierKeys::leftButtonModifier : 0), 1, 0, 0, 0, 0,
                          &c, &c, Time::getCurrentTime(), at, Time::getCurrentTime(), 1, false);
    };
    static_cast<Component &>(*toggle).mouseDown(event(*toggle, true));
    Timer::callPendingTimersSynchronously();
    check(toggle->isDown(), "refresh preserves held disclosure button");
    toggle->createComponentSnapshot(toggle->getLocalBounds());
    static_cast<Component &>(*toggle).mouseUp(event(*toggle, false));
    check(ui->isModulationCollapsed() && ui->getWidth() == 1240 && ui->getHeight() == 640,
          "first click shortens the actual editor by 220 pixels");
    for (auto id : {"env1a", "env2a", "lfo1rate", "lfo2rate", "misc1source"}) {
        auto *field = console->findChildWithID(id);
        check(field != nullptr && !field->isVisible(),
              "all modulation sections are present and hidden together");
    }
    for (auto *c : console->getChildren())
        if (c->isVisible())
            check(console->getLocalBounds().contains(c->getBounds()),
                  "compact widgets stay inside canvas");
    check(ui->validateLayout().isEmpty() && ui->isModulationCollapsed() && ui->getHeight() == 640,
          "all sound parameters remain reachable through disclosure");
    write("compact");
    auto *rate = p->state.getParameter("lfo1rate");
    check(rate != nullptr, "LFO automation parameter exists");
    const auto original = rate->getValue();
    rate->setValueNotifyingHost(.37f);
    for (int theme = 0; theme < 10; ++theme) {
        ui->setTheme(theme);
        for (double scale : {80., 100., 125., 133.33, 200.}) {
            ui->setUIScale(scale);
            for (bool collapsed : {false, true}) {
                ui->setModulationCollapsed(collapsed);
                check(ui->getWidth() == roundToInt(1240 * scale / 100) &&
                          ui->getHeight() ==
                              roundToInt(EditorLayout::height(collapsed) * scale / 100),
                      "both layouts preserve typed scale in every factory theme");
                check(console->getTransform().mat00 == console->getTransform().mat11,
                      "compact resizing preserves uniform text and control proportions");
            }
        }
    }
    check(std::abs(rate->getValue() - .37f) < .001f, "hidden modulation retains host automation");
    rate->setValueNotifyingHost(original);
    ui->setUIScale(133.33);
    ui.reset();
    check(ThemeSettings::window().modulationCollapsed, "compact layout preference is saved");
    ThemeSettings::reload();
    ui.reset(dynamic_cast<LogisthEditor *>(p->createEditor()));
    check(ui->isModulationCollapsed() && ui->uiScale() == 133.33 && ui->getHeight() == 853,
          "compact height and fractional scale survive preference reload and editor recreation");
    ui->setUIScale(100);
    console = ui->getChildComponent(0);
    dynamic_cast<Button *>(console->findChildWithID("ui.settings"))->onClick();
    auto *panel = console->findChildWithID("settings.panel");
    dynamic_cast<Button *>(panel->findChildWithID("settings.tab.2"))->onClick();
    auto *choices = dynamic_cast<ComboBox *>(panel->findChildWithID("settings.background.factory"));
    check(choices && choices->getNumItems() == 20,
          "twenty built-in backgrounds are accessible offline");
    std::set<size_t> unique;
    for (int i = 0; i < 20; ++i) {
        const auto data = BackgroundLibrary::data(i);
        check(!data.isEmpty(), "factory background resource exists");
        auto image = ImageFileFormat::loadFrom(data.getData(), data.getSize());
        check(image.isValid() && image.getWidth() <= 4096 && image.getHeight() <= 4096 &&
                  image.getWidth() >= 1240 && image.getHeight() >= 860,
              "every original background decodes within supported size limits");
        unique.insert(std::hash<std::string_view>{}(
            std::string_view(static_cast<const char *>(data.getData()), data.getSize())));
        const auto &entry = BackgroundLibrary::entries[size_t(i)];
        for (int theme = 0; theme < 10; ++theme)
            if (ThemeSettings::theme(theme).key == entry.theme)
                ui->setTheme(theme);
        choices->setSelectedId(i + 1, sendNotificationSync);
        check(p->appearanceDraft && p->appearanceDraft->edited.backgroundImage ==
                                        Base64::toBase64(data.getData(), data.getSize()),
              "factory selection uses the actual generated original image");
        const auto &draft = p->appearanceDraft->edited;
        ThemeDefinition decoded;
        check(ThemeSettings::decode(ThemeSettings::encode(draft), decoded) &&
                  decoded.backgroundImage == draft.backgroundImage,
              "factory background is portable in a saved theme");
        if (i % 2 == 0)
            write("background-" + String(i / 2));
    }
    check(unique.size() == 20 && BackgroundLibrary::data(-1).isEmpty() &&
              BackgroundLibrary::data(20).isEmpty(),
          "backgrounds are distinct and out-of-range selections are safe");
    const auto selectedArtwork = p->appearanceDraft->edited;
    const auto savedTheme = profile.themes.getChildFile("custom-background.json");
    check(ThemeSettings::saveTo(selectedArtwork, savedTheme).wasOk(),
          "factory background saves to a real portable theme file");
    ThemeDefinition loadedArtwork;
    check(ThemeSettings::decode(JSON::parse(savedTheme.loadFileAsString()), loadedArtwork) &&
              loadedArtwork.backgroundImage == selectedArtwork.backgroundImage &&
              SkinBackground::decode(loadedArtwork.backgroundImage).isValid(),
          "saved factory background reloads from disk without a source-file dependency");
    ui->setModulationCollapsed(false);
    check(ui->getHeight() == 860, "disclosure restores the complete editor");
    std::cout << "PASS compact layout: first-click disclosure, 100 theme/scale/layout cases, "
                 "persistence, hidden automation, 20 portable backgrounds\n";
}
} // namespace compact_layout_test
