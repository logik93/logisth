#pragma once
#include "FactoryPresets.h"
#include "ProcessorPhaseTests.h"
#include <set>
namespace factorybank_test {
using namespace juce;
inline void require(bool ok, const char *message) {
    if (!ok)
        throw std::runtime_error(message);
}
inline AudioBuffer<float> render(LogisthProcessor &source, int group, int seconds = 3) {
    // Independent initial random phases make every profile reproducible, including
    // when the fast test auditions only one sound from each category.
    auto instance = std::make_unique<LogisthProcessor>();
    MemoryBlock state;
    source.getStateInformation(state);
    instance->setStateInformation(state.getData(), int(state.getSize()));
    auto &p = *instance;
    constexpr int rate = 48000, blockSize = 512;
    p.prepareToPlay(rate, blockSize);
    AudioBuffer<float> result(2, seconds * rate), block(2, blockSize);
    std::vector<int> notes = group == 0 ? std::vector<int>{36} : std::vector<int>{48, 55, 60, 64};
    int off = seconds * rate / 2;
    for (int at = 0; at < result.getNumSamples(); at += blockSize) {
        int n = jmin(blockSize, result.getNumSamples() - at);
        block.setSize(2, n, false, false, true);
        MidiBuffer midi;
        for (int key : notes) {
            if (at == 0)
                midi.addEvent(MidiMessage::noteOn(1, key, 1.f), 0);
            if (at <= off && off < at + n)
                midi.addEvent(MidiMessage::noteOff(1, key), off - at);
        }
        p.processBlock(block, midi);
        for (int ch = 0; ch < 2; ++ch)
            result.copyFrom(ch, at, block, ch, 0, n);
    }
    return result;
}
inline void writeWav(const File &file, const AudioBuffer<float> &audio) {
    file.getParentDirectory().createDirectory();
    auto stream = file.createOutputStream();
    require(bool(stream), "factory WAV stream");
    stream->setPosition(0);
    stream->truncate();
    WavAudioFormat format;
    std::unique_ptr<AudioFormatWriter> writer(
        format.createWriterFor(stream.get(), 48000, 2, 24, {}, 0));
    require(bool(writer), "factory WAV writer");
    stream.release();
    require(writer->writeFromAudioSampleBuffer(audio, 0, audio.getNumSamples()),
            "factory WAV write");
}
inline void run(bool exportAll = false, const File &folder = {}) {
    auto owner = std::make_unique<LogisthProcessor>();
    auto &p = *owner;
    std::set<String> names;
    String csv = "index,name,profile,main,peak_dbfs,rms_dbfs\n";
    AudioBuffer<float> demo(2, 8 * 72000);
    demo.clear();
    for (int sound = 0; sound < int(logisth::designedSounds.size()); ++sound) {
        int index = sound + 10, group = sound / 6;
        p.loadPreset(index);
        require(names.insert(p.currentPresetName()).second, "new factory names are unique");
        require(p.presetComment().isNotEmpty(),
                "new factory patches have useful editable comments");
        require(p.macroName(0) == "Brightness" && p.macroName(1) == "Harmonics",
                "new patches have performance macros");
        for (const auto &spec : logisth::parameterSpecs) {
            float value = p.state.getRawParameterValue(spec.id)->load();
            require(std::isfinite(value) && value >= spec.low && value <= spec.high,
                    "factory parameters stay in range");
        }
        // A separately restored processor proves portable native patches preserve all controls.
        TemporaryFile patch(".logisth");
        require(p.savePresetFile(patch.getFile()).wasOk(), "authored factory preset saves");
        auto restored = std::make_unique<LogisthProcessor>();
        require(restored->loadPresetFile(patch.getFile()).wasOk(), "authored factory preset loads");
        require(restored->currentPresetName() == p.currentPresetName() &&
                    restored->presetComment() == p.presetComment(),
                "factory preset name and description survive file round-trip");
        for (const auto &spec : logisth::parameterSpecs)
            require(std::abs(restored->state.getRawParameterValue(spec.id)->load() -
                             p.state.getRawParameterValue(spec.id)->load()) < 1e-5f,
                    "all factory controls survive file round-trip");
        for (int macro = 0; macro < 4; ++macro)
            require(restored->macroName(macro) == p.macroName(macro),
                    "factory performance macro labels survive file round-trip");
        restored.reset();
        if (exportAll || sound % 6 == 0) {
            for (int profile = 0; profile < (exportAll ? 2 : 1); ++profile) {
                p.loadPreset(index);
                for (int macro = 1; macro <= 4; ++macro)
                    gain_test::set(p, "macro" + String(macro), profile ? 1.f : 0.f);
                auto audio = render(p, group);
                double peak = 0, energy = 0;
                for (int ch = 0; ch < 2; ++ch)
                    for (int i = 0; i < audio.getNumSamples(); ++i) {
                        float x = audio.getSample(ch, i);
                        require(std::isfinite(x) && std::abs(x) < 64,
                                "factory audio is finite and bounded");
                        peak = std::max(peak, double(std::abs(x)));
                        energy += x * x;
                    }
                require(peak > 1e-4, "each authored sound produces audio");
                double rms = std::sqrt(energy / (2 * audio.getNumSamples()));
                csv += String(index) + "," + p.currentPresetName() + "," +
                       (profile ? "macros_max" : "default") + "," +
                       String(p.state.getRawParameterValue("master")->load(), 9) + "," +
                       String(20 * std::log10(peak), 6) + "," +
                       String(20 * std::log10(jmax(1e-12, rms)), 6) + "\n";
                if (exportAll && profile == 0 && sound % 6 == 0) {
                    for (int ch = 0; ch < 2; ++ch)
                        demo.copyFrom(ch, group * 72000, audio, ch, 0, 72000);
                }
                require(peak < .708, "default and maximum macros retain at least 3 dB headroom");
            }
        }
        if (exportAll) {
            p.loadPreset(index);
            String name = p.currentPresetName();
            auto path = folder.getChildFile("Presets")
                            .getChildFile(name.upToFirstOccurrenceOf("/", false, false).trim())
                            .getChildFile(name.fromFirstOccurrenceOf("/", false, false).trim() +
                                          ".logisth");
            require(p.savePresetFile(path).wasOk(), "factory preset exports in the native format");
        }
    }
    if (exportAll) {
        folder.createDirectory();
        require(folder.getChildFile("factory-measurements.csv").replaceWithText(csv),
                "factory measurement report");
        writeWav(folder.getChildFile("factory-demo.wav"), demo);
        folder.getChildFile("README.md")
            .replaceWithText(
                "# logisth factory collection\n\n48 authored patches in eight categories. Embedded "
                "factory material only; no external samples.\n"
                "Every patch includes a description and four neutral-at-zero performance macros.\n"
                "Measurements: 48 kHz, full velocity, three-second chord/mono audition, default "
                "and maximum macros.\n"
                "The twelve-second demo plays the first patch from each category.\n");
    }
    std::cout << "PASS factory collection: 48 unique authored patches, descriptions, macros, valid "
                 "parameters and audition checks\n";
}
inline void search() {
    auto owner = std::make_unique<LogisthProcessor>();
    auto &p = *owner;
    std::unique_ptr<AudioProcessorEditor> ui(p.createEditor());
    auto *console = ui->getChildComponent(0);
    auto *button = dynamic_cast<Button *>(console->findChildWithID("preset.search"));
    auto *panel = console->findChildWithID("preset.search.panel");
    require(button && panel, "preset search button and panel exist");
    auto *query = dynamic_cast<TextEditor *>(panel->findChildWithID("preset.search.query"));
    auto *results = dynamic_cast<ListBox *>(panel->findChildWithID("preset.search.results"));
    button->onClick();
    require(panel->isVisible() &&
                results->getListBoxModel()->getNumRows() >= logisth::factoryPresetCount,
            "magnifier opens searchable factory list");
    query->setText("bass", false);
    query->onTextChange();
    require(results->getListBoxModel()->getNumRows() >= 6 &&
                results->getListBoxModel()->getNumRows() < logisth::factoryPresetCount,
            "search filters names and categories");
    query->setText("SAMPLE formant glass", false);
    query->onTextChange();
    require(results->getListBoxModel()->getNumRows() == 1,
            "multiple query words narrow the factory list");
    query->onReturnKey();
    require(!panel->isVisible() && p.currentPresetName() == "Sample / Formant Glass",
            "Enter loads the matched patch and closes search");
    button->onClick();
    query->setText("no-such-patch-894", false);
    query->onTextChange();
    require(results->getListBoxModel()->getNumRows() == 0,
            "unmatched searches show no stale items");
    query->onEscapeKey();
    require(!panel->isVisible(), "Escape closes preset search");
    const int oldTheme = logisth::ThemeSettings::current();
    auto *editor = dynamic_cast<LogisthEditor *>(ui.get());
    button->onClick();
    query->setText("WWWiiiWWWiii", false);
    editor->setTheme(6);
    const auto target = dynamic_cast<LogisthLook &>(ui->getLookAndFeel()).screenText;
    auto image = query->createComponentSnapshot(query->getLocalBounds());
    int coloured = 0;
    for (int y = 3; y < image.getHeight() - 3; ++y)
        for (int x = 4; x < image.getWidth() - 4; ++x) {
            auto pixel = image.getPixelAt(x, y);
            coloured += std::abs(pixel.getFloatRed() - target.getFloatRed()) +
                            std::abs(pixel.getFloatGreen() - target.getFloatGreen()) +
                            std::abs(pixel.getFloatBlue() - target.getFloatBlue()) <
                        .15f;
        }
    require(coloured > 15, "theme changes recolour existing search text");
    auto *settings = dynamic_cast<Button *>(console->findChildWithID("ui.settings"));
    settings->onClick();
    require(!panel->isVisible(), "opening Settings dismisses the preset search overlay");
    editor->setTheme(oldTheme);
    std::cout << "PASS preset search: magnifier, case-insensitive categories, multiple words, "
                 "Enter and Escape\n";
}
} // namespace factorybank_test
