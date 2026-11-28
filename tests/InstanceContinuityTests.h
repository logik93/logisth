#pragma once
#include "FactoryBankTests.h"
#include "Features0983Tests.h"
#include <sstream>

namespace instance_continuity_test {
using namespace juce;
using gain_test::set;

inline void makeRecording(const File &file) {
    AudioBuffer<float> pcm(2, 16384);
    for (int i = 0; i < pcm.getNumSamples(); ++i) {
        pcm.setSample(0, i, .4f * std::sin(float(i) * .047f));
        pcm.setSample(1, i, .3f * std::cos(float(i) * .093f));
    }
    factorybank_test::writeWav(file, pcm);
}
inline double peak(const AudioBuffer<float> &audio) {
    return audio.getMagnitude(0, audio.getNumSamples());
}
inline double difference(const AudioBuffer<float> &a, const AudioBuffer<float> &b) {
    double result = 0;
    for (int ch = 0; ch < 2; ++ch)
        for (int i = 0; i < a.getNumSamples(); ++i)
            result = std::max(result, double(std::abs(a.getSample(ch, i) - b.getSample(ch, i))));
    return result;
}
inline void configure(LogisthProcessor &p, const File &file, int mode, bool grainModel) {
    gain_test::require(p.loadSource(0, file).wasOk(), "import stereo continuity fixture");
    set(p, "o1engine", float(mode));
    set(p, "o1loop", 1);
    set(p, "o1gmodel", grainModel ? 1 : 0);
    set(p, "o1retrig", 1);
    set(p, "o1spray", 0);
    set(p, "o1position", .12f);
    set(p, "o1end", .88f);
    p.prepareToPlay(48000, 256);
}
inline void checkOutputOwnership() {
    auto p = std::make_unique<LogisthProcessor>();
    p->prepareToPlay(48000, 128);
    set(*p, "arpout", 1);
    set(*p, "chordon", 1);
    set(*p, "arpon", 1);
    AudioBuffer<float> audio(2, 128);
    MidiBuffer midi;
    p->processBlock(audio, midi);
    gain_test::require(midi.isEmpty(), "new MIDI-out instance leaves shared channels alone");
    set(*p, "chordon", 0);
    set(*p, "arpon", 0);
    midi.addEvent(MidiMessage::noteOn(3, 63, .75f), 5);
    midi.addEvent(MidiMessage::controllerEvent(3, 64, 127), 12);
    p->processBlock(audio, midi);
    gain_test::require(midi.getNumEvents() == 2, "normal MIDI-out note and pedal pass through");
    midi.clear();
    set(*p, "arpout", 0);
    p->processBlock(audio, midi);
    bool releasedKey = false, releasedPedal = false;
    for (auto meta : midi) {
        auto m = meta.getMessage();
        gain_test::require(m.getChannel() == 3 && meta.samplePosition == 0, "only owned channel");
        releasedKey |= m.isNoteOff() && m.getNoteNumber() == 63;
        releasedPedal |= m.isControllerOfType(64) && m.getControllerValue() == 0;
    }
    gain_test::require(midi.getNumEvents() == 2 && releasedKey && releasedPedal,
                       "disabling MIDI out releases outstanding key and pedal");
    midi.clear();
    set(*p, "arpout", 1);
    midi.addEvent(MidiMessage::allNotesOff(4), 11);
    p->processBlock(audio, midi);
    gain_test::require(midi.getNumEvents() == 1 && (*midi.begin()).getMessage().isAllNotesOff() &&
                           (*midi.begin()).samplePosition == 11,
                       "an explicit incoming panic remains routable at its original offset");
}
inline void checkImportOrdering(const File &file) {
    auto p = std::make_unique<LogisthProcessor>();
    p->loadSourceAsync(0, file, 0, true);
    p->loadPreset(0);
    // Jobs are serial per instance. Completion on another slot fences the older decode.
    p->loadSourceAsync(5, file);
    for (int tries = 0;
         tries < 1000 && p->sourceData(5)->name != file.getFileNameWithoutExtension().toStdString();
         ++tries)
        Thread::sleep(5);
    gain_test::require(p->sourceData(5)->name == file.getFileNameWithoutExtension().toStdString() &&
                           p->sourceData(0) == logisth::SourceData::factory() &&
                           p->state.getRawParameterValue("o1engine")->load() == 0,
                       "a previous drop cannot overwrite a newly recalled preset");
}
inline void checkTrimControls(const File &report) {
    auto p = std::make_unique<LogisthProcessor>();
    std::unique_ptr<LogisthEditor> editor(static_cast<LogisthEditor *>(p->createEditor()));
    auto *console = editor->getChildComponent(0);
    auto *slotMenu = dynamic_cast<ComboBox *>(console->findChildWithID("sample.slot"));
    auto *wave = console->findChildWithID("sample.waveform");
    auto *position = dynamic_cast<Label *>(console->findChildWithID("sample.position"));
    gain_test::require(slotMenu && wave && position, "Sample page present");
    auto field = [&](const char *name) -> Slider & {
        if (auto *control = console->findChildWithID(name))
            for (auto *c : control->getChildren())
                if (auto *slider = dynamic_cast<Slider *>(c);
                    slider && control->isVisible() && slider->isVisible())
                    return *slider;
        throw std::runtime_error("missing Sample trim field");
    };
    for (int slot = 0; slot < logisth::playerCount; ++slot) {
        const String prefix = "o" + String(slot + 1);
        slotMenu->setSelectedId(slot + 1, sendNotificationSync);
        for (int mode : {2, 3}) {
            features0983_test::selectChoice(*console, prefix + "engine", mode + 1);
            features0983_test::tick();
            auto &start = field("sample.start"), &end = field("sample.end");
            logisth::PlaybackPicture live;
            live.active = true;
            live.position = .4275f;
            live.revision = p->sourceData(slot)->revision;
            p->playback[size_t(slot)].write(live);
            features0983_test::tick();
            gain_test::require(position->isVisible() && position->getText() == "42.75%" &&
                                   position->getX() >= start.getParentComponent()->getRight() &&
                                   position->getRight() <= end.getParentComponent()->getX(),
                               "live playhead readout is between trim controls");
            ++live.revision;
            p->playback[size_t(slot)].write(live);
            features0983_test::tick();
            gain_test::require(position->getText() == "--", "discard playhead from replaced source");
            auto *a = p->state.getParameter(prefix + (mode == 2 ? "position" : "trimstart"));
            auto *b = p->state.getParameter(prefix + "end");
            gain_test::require(editor->getControlParameterIndex(start) == a->getParameterIndex() &&
                                   editor->getControlParameterIndex(end) ==
                                       b->getParameterIndex() &&
                                   a->isAutomatable() && b->isAutomatable(),
                               "host identifies each player's independent trim endpoints");
            start.setValue(start.getValueFromText("25.50%"), sendNotificationSync);
            end.setValue(end.getValueFromText("76.25%"), sendNotificationSync);
            gain_test::require(std::abs(a->getValue() - .255f) < 1e-6 &&
                                   std::abs(b->getValue() - .7625f) < 1e-6,
                               "editing Sample UI values reaches host parameters");
            a->setValueNotifyingHost(.375f);
            b->setValueNotifyingHost(.625f);
            gain_test::require(std::abs(start.getValue() - .375) < 1e-6 &&
                                   std::abs(end.getValue() - .625) < 1e-6,
                               "automated bounds update both percentage fields");
            struct Gestures : AudioProcessorParameter::Listener {
                int starts = 0, ends = 0;
                void parameterValueChanged(int, float) override {}
                void parameterGestureChanged(int, bool begin) override {
                    (begin ? starts : ends)++;
                }
            } startLog, endLog;
            a->addListener(&startLog);
            b->addListener(&endLog);
            auto drag = [&](float from, float to) {
                wave->mouseDown(
                    features0983_test::event(*wave, {from * wave->getWidth(), 87}, true));
                wave->mouseDrag(features0983_test::event(*wave, {to * wave->getWidth(), 87}, true));
                wave->mouseUp(features0983_test::event(*wave, {to * wave->getWidth(), 87}, false));
            };
            drag(.375f, .50f);
            const bool onlyStart = startLog.starts == 1 && startLog.ends == 1 &&
                                   endLog.starts == 0 && endLog.ends == 0 &&
                                   std::abs(b->getValue() - .625f) < 1e-6;
            drag(.625f, .51f);
            a->removeListener(&startLog);
            b->removeListener(&endLog);
            gain_test::require(
                onlyStart && startLog.starts == 1 && endLog.starts == 1 && endLog.ends == 1 &&
                    std::abs(a->getValue() - .5f) < 1e-6 && std::abs(b->getValue() - .51f) < 1e-6,
                "waveform handles record exactly one balanced gesture on the touched endpoint");
            if (slot == 0 && mode == 2 && report != File{}) {
                report.createDirectory();
                auto image = wave->createComponentSnapshot(wave->getLocalBounds());
                FileOutputStream stream(report.getChildFile("adjacent-trim-handles.png"));
                stream.setPosition(0);
                stream.truncate();
                gain_test::require(PNGImageFormat{}.writeImageToStream(image, stream),
                                   "trim marker detail");
            }
            if (slot == 0 && mode == 2) {
                gain_test::set(*p, prefix + "position", .2f);
                gain_test::set(*p, prefix + "end", .8f);
                gain_test::set(*p, prefix + "tailon", 1);
                auto dragLoop = [&](float y, float destination) {
                    gain_test::set(*p, prefix + "tailstart", .5f);
                    gain_test::set(*p, prefix + "tailend", .5f);
                    auto from = Point<float>{.5f * wave->getWidth(), y};
                    auto to = Point<float>{destination * wave->getWidth(), y};
                    wave->mouseDown(features0983_test::event(*wave, from, true));
                    wave->mouseDrag(features0983_test::event(*wave, to, true));
                    wave->mouseUp(features0983_test::event(*wave, to, false));
                };
                auto raw = [&](const char *id) { return p->state.getRawParameterValue(prefix + id)->load(); };
                dragLoop(145, .4f);
                gain_test::require(std::abs(raw("tailstart") - .4f) < .00001f && raw("tailend") == .5f,
                                   "upper loop grip moves only LOOP IN when both are aligned");
                dragLoop(169, .6f);
                gain_test::require(raw("tailstart") == .5f && std::abs(raw("tailend") - .6f) < .00001f &&
                                       raw("position") == .2f && raw("end") == .8f,
                                   "lower loop grip moves only LOOP OUT and keeps sample trim");
            }
        }
        features0983_test::selectChoice(*console, prefix + "engine", 1);
        features0983_test::tick();
        gain_test::require(!console->findChildWithID("sample.start")->isVisible() &&
                               !console->findChildWithID("sample.end")->isVisible() &&
                               !position->isVisible(),
                           "trim hidden for classic");
        const auto position = p->state.getParameter(prefix + "position")->getValue();
        wave->mouseDown(features0983_test::event(*wave, {12, 90}, true));
        wave->mouseUp(features0983_test::event(*wave, {12, 90}, false));
        gain_test::require(p->state.getParameter(prefix + "position")->getValue() == position,
                           "classic waveform ignores sample trim gestures");
    }
}
inline void run(const File &folder = {}) {
    TemporaryFile recording(".wav");
    makeRecording(recording.getFile());
    checkOutputOwnership();
    checkImportOrdering(recording.getFile());
    checkTrimControls(folder);
    std::ostringstream csv;
    csv << "engine,second_instance_error,reprepare_error,reference_peak,actual_peak,reset_peak\n";
    bool continuous = true;
    for (int model = 0; model < 3; ++model) {
        auto primary = std::make_unique<LogisthProcessor>();
        auto reference = std::make_unique<LogisthProcessor>();
        for (auto *p : {primary.get(), reference.get()})
            configure(*p, recording.getFile(), model == 0 ? 2 : 3, model == 2);
        processor_phase_test::render(*primary, 2048, true);
        processor_phase_test::render(*reference, 2048, true);
        double openError = 0;
        {
            auto other = std::make_unique<LogisthProcessor>();
            configure(*other, recording.getFile(), 3, true);
            std::unique_ptr<AudioProcessorEditor> editor(other->createEditor());
            set(*other, "o1position", .76f);
            processor_phase_test::render(*other, 768, true);
            other->clearSource(0);
            openError = difference(processor_phase_test::render(*primary, 768),
                                   processor_phase_test::render(*reference, 768));
        }
        // A host can renegotiate the whole graph when inserting an instrument. No new
        // note-on is due on the track that was already playing.
        primary->releaseResources();
        primary->prepareToPlay(48000, 1024);
        primary->prepareToPlay(48000, 1024);
        auto after = processor_phase_test::render(*primary, 2048);
        auto expected = processor_phase_test::render(*reference, 2048);
        const double error = difference(after, expected);
        primary->reset();
        const double resetPeak = peak(processor_phase_test::render(*primary, 512));
        const char *name = model == 0 ? "Sampler" : model == 1 ? "Cloud" : "Granulizer";
        csv << name << ',' << openError << ',' << error << ',' << peak(expected) << ','
            << peak(after) << ',' << resetPeak << '\n';
        continuous &= openError < 1e-6 && error < 1e-6 && peak(expected) > .005 && resetPeak == 0;
        gain_test::require(primary->sourceData(0)->name == reference->sourceData(0)->name,
                           "second instance retains separate source ownership");
        processor_phase_test::render(*primary, 256, true);
        primary->prepareToPlay(44100, 256);
        gain_test::require(peak(processor_phase_test::render(*primary, 512)) == 0 &&
                               peak(processor_phase_test::render(*primary, 2048, true)) > .005,
                           "sample-rate change resets DSP and new notes play the stored recording");
    }
    if (folder != File{}) {
        gain_test::require(folder.createDirectory().wasOk() &&
                               folder.getChildFile("continuity.csv").replaceWithText(csv.str()),
                           "write continuity measurements");
    }
    std::cout << csv.str();
    gain_test::require(continuous, "a same-rate graph rebuild interrupted held sample audio");
}
} // namespace instance_continuity_test
