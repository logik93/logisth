#pragma once
#include "PluginEditor.h"
#include "SkinBackground.h"
#include "TextRendering.h"
namespace appearance0981_test {
using namespace juce;
using namespace logisth;
inline void check(bool ok, const char *message) {
    if (!ok)
        throw std::runtime_error(message);
}
inline Button &button(Component &c, const String &id) {
    auto *b = dynamic_cast<Button *>(c.findChildWithID(id));
    check(b != nullptr, "appearance control exists");
    return *b;
}
inline bool equalPixels(const Image &a, const Image &b, Rectangle<int> area) {
    Image::BitmapData first(a, Image::BitmapData::readOnly), second(b, Image::BitmapData::readOnly);
    for (int y = area.getY(); y < area.getBottom(); ++y)
        for (int x = area.getX(); x < area.getRight(); ++x)
            if (first.getPixelColour(x, y) != second.getPixelColour(x, y))
                return false;
    return true;
}
inline void typography(const File &report) {
    LogisthLook look;
    struct Back : Component {
        void paint(Graphics &g) override {
            g.fillAll(Colours::black);
        }
    } frame;
    struct Glyphs : Component {
        LogisthLook &look;
        String text = String::fromUTF8("Rate 1/8 Zażółć gęślą");
        explicit Glyphs(LogisthLook &l) : look(l) {}
        void paint(Graphics &g) override {
            g.setColour(Colours::white);
            g.setFont(look.font(16));
            g.drawText(text, 5, 4, 235, 35, Justification::centredLeft);
            g.drawEllipse(260.25f, 4.75f, 31.5f, 31.5f, 1.2f);
            g.drawLine(300.3f, 3.7f, 326.8f, 37.4f, 1.4f);
        }
    } glyphs(look);
    frame.addAndMakeVisible(glyphs);
    glyphs.setBounds(0, 0, 340, 45);
    String csv = "ui_scale,dpi,quality,antialias,gray_levels,vector_identical\n";
    int cases = 0;
    for (float scale : {.8f, 1.f, 1.1f, 1.25f, 1.333f, 2.f})
        for (float dpi : {1.f, 2.f}) {
            glyphs.setTransform(AffineTransform::scale(scale));
            frame.setSize(int(std::ceil(340 * scale)), int(std::ceil(45 * scale)));
            glyphs.setCachedComponentImage(nullptr);
            auto normal = frame.createComponentSnapshot(frame.getLocalBounds(), true, dpi);
            const auto textArea = Rectangle<int>(3, 2, 237, 39)
                                      .toFloat()
                                      .transformedBy(AffineTransform::scale(scale * dpi))
                                      .getSmallestIntegerContainer();
            const auto vectors = Rectangle<int>(252, 1, 82, 42)
                                     .toFloat()
                                     .transformedBy(AffineTransform::scale(scale * dpi))
                                     .getSmallestIntegerContainer();
            for (int quality = 0; quality < 4; ++quality)
                for (bool aa : {false, true}) {
                    UserSettings::Text prefs;
                    prefs.quality = quality;
                    prefs.antialias = aa;
                    glyphs.setCachedComponentImage(new logisth_ui::TextImage(glyphs, prefs));
                    auto image = frame.createComponentSnapshot(frame.getLocalBounds(), true, dpi);
                    check(
                        equalPixels(normal, image, vectors),
                        "text rendering preserves exact vector pixels at fractional UI/DPI scales");
                    std::set<int> levels;
                    Image::BitmapData pixels(image, Image::BitmapData::readOnly);
                    for (int y = textArea.getY(); y < textArea.getBottom(); ++y)
                        for (int x = textArea.getX(); x < textArea.getRight(); ++x)
                            levels.insert(pixels.getPixelColour(x, y).getRed());
                    if (!(aa ? levels.size() > 5 : levels.size() == 2))
                        std::cerr << "Glyph matrix scale=" << scale << " dpi=" << dpi
                                  << " quality=" << quality << " aa=" << aa
                                  << " levels=" << levels.size() << '\n';
                    check(
                        aa ? levels.size() > 5 : levels.size() == 2,
                        "glyph smoothing/Off operate at physical pixel resolution at every scale");
                    csv += String(scale, 3) + "," + String(dpi) + "," + String(quality) + "," +
                           String(int(aa)) + "," + String(int(levels.size())) + ",1\n";
                    ++cases;
                    if (report != File{} && dpi == 1 && quality == 2 && aa) {
                        FileOutputStream out(
                            report.getChildFile("glyph-scale-" + String(scale, 3) + ".png"));
                        PNGImageFormat png;
                        check(png.writeImageToStream(image, out), "glyph report PNG");
                    }
                }
        }
    check(cases == 96, "complete UI/DPI/quality/Off matrix");
    glyphs.setTransform({});
    frame.setSize(340, 45);
    double previous = -1;
    for (double weight : {.5, 1., 2.}) {
        UserSettings::Text prefs;
        prefs.quality = 3;
        prefs.edgeWeight = weight;
        glyphs.setCachedComponentImage(new logisth_ui::TextImage(glyphs, prefs));
        auto image = frame.createComponentSnapshot(frame.getLocalBounds());
        double energy = 0;
        Image::BitmapData pixels(image, Image::BitmapData::readOnly);
        for (int y = 0; y < 44; ++y)
            for (int x = 0; x < 240; ++x)
                energy += pixels.getPixelColour(x, y).getRed();
        check(energy > previous, "50-200% edge weight changes actual glyph coverage monotonically");
        previous = energy;
    }
    UserSettings::Text prefs;
    glyphs.setCachedComponentImage(new logisth_ui::TextImage(glyphs, prefs));
    glyphs.text = "MMMMMMMMMMMMMMMMMMMM";
    frame.createComponentSnapshot(frame.getLocalBounds());
    glyphs.text = "I";
    glyphs.repaint();
    auto changed = frame.createComponentSnapshot(frame.getLocalBounds());
    glyphs.setCachedComponentImage(new logisth_ui::TextImage(glyphs, prefs));
    auto fresh = frame.createComponentSnapshot(frame.getLocalBounds());
    check(equalPixels(changed, fresh, fresh.getBounds()),
          "shorter transparent text leaves no stale bitmap glyphs");
    if (report != File{})
        check(report.getChildFile("text-rendering.csv").replaceWithText(csv), "text matrix CSV");
}
inline void run(const File &report = {}) {
    TemporaryFile themeFile{".json"}, userFile{".json"}, fixture;
    struct Restore {
        File theme = ThemeSettings::preferencesFile(), user = UserSettings::preferencesFile();
        ~Restore() {
            ThemeSettings::useStorageFileForTests(theme);
            UserSettings::useFileForTests(user);
        }
    } restore;
    ThemeSettings::useStorageFileForTests(themeFile.getFile());
    UserSettings::useFileForTests(userFile.getFile());
    if (report != File{})
        check(report.createDirectory().wasOk(), "appearance report directory");
    typography(report);
    auto p = std::make_unique<LogisthProcessor>();
    std::unique_ptr<AudioProcessorEditor> ui(p->createEditor());
    auto &editor = dynamic_cast<LogisthEditor &>(*ui);
    auto &console = *ui->getChildComponent(0);
    ui->setSize(1240, 860);
    auto write = [&](const String &name) {
        if (report == File{})
            return;
        FileOutputStream out(report.getChildFile(name + ".png"));
        PNGImageFormat png;
        check(png.writeImageToStream(ui->createComponentSnapshot(ui->getLocalBounds()), out),
              "appearance UI report");
    };
    button(console, "ui.settings").onClick();
    auto &panel = *console.findChildWithID("settings.panel");
    button(panel, "settings.tab.4").onClick();
    auto &quality = *dynamic_cast<ComboBox *>(panel.findChildWithID("settings.text.quality"));
    auto &edge = *dynamic_cast<Slider *>(panel.findChildWithID("settings.text.edge"));
    quality.setSelectedId(1, sendNotificationSync);
    check(!console.getCachedComponentImage(), "Native neutral style uses the normal renderer");
    edge.setValue(150, sendNotificationSync);
    check(console.getCachedComponentImage() && UserSettings::text().edgeWeight == 1.5,
          "edge weight changes renderer and persisted preferences");
    for (int mode = 1; mode <= 4; ++mode)
        quality.setSelectedId(mode, sendNotificationSync);
    edge.setValue(175, sendNotificationSync);
    const auto preferences = UserSettings::preferencesFile();
    UserSettings::useFileForTests(preferences);
    check(UserSettings::text().quality == 3 && UserSettings::text().edgeWeight == 1.75,
          "AA quality and edge weight survive preference reload");
    edge.setValue(100, sendNotificationSync);
    quality.setSelectedId(3, sendNotificationSync);
    write("text-settings");
    for (double scale : {80., 110., 125., 133.33, 200.}) {
        editor.setUIScale(scale);
        const auto t = console.getTransform();
        check(t.mat00 == t.mat11, "rounded editor dimensions preserve uniform glyph aspect ratio");
        write("interface-scale-" + String(scale, 2));
    }
    editor.setUIScale(100);
    const auto root = fixture.getFile();
    check(root.createDirectory().wasOk(), "folder fixtures");
    button(panel, "settings.tab.6").onClick();
    for (int i = 0; i < 5; ++i) {
        const auto folder = root.getChildFile("folder-" + String(i));
        folder.createDirectory();
        check(editor.setBrowserFolder(UserSettings::Folder(i), folder),
              "settings accepts each folder kind");
        check(UserSettings::folder(UserSettings::Folder(i)) == folder,
              "settings stores selected browser folder");
        check(button(panel, "settings.folder.choose." + String(i)).isVisible() &&
                  button(panel, "settings.folder.open." + String(i)).isEnabled(),
              "each folder has choose/open controls");
        auto *label = dynamic_cast<Label *>(panel.findChildWithID("settings.folder." + String(i)));
        check(label && label->getText() == folder.getFullPathName(),
              "current selected folder is displayed");
        check(!editor.setBrowserFolder(UserSettings::Folder(i), root.getChildFile("missing")),
              "invalid folder preserves selection");
    }
    check(p->presetDirectory() == UserSettings::folder(UserSettings::Folder::Library),
          "library choice updates the preset browser");
    const auto selectedLibrary = p->presetDirectory();
    auto other = std::make_unique<LogisthProcessor>();
    const auto otherLibrary = root.getChildFile("another-library");
    otherLibrary.createDirectory();
    other->setPresetDirectory(otherLibrary);
    button(panel, "settings.tab.6").onClick();
    check(dynamic_cast<Label *>(panel.findChildWithID("settings.folder.4"))->getText() ==
              selectedLibrary.getFullPathName(),
          "library row reports the actual instance folder when another instance changes the shared "
          "default");
    check(editor.setBrowserFolder(UserSettings::Folder::Library, selectedLibrary),
          "restore selected library");
    check(!editor.setBrowserFolder(UserSettings::Folder::Count, root),
          "invalid folder kind is rejected");
    write("browser-folders");
    button(panel, "settings.tab.2").onClick();
    auto &style = *dynamic_cast<ComboBox *>(panel.findChildWithID("settings.theme.background"));
    auto &opacity = *dynamic_cast<Slider *>(panel.findChildWithID("settings.background.panels"));
    style.setSelectedId(1, sendNotificationSync);
    auto flat = ui->createComponentSnapshot(ui->getLocalBounds());
    style.setSelectedId(3, sendNotificationSync);
    opacity.setValue(40, sendNotificationSync);
    auto grid = ui->createComponentSnapshot(ui->getLocalBounds());
    check(grid.getPixelAt(24, 200) != grid.getPixelAt(25, 200),
          "grid remains visible inside panels");
    check(flat.getPixelAt(24, 200) != grid.getPixelAt(24, 200),
          "skin background selection changes rendered panel pixels");
    write("grid-background");
    Image source(Image::ARGB, 256, 256, true);
    {
        Graphics g(source);
        g.fillAll(Colours::magenta);
    }
    auto pngFile = root.getChildFile("background.png"),
         jpegFile = root.getChildFile("background.jpg");
    {
        FileOutputStream out(pngFile);
        PNGImageFormat png;
        check(png.writeImageToStream(source, out), "PNG fixture");
    }
    {
        FileOutputStream out(jpegFile);
        JPEGImageFormat jpeg;
        check(jpeg.writeImageToStream(source, out), "JPEG fixture");
    }
    check(editor.importBackground(jpegFile).wasOk() && editor.importBackground(pngFile).wasOk(),
          "actual PNG/JPEG background import");
    dynamic_cast<Slider *>(panel.findChildWithID("settings.background.mix"))
        ->setValue(100, sendNotificationSync);
    auto image = ui->createComponentSnapshot(ui->getLocalBounds());
    check(image.getPixelAt(8, 300) == Colours::magenta,
          "background image fills the canvas with correct image mix");
    check(image.getPixelAt(200, 257) != flat.getPixelAt(200, 257),
          "image shows through panel opacity");
    write("image-background");
    const auto before = p->appearanceDraft->edited;
    const auto invalid = root.getChildFile("invalid.png");
    invalid.replaceWithText("not an image");
    check(editor.importBackground(invalid).failed() && p->appearanceDraft->edited.same(before),
          "failed image import retains previous theme draft");
    String encoded;
    Image decoded;
    check(SkinBackground::import(pngFile, encoded, decoded).wasOk(), "PNG source encoded");
    ThemeDefinition invalidTheme = before;
    invalidTheme.backgroundImage = "broken base64";
    ThemeDefinition next;
    check(!ThemeSettings::decode(ThemeSettings::encode(invalidTheme), next),
          "invalid embedded image rejected");
    auto huge = root.getChildFile("oversized.png");
    MemoryBlock bytes;
    pngFile.loadFileAsData(bytes);
    auto *raw = static_cast<uint8_t *>(bytes.getData());
    raw[16] = 0;
    raw[17] = 0;
    raw[18] = 16;
    raw[19] = 1;
    huge.replaceWithData(bytes.getData(), bytes.getSize());
    check(editor.importBackground(huge).failed(),
          "oversized image dimensions rejected before decompression");
    button(panel, "settings.tab.1").onClick();
    dynamic_cast<TextEditor *>(panel.findChildWithID("settings.theme.name"))
        ->setText("Portable image background");
    button(panel, "settings.theme.save").onClick();
    auto saved = ThemeSettings::theme(ThemeSettings::current());
    check(saved.backgroundImage == encoded && saved.backgroundStyle == 4 &&
              saved.panelOpacity == .4 && saved.backgroundMix == 1,
          "theme save retains embedded image, grid and panel/image opacity");
    pngFile.deleteFile();
    jpegFile.deleteFile();
    check(SkinBackground::decode(saved.backgroundImage).isValid(),
          "theme image is portable after original files are removed");
    if (report != File{}) {
        Image artwork(Image::RGB, 1240, 860, true);
        {
            Graphics g(artwork);
            g.setGradientFill(
                ColourGradient(Colour(0xff082334), 0, 0, Colour(0xff292044), 1240, 860, false));
            g.fillAll();
            g.setColour(Colour(0xff65c7dc).withAlpha(.17f));
            for (int x = 0; x < 1240; x += 36)
                g.drawVerticalLine(x, 0, 860);
            for (int y = 0; y < 860; y += 36)
                g.drawHorizontalLine(y, 0, 1240);
            g.setColour(Colour(0xffbc79f5).withAlpha(.3f));
            for (int i = 0; i < 9; ++i)
                g.drawEllipse(40.f + i * 35, 180.f + i * 21, 720.f, 720.f, 2.f);
        }
        auto file = report.getChildFile("example-background.png");
        {
            FileOutputStream out(file);
            PNGImageFormat png;
            check(png.writeImageToStream(artwork, out), "example background");
        }
        check(editor.importBackground(file).wasOk(), "example skin import");
        button(panel, "settings.tab.2").onClick();
        style.setSelectedId(4, sendNotificationSync);
        dynamic_cast<Slider *>(panel.findChildWithID("settings.background.mix"))
            ->setValue(75, sendNotificationSync);
        opacity.setValue(65, sendNotificationSync);
        write("custom-background");
    }
    ui.reset();
    ui.reset(p->createEditor());
    auto reload = ThemeSettings::theme(ThemeSettings::reload());
    check(reload.backgroundImage == saved.backgroundImage,
          "embedded image survives preference/folder reload");
    auto &look = dynamic_cast<LogisthLook &>(ui->getLookAndFeel());
    for (int theme = 0; theme < int(ThemeSettings::defaults().size()); ++theme) {
        look.setTheme(theme);
        auto menu = look.contextMenu();
        menu.addItem(1, "Patch / reorder");
        check(menu.getNumItems() == 1 &&
                  look.findColour(PopupMenu::backgroundColourId) == look.screen &&
                  look.findColour(PopupMenu::textColourId) == look.screenText,
              "context menus use each selected theme");
        Image background(Image::ARGB, 80, 60, true);
        {
            Graphics g(background);
            look.drawPopupMenuBackground(g, 80, 60);
        }
        check(background.getPixelAt(40, 30) == look.screen,
              "popup background raster uses theme screen colour");
    }
    Image noise(Image::RGB, 256, 256, true);
    {
        Image::BitmapData pixels(noise, Image::BitmapData::writeOnly);
        uint32_t seed = 0x195ab3;
        for (int y = 0; y < 256; ++y)
            for (int x = 0; x < 256; ++x) {
                seed = 1664525u * seed + 1013904223u;
                pixels.setPixelColour(x, y, Colour(0xff000000u | (seed >> 8)));
            }
    }
    const auto large = root.getChildFile("large.png");
    {
        FileOutputStream out(large);
        PNGImageFormat png;
        check(png.writeImageToStream(noise, out), "large PNG fixture");
    }
    auto largeTheme = saved;
    largeTheme.name = "Large portable image";
    check(SkinBackground::import(large, largeTheme.backgroundImage, decoded).wasOk(),
          "large image import");
    const auto largeJson = root.getChildFile("large-theme.json");
    check(ThemeSettings::saveTo(largeTheme, largeJson).wasOk() && largeJson.getSize() > 65536 &&
              ThemeSettings::theme(ThemeSettings::reload()).backgroundImage ==
                  largeTheme.backgroundImage,
          "theme scanner accepts portable images exceeding the old 64 KiB JSON limit");
    root.deleteRecursively();
    std::cout << "PASS 0.9.8.1 appearance: 96 text scale/DPI/quality cases, coverage, exact "
                 "vectors, five folders, portable PNG/JPEG/grid backgrounds and ten-theme popups\n";
}
} // namespace appearance0981_test
