#pragma once
#include "Features0962Tests.h"
#include "ThemeContrast.h"
namespace features097_test {
inline void run(const juce::File &report = {}) {
    using namespace juce;
    using namespace logisth;
    auto check = [](bool ok, const char *why) {
        if (!ok)
            throw std::runtime_error(why);
    };
    auto owner = std::make_unique<LogisthProcessor>();
    auto &p = *owner;
    check(p.effectivePolyphony() == 16, "Init retains sixteen voices by default");
    const auto contract = JSON::parse(
        File(String(LOGISTH_SOURCE_DIR)).getChildFile("tests/fixtures/parameters-0.9.6.2.json"));
    check(contract.isArray() && contract.size() == 1070 && parameterCount >= 1071,
          "complete previous host contract");
    for (int i = 0; i < contract.size(); ++i) {
        const auto row = contract[i];
        const auto &s = parameterSpecs[size_t(i)];
        const double expectedDefault = i == int(Param::bendrange) ? 12. : double(row["initial"]);
        check(row["id"].toString() == s.id && int(row["index"]) == i &&
                  row["choices"].toString() == s.choices &&
                  std::abs(double(row["low"]) - s.low) < 1e-6 &&
                  std::abs(double(row["high"]) - s.high) < 1e-6 &&
                  std::abs(expectedDefault - s.initial) < 1e-6 &&
                  std::abs(double(row["skew"]) - s.skew) < 1e-6 &&
                  bool(row["integer"]) == s.integer &&
                  p.getParameters()[i] == p.state.getParameter(s.id),
              "all 1070 prior parameter definitions and indices remain intact");
    }
    std::unique_ptr<AudioProcessorEditor> ui(p.createEditor());
    auto *editor = dynamic_cast<LogisthEditor *>(ui.get());
    auto *console = ui->getChildComponent(0);
    check(console->findChildWithID("sample.waveform")->isVisible(),
          "new editors open the sample view");
    Slider *poly = nullptr;
    for (auto *child : console->findChildWithID("polyphony")->getChildren())
        if (auto *slider = dynamic_cast<Slider *>(child))
            poly = slider;
    check(poly && poly->getMaximum() == 32 && poly->getValue() == 16,
          "one polyphony control exposes 1-32 with default 16");
    poly->setValue(32, sendNotificationSync);
    check(p.effectivePolyphony() == 32 && p.effectiveValue(Param::polyphony) == 16 &&
              p.effectiveValue(Param::polyphonyextra) == 16,
          "combined polyphony preserves the old normalized host range");
    poly->setValue(7, sendNotificationSync);
    check(p.effectivePolyphony() == 7 && p.effectiveValue(Param::polyphonyextra) == 0,
          "lower polyphony clears the extension");
    gain_test::set(p, "polyphony", 16);
    gain_test::set(p, "polyphonyextra", 16);
    check(poly->getValue() == 32, "one host update refreshes the combined polyphony display");
    gain_test::set(p, "polyphonyextra", 0);
    check(poly->getValue() == 16, "one host update clears the visible voice extension");
    Slider *detune = nullptr;
    ComboBox *destination = nullptr;
    for (auto *child : console->findChildWithID("o1detune")->getChildren())
        if (auto *slider = dynamic_cast<Slider *>(child))
            detune = slider;
    for (auto *child : console->findChildWithID("env1dest1")->getChildren())
        if (auto *menu = dynamic_cast<ComboBox *>(child))
            destination = menu;
    gain_test::set(p, "o1detune", 20);
    gain_test::set(p, "o1detuneextra", 80);
    gain_test::set(p, "o1detunewide", 160);
    check(detune && detune->getValue() == 260,
          "host detune updates show the current three-parameter sum");
    gain_test::set(p, "o1detune", 10);
    check(detune->getValue() == 250,
          "a single base update does not lag behind its numeric display");
    const auto extra = choiceExtension(Param::env1dest1);
    gain_test::set(p, parameterSpecs[size_t(extra)].id, 1);
    check(destination &&
              destination->getSelectedId() == int(p.effectiveValue(Param::env1dest1)) + 1,
          "host Part C destination changes immediately refresh the expanded menu");
    gain_test::set(p, parameterSpecs[size_t(extra)].id, 0);
    gain_test::set(p, "env1dest1", 14);
    check(destination->getSelectedId() == 15,
          "host base destination changes refresh on the first update");
    auto *clip = console->findChildWithID("rack.11");
    const auto start = Point<float>{float(clip->getWidth() - 6), 10};
    auto mouse = [&](Point<float> at) {
        return MouseEvent(Desktop::getInstance().getMainMouseSource(), at,
                          ModifierKeys::leftButtonModifier, 1, 0, 0, 0, 0, clip, clip,
                          Time::getCurrentTime(), start, Time::getCurrentTime(), 1, true);
    };
    clip->mouseDown(mouse(start));
    clip->mouseDrag(mouse({start.x, float(342 - clip->getY())}));
    clip->mouseUp(mouse(start));
    check(p.effectOrder().stages[0] == EffectStage::Clipper && clip->getY() == 332,
          "actual pointer drag moves the DSP stage and its visible row");
    check(console->findChildWithID("sample.waveform")->isVisible(),
          "reordering does not unexpectedly switch editing pages");
    check(clip->keyPressed(KeyPress(KeyPress::downKey, ModifierKeys::altModifier, 0)) &&
              p.effectOrder().stages[1] == EffectStage::Clipper,
          "keyboard reorder matches drag behavior");
    check(console->findChildWithID("rack.8")->getBottom() < clip->getY() &&
              console->findChildWithID("rack.0")->getBottom() < clip->getY() &&
              console->findChildWithID("rack.12")->getY() == 492,
          "generators stay separate and final limiter remains fixed");
    const auto savedOrder = p.effectOrder();
    TemporaryFile preset(".logisth");
    check(p.savePresetFile(preset.getFile()).wasOk(), "custom order preset saves");
    auto restored = std::make_unique<LogisthProcessor>();
    check(restored->loadPresetFile(preset.getFile()).wasOk() &&
              restored->effectOrder() == savedOrder,
          "custom order restores from a portable preset");
    MemoryBlock state;
    p.getStateInformation(state);
    auto xml = AudioProcessor::getXmlFromBinary(state.getData(), int(state.getSize()));
    auto tree = ValueTree::fromXml(*xml);
    tree.setProperty("effectOrder", 0, nullptr);
    MemoryBlock bad;
    AudioProcessor::copyXmlToBinary(*tree.createXml(), bad);
    check(!p.applyState(bad.getData(), int(bad.getSize())) && p.effectOrder() == savedOrder,
          "invalid duplicate stage state is rejected transactionally");
    for (const auto *malformed : {"12junk", "-2", "16777216", "", "1.0", "999999999999999999"}) {
        tree.setProperty("effectOrder", malformed, nullptr);
        AudioProcessor::copyXmlToBinary(*tree.createXml(), bad);
        check(!p.applyState(bad.getData(), int(bad.getSize())) && p.effectOrder() == savedOrder,
              "rack metadata rejects invalid decimal attributes without changing the patch");
    }
    tree.removeProperty("effectOrder", nullptr);
    AudioProcessor::copyXmlToBinary(*tree.createXml(), bad);
    check(p.applyState(bad.getData(), int(bad.getSize())) && p.effectOrder() == EffectOrder{},
          "older states without rack metadata retain the original order");
    gain_test::cleanSine(p);
    gain_test::set(p, "polyphony", 16);
    gain_test::set(p, "polyphonyextra", 16);
    gain_test::set(p, "compon", 1);
    gain_test::set(p, "clipon", 1);
    gain_test::set(p, "limiton", 1);
    gain_test::set(p, "limitceiling", -6);
    p.prepareToPlay(48000, 128);
    AudioBuffer<float> block(2, 128);
    MidiBuffer midi;
    for (int note = 0; note < 32; ++note)
        midi.addEvent(MidiMessage::noteOn(1, 40 + note, .1f), note);
    p.processBlock(block, midi);
    midi.clear();
    check(p.voices.load() == 32, "processor actually renders thirty-two held notes");
    p.moveEffect(7, 0);
    audio_allocation_probe::begin();
    for (int i = 0; i < 12; ++i) {
        p.processBlock(block, midi);
        midi.clear();
    }
    const auto allocations = audio_allocation_probe::end();
    check(allocations == 0 && block.getMagnitude(0, 128) <= amplitude(-6) + 1e-6,
          "32-voice live reorder allocates no C++ audio memory and retains final ceiling");
    p.loadPreset(0);
    gain_test::set(p, "polyphony", 16);
    gain_test::set(p, "polyphonyextra", 16);
    gain_test::set(p, "o1engine", 2);
    gain_test::set(p, "o1stretchmode", 7);
    gain_test::set(p, "o1ssblock", 20);
    p.prepareToPlay(8000, 128);
    check(p.spectralReady(0), "processor prepares the spectral bank for 32 voices");
    for (int note = 0; note < 32; ++note)
        midi.addEvent(MidiMessage::noteOn(1, 40 + note, .02f), 0);
    p.processBlock(block, midi);
    midi.clear();
    check(p.voices.load() == 32, "sampler spectral playback also supports all 32 notes");
    audio_allocation_probe::begin();
    p.processBlock(block, midi);
    midi.clear();
    const auto spectralAllocations = audio_allocation_probe::end();
    check(spectralAllocations == 0,
          "prepared spectral playback remains allocation-free at 32 voices");
    LogisthLook look;
    String csv = "theme,style,screen,small,pointer_top,pointer_bottom,active,track,raster\n";
    Image contact(Image::RGB, 1000, 360, true);
    Graphics sheet(contact);
    sheet.fillAll(Colour(0xff151d29));
    const auto themes = ThemeSettings::defaults();
    int cases = 0;
    for (size_t theme = 0; theme < themes.size(); ++theme) {
        auto definition = themes[theme];
        const int originX = int(theme % 5) * 200, originY = int(theme / 5) * 180;
        sheet.setColour(definition.colours[1]);
        sheet.fillRect(originX, originY, 200, 180);
        sheet.setFont(Font(FontOptions(10)));
        sheet.setColour(definition.colours[3]);
        sheet.drawFittedText(definition.name, {originX + 5, originY + 3, 190, 24},
                             Justification::centred, 2);
        for (int style = 0; style < 3; ++style)
            for (bool screen : {false, true})
                for (bool small : {false, true}) {
                    definition.knobStyle = style;
                    look.setPalette(definition);
                    check(colourContrast(look.screenAccent, look.screen) >= 2.99,
                          "screen plot accents retain contrast across every palette");
                    const auto colours = look.knobColours(screen);
                    const double a = colourContrast(colours.pointer, colours.top),
                                 b = colourContrast(colours.pointer, colours.bottom);
                    check(a >= 4.49 && b >= 4.49 &&
                              colourContrast(colours.active, colours.surface) >= 2.99 &&
                              colourContrast(colours.track, colours.surface) >= 2.99,
                          "all ten themes and styles meet enabled knob contrast targets");
                    Slider slider;
                    slider.getProperties().set("dark", screen);
                    slider.getProperties().set("trim", small);
                    Image image(Image::RGB, 60, 60, true);
                    {
                        // Native Windows images finish their Direct2D frame when
                        // the Graphics context closes. Read pixels only afterward.
                        Graphics graphics(image);
                        graphics.fillAll(colours.surface);
                        look.drawRotarySlider(graphics, 0, 0, 60, 60, .5f, -2.4f, 2.4f, slider);
                    }
                    double raster = 0;
                    for (int x = 29; x <= 30; ++x)
                        for (int y = 15; y <= 17; ++y)
                            raster = std::max(raster, colourContrast(image.getPixelAt(x, y),
                                                                     colours.top.interpolatedWith(
                                                                         colours.bottom, .2f)));
                    if (raster <= 3.0)
                        std::cerr << "Needle raster: " << definition.name << " style=" << style
                                  << " screen=" << screen << " small=" << small
                                  << " contrast=" << raster << " pixels="
                                  << image.getPixelAt(29, 16).toDisplayString(true) << "/"
                                  << image.getPixelAt(30, 16).toDisplayString(true) << '\n';
                    check(raster > 3.0,
                          "actual rasterized needle remains visible in bright and screen contexts");
                    if (!small)
                        sheet.drawImageAt(image, originX + 10 + style * 62,
                                          originY + 30 + (screen ? 70 : 0));
                    csv += definition.name + "," + String(style) + "," + String(int(screen)) + "," +
                           String(int(small)) + "," + String(a, 4) + "," + String(b, 4) + "," +
                           String(colourContrast(colours.active, colours.surface), 4) + "," +
                           String(colourContrast(colours.track, colours.surface), 4) + "," +
                           String(raster, 4) + "\n";
                    ++cases;
                }
    }
    check(cases == 120, "all theme/style/context/trim combinations checked");
    const auto layout = editor->validateLayout();
    if (layout.isNotEmpty())
        std::cerr << layout << '\n';
    check(layout.isEmpty(), "every control remains reachable without overlaps after rack changes");
    if (report != File{}) {
        check(report.createDirectory().wasOk(), "theme/rack report directory");
        check(report.getChildFile("theme-contrast.csv").replaceWithText(csv),
              "contrast CSV export");
        auto write = [&](const char *name, const Image &image) {
            FileOutputStream output(report.getChildFile(name));
            output.setPosition(0);
            output.truncate();
            PNGImageFormat png;
            check(png.writeImageToStream(image, output), "theme/rack PNG export");
        };
        write("knob-contrast.png", contact);
        p.moveEffect(7, 0);
        editor->snapshotPage(35);
        editor->setTheme(0);
        write("light-compressor.png", ui->createComponentSnapshot(ui->getLocalBounds()));
        editor->setTheme(1);
        editor->snapshotPage(36);
        write("rack-reordered.png", ui->createComponentSnapshot(ui->getLocalBounds()));
    }
    std::cout
        << "PASS 0.9.7: 1070-field compatibility, 32 voices, default Sample, pointer/keyboard "
           "reorder, preset/legacy order, allocation/ceiling, 120 theme renders and layout\n";
}
} // namespace features097_test
