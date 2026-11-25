// A small host for the shipped VST3 module. Inserting an instrument may suspend
// the graph mid-note and renegotiate block size without moving the song cursor.
#include <algorithm>
#include <array>
#include <cmath>
#include <cstring>
#include <filesystem>
#include <iostream>
#include <pluginterfaces/vst/ivstaudioprocessor.h>
#include <pluginterfaces/vst/ivstevents.h>
#include <pluginterfaces/vst/ivsteditcontroller.h>
#include <pluginterfaces/vst/ivsthostapplication.h>
#include <pluginterfaces/vst/ivstmidicontrollers.h>
#include <pluginterfaces/vst/ivstmessage.h>
#include <pluginterfaces/vst/ivstparameterchanges.h>
#include <pluginterfaces/vst/ivstprocesscontext.h>
#include <stdexcept>
#ifdef _WIN32
#include <windows.h>
#else
#include <dlfcn.h>
#endif
#ifdef __APPLE__
#include <CoreFoundation/CoreFoundation.h>
#endif
using namespace Steinberg;
using namespace Steinberg::Vst;
namespace {
void check(bool ok, const char *message) {
    if (!ok)
        throw std::runtime_error(message);
}

class Module {
#ifdef _WIN32
    HMODULE handle{};
#else
    void *handle{};
#endif
    bool (*leave)() = nullptr;

  public:
    template <typename T> T symbol(const char *name) {
#ifdef _WIN32
        auto address = GetProcAddress(handle, name);
#else
        auto address = dlsym(handle, name);
#endif
        check(address != nullptr, name);
        return reinterpret_cast<T>(address);
    }
    explicit Module(const char *path) {
#ifdef _WIN32
        handle = LoadLibraryW(std::filesystem::u8path(path).c_str());
        check(handle != nullptr && symbol<bool (*)()>("InitDll")(), "VST3 DLL init");
        leave = symbol<bool (*)()>("ExitDll");
#else
        handle = dlopen(path, RTLD_NOW | RTLD_LOCAL);
        if (!handle)
            throw std::runtime_error(dlerror());
#ifdef __APPLE__
        const auto bundle =
            std::filesystem::path(path).parent_path().parent_path().parent_path().string();
        auto url = CFURLCreateFromFileSystemRepresentation(
            nullptr, reinterpret_cast<const UInt8 *>(bundle.data()), CFIndex(bundle.size()), true);
        auto ref = CFBundleCreate(nullptr, url);
        CFRelease(url);
        check(ref != nullptr && symbol<bool (*)(CFBundleRef)>("bundleEntry")(ref), "bundle init");
        CFRelease(ref);
        leave = symbol<bool (*)()>("bundleExit");
#else
        check(symbol<bool (*)(void *)>("ModuleEntry")(handle), "VST3 module init");
        leave = symbol<bool (*)()>("ModuleExit");
#endif
#endif
    }
    ~Module() {
        leave();
#ifdef _WIN32
        FreeLibrary(handle);
#else
        dlclose(handle);
#endif
    }
};

// ProcessData borrows these stack objects only for the duration of process().
template <class Interface> struct Borrowed : Interface {
    tresult PLUGIN_API queryInterface(const TUID, void **out) override {
        *out = nullptr;
        return kNoInterface;
    }
    uint32 PLUGIN_API addRef() override {
        return 1;
    }
    uint32 PLUGIN_API release() override {
        return 1;
    }
};
struct FlStudioHost : Borrowed<IHostApplication> {
    tresult PLUGIN_API queryInterface(const TUID iid, void **out) override {
        if (std::memcmp(iid, IHostApplication_iid, sizeof(TUID)) == 0 ||
            std::memcmp(iid, FUnknown_iid, sizeof(TUID)) == 0) {
            *out = static_cast<IHostApplication *>(this);
            return kResultOk;
        }
        return Borrowed<IHostApplication>::queryInterface(iid, out);
    }
    tresult PLUGIN_API getName(String128 name) override {
        constexpr TChar title[] = u"FL Studio";
        std::copy_n(title, std::size(title), name);
        return kResultOk;
    }
    tresult PLUGIN_API createInstance(TUID, TUID, void **out) override {
        *out = nullptr;
        return kNoInterface;
    }
};
struct Note : Borrowed<IEventList> {
    bool releaseKey = false;
    int32 PLUGIN_API getEventCount() override {
        return 1;
    }
    tresult PLUGIN_API getEvent(int32 i, Event &event) override {
        if (i != 0)
            return kInvalidArgument;
        event = {};
        event.type = Event::kNoteOnEvent;
        event.noteOn.pitch = 60;
        event.noteOn.velocity = .8f;
        event.noteOn.noteId = -1;
        if (releaseKey) {
            event.type = Event::kNoteOffEvent;
            event.noteOff = {};
            event.noteOff.pitch = 60;
            event.noteOff.noteId = -1;
            event.sampleOffset = 37;
        }
        return kResultOk;
    }
    tresult PLUGIN_API addEvent(Event &) override {
        return kNotImplemented;
    }
};
struct Point : Borrowed<IParamValueQueue> {
    ParamID id = 0;
    double value = 0;
    Point(const char *key, double v) : value(v) {
        // Published VST3 parameter IDs use the project's JUCE string hash.
        while (*key)
            id = 31 * id + uint32(*key++);
        id &= 0x7fffffff;
    }
    Point(ParamID parameter, double v) : id(parameter), value(v) {}
    ParamID PLUGIN_API getParameterId() override {
        return id;
    }
    int32 PLUGIN_API getPointCount() override {
        return 1;
    }
    tresult PLUGIN_API getPoint(int32 i, int32 &offset, double &v) override {
        if (i != 0)
            return kInvalidArgument;
        offset = 0;
        v = value;
        return kResultOk;
    }
    tresult PLUGIN_API addPoint(int32, double, int32 &) override {
        return kNotImplemented;
    }
};
struct SourceMode : Borrowed<IParameterChanges> {
    std::array<Point, 5> points;
    explicit SourceMode(int model)
        : points{Point("o1engine", model < 0 ? 0.
                                   : model   ? 1.
                                             : 2. / 3.),
                 Point("o1loop", 1.), Point("o1gmodel", model == 2 ? 1. : 0.),
                 Point("o1retrig", 1.), Point("o1spray", 0.)} {}
    int32 PLUGIN_API getParameterCount() override {
        return int32(points.size());
    }
    IParamValueQueue *PLUGIN_API getParameterData(int32 i) override {
        return &points.at(size_t(i));
    }
    IParamValueQueue *PLUGIN_API addParameterData(const ParamID &, int32 &) override {
        return nullptr;
    }
};
using Audio = std::array<std::array<float, 256>, 2>;
enum class ClockReport { normal, missing, rebased, rounded };
struct Player {
    IComponent *component = nullptr;
    IAudioProcessor *audio = nullptr;
    int64 cursor = 0;
    int32 processingMode = kRealtime;
    Player(IPluginFactory &factory, const TUID &cid, FUnknown *host = nullptr) {
        check(factory.createInstance(cid, IComponent_iid, reinterpret_cast<void **>(&component)) ==
                      kResultOk &&
                  component->initialize(host) == kResultOk,
              "create VST3 instrument");
        check(component->queryInterface(IAudioProcessor_iid, reinterpret_cast<void **>(&audio)) ==
                  kResultOk,
              "VST3 processor interface");
        check(component->activateBus(kAudio, kOutput, 0, true) == kResultOk &&
                  component->activateBus(kEvent, kInput, 0, true) == kResultOk,
              "VST3 audio and event buses");
        start(256);
    }
    void start(int block, int32 processMode = kRealtime) {
        processingMode = processMode;
        ProcessSetup setup{processMode, kSample32, block, 48000.};
        check(audio->setupProcessing(setup) == kResultOk &&
                  component->setActive(true) == kResultOk &&
                  audio->setProcessing(true) == kResultOk,
              "activate VST3 playback");
    }
    void suspend() {
        check(audio->setProcessing(false) == kResultOk && component->setActive(false) == kResultOk,
              "suspend VST3 playback");
    }
    Audio render(bool note = false, IParameterChanges *parameters = nullptr, bool playing = true,
                 ClockReport clock = ClockReport::normal, bool releaseKey = false) {
        Audio result{};
        float *channels[]{result[0].data(), result[1].data()};
        AudioBusBuffers bus{};
        bus.numChannels = 2;
        bus.channelBuffers32 = channels;
        ProcessContext transport{};
        transport.sampleRate = 48000;
        transport.projectTimeSamples =
            clock == ClockReport::rebased ? 0 : cursor + (clock == ClockReport::rounded ? 1 : 0);
        transport.projectTimeMusic = double(cursor) / 24000.;
        transport.tempo = 120;
        transport.state = ProcessContext::kProjectTimeMusicValid | ProcessContext::kTempoValid |
                          (playing ? ProcessContext::kPlaying : 0);
        Note input;
        input.releaseKey = releaseKey;
        ProcessData data;
        data.processMode = processingMode;
        data.numSamples = 256;
        data.numOutputs = 1;
        data.outputs = &bus;
        data.processContext = clock == ClockReport::missing ? nullptr : &transport;
        data.inputEvents = note || releaseKey ? &input : nullptr;
        data.inputParameterChanges = parameters;
        check(audio->process(data) == kResultOk, "render VST3 audio");
        cursor += data.numSamples;
        return result;
    }
    ~Player() {
        audio->setProcessing(false);
        component->setActive(false);
        component->terminate();
        audio->release();
        component->release();
    }
};
float peak(const Audio &audio) {
    float value = 0;
    for (const auto &channel : audio)
        for (float sample : channel) {
            check(std::isfinite(sample), "finite VST3 stereo output");
            value = std::max(value, std::abs(sample));
        }
    return value;
}
void continuity(IPluginFactory &factory, const TUID &cid, int model) {
    SourceMode mode(model);
    Player first(factory, cid), reference(factory, cid);
    first.render(true, &mode);
    reference.render(true, &mode);
    for (int i = 0; i < 8; ++i) {
        first.render();
        reference.render();
    }
    for (auto report : {ClockReport::missing, ClockReport::missing, ClockReport::normal,
                        ClockReport::rebased, ClockReport::normal, ClockReport::rounded}) {
        auto expected = reference.render(), actual = first.render(false, nullptr, true, report);
        check(peak(expected) > .00001f && actual == expected,
              "held VST3 piano-roll note lost during missing/rebased transport report");
    }
    first.suspend();
    {
        Player second(factory, cid);
        second.render(true, &mode);
    }
    first.start(1024);
    for (int i = 0; i < 16; ++i) {
        const auto report = i == 0 ? ClockReport::missing : ClockReport::normal;
        const auto expected = reference.render(), actual = first.render(false, nullptr, true, report);
        check(peak(expected) > .00001f, "held reference voice is audible");
        check(actual == expected, "held VST3 note interrupted during same-rate graph restart");
    }
    // Note Off must remain operative even in a context-free audio block.
    check(first.render(false, nullptr, true, ClockReport::missing, true) ==
              reference.render(false, nullptr, true, ClockReport::normal, true),
          "note-off during clock gap");
    for (int i = 0; i < 180; ++i)
        check(first.render() == reference.render(), "release envelope diverged after clock gap");
    check(peak(first.render()) < .00001f, "a real note-off releases the held key");
    first.render(true);
    first.suspend();
    first.start(256);
    first.cursor += 48000;
    check(peak(first.render()) == 0, "transport seek releases suspended VST3 voices");
    check(peak(first.render(true)) > .00001f, "VST3 responds after seek");
    first.suspend();
    first.start(256);
    check(peak(first.render(false, nullptr, false)) == 0, "stopped VST3 transport clears the note");
}

void liveMidiContinuity(IPluginFactory &factory, const TUID &cid, int model, ClockReport report) {
    SourceMode mode(model);
    Player first(factory, cid), reference(factory, cid);
    first.render(true, &mode, false, report);
    reference.render(true, &mode, false, report);
    for (int i = 0; i < 8; ++i) {
        first.render(false, nullptr, false, report);
        reference.render(false, nullptr, false, report);
    }

    // The DAW can rebuild the graph for a second instrument without sending a
    // transport transition. The original key has been down since the first block.
    first.suspend();
    {
        Player second(factory, cid);
        second.render(true, &mode, false, report);
        first.start(1024);
        const auto expected = reference.render(false, nullptr, false, report);
        const auto actual = first.render(false, nullptr, false, report);
        check(peak(expected) > .00001f && actual == expected,
              report == ClockReport::missing ? "restart cut a note started without transport"
                                             : "restart cut a live note with stopped transport");
        second.render(false, nullptr, false, report, true);
    }
    check(first.render(false, nullptr, false, report) ==
              reference.render(false, nullptr, false, report),
          "another instance's note-off or destruction affected the held note");
    for (int block = 0; block < 180; ++block) {
        const bool releaseKey = block == 0;
        check(first.render(false, nullptr, false, report, releaseKey) ==
                  reference.render(false, nullptr, false, report, releaseKey),
              "live note-off or release envelope diverged after graph restart");
    }
    check(peak(first.render(false, nullptr, false, report)) < .00001f,
          "live MIDI note-off ends the held note");
}

template <size_t N> struct WheelChanges : Borrowed<IParameterChanges> {
    std::array<Point, N> points;
    explicit WheelChanges(std::array<Point, N> data) : points(data) {}
    int32 PLUGIN_API getParameterCount() override {
        return int32(points.size());
    }
    IParamValueQueue *PLUGIN_API getParameterData(int32 i) override {
        return &points.at(size_t(i));
    }
    IParamValueQueue *PLUGIN_API addParameterData(const ParamID &, int32 &) override {
        return nullptr;
    }
};

void pitchBend(IPluginFactory &factory, const TUID &cid, bool flStudio = false) {
    FlStudioHost flStudioHost;
    FUnknown *host = flStudio ? &flStudioHost : nullptr;
    Player player(factory, cid, host);
    TUID controllerId{};
    IEditController *controller = nullptr;
    check(player.component->getControllerClassId(controllerId) == kResultOk &&
              factory.createInstance(controllerId, IEditController_iid,
                                     reinterpret_cast<void **>(&controller)) == kResultOk &&
              controller->initialize(host) == kResultOk,
          "create VST3 edit controller");
    IConnectionPoint *componentLink = nullptr, *controllerLink = nullptr;
    check(player.component->queryInterface(IConnectionPoint_iid,
                                           reinterpret_cast<void **>(&componentLink)) ==
                  kResultOk &&
              controller->queryInterface(IConnectionPoint_iid,
                                         reinterpret_cast<void **>(&controllerLink)) == kResultOk &&
              controllerLink->connect(componentLink) == kResultOk &&
              componentLink->connect(controllerLink) == kResultOk,
          "connect VST3 component and edit controller");
    IMidiMapping *mapping = nullptr;
    check(controller->queryInterface(IMidiMapping_iid, reinterpret_cast<void **>(&mapping)) ==
              kResultOk,
          "VST3 offers MIDI mapping");
    auto midiId = [&](CtrlNumber number) {
        ParamID id{};
        check(mapping->getMidiControllerAssignment(0, 0, number, id) == kResultOk,
              "VST3 MIDI channel 1 controller mapping");
        return id;
    };
    const auto wheel = midiId(kPitchBend);
    const auto modulation = midiId(1);
    const auto rpnMsb = midiId(101);
    const auto rpnLsb = midiId(100);
    const auto dataEntry = midiId(6);
    mapping->release();

    const auto bend = Point("bendwheel", 0.).id;
    const auto mod = Point("modwheel", 0.).id;
    int automatableWheels = 0;
    for (int32 i = 0; i < controller->getParameterCount(); ++i) {
        ParameterInfo info{};
        if (controller->getParameterInfo(i, info) == kResultOk &&
            (info.id == bend || info.id == mod) && (info.flags & ParameterInfo::kCanAutomate))
            ++automatableWheels;
    }
    check(automatableWheels == 2, "Bend and Mod are automatable VST3 parameters");

    WheelChanges sound{std::array{Point("o1wave", 0.), Point("o2level", 0.), Point("p1amps", 1.)}};
    player.render(true, &sound);
    auto measure = [&](ParamID id, double position, int cents) {
        WheelChanges changes{std::array{Point(id, position)}};
        player.render(false, &changes);
        for (int block = 0; block < 64; ++block)
            player.render();
        int sampleIndex = 0, crossings = 0;
        float previous = 0;
        double first = 0, last = 0;
        for (int block = 0; block < 128; ++block) {
            const auto audio = player.render();
            for (float sample : audio[0]) {
                if (sampleIndex > 0 && previous <= 0 && sample > 0) {
                    last = sampleIndex - 1. - previous / (sample - previous);
                    if (crossings++ == 0)
                        first = last;
                }
                previous = sample;
                ++sampleIndex;
            }
        }
        check(crossings > 2, "audible periodic VST3 bend output");
        const double actual = 48000. * (crossings - 1) / (last - first);
        const double expected = 440. * std::exp2((-9. + cents / 100.) / 12.);
        std::cout << (flStudio ? "FL Studio host profile" : "VST3 host") << ", "
                  << cents << " cents: " << actual
                  << " Hz (expected " << expected << " Hz)\n";
        check(std::abs(1200. * std::log2(actual / expected)) < 1.,
              "VST3 measured pitch bend within one cent");
    };
    measure(wheel, .5, 0);
    measure(wheel, 1., flStudio ? 200 : 1200);
    measure(wheel, 0., flStudio ? -200 : -1200);
    if (flStudio) {
        measure(wheel, .75, 100);
        measure(wheel, .25, -100);
        WheelChanges octaveRange{std::array{Point(rpnMsb, 0.), Point(rpnLsb, 0.),
                                            Point(dataEntry, 12. / 128.)}};
        player.render(false, &octaveRange);
        measure(wheel, (8192. + 1365.) / 16384., 200);
        measure(wheel, (8192. - 1365.) / 16384., -200);
        measure(wheel, 1., 1200);
        measure(wheel, 0., -1200);
    }

    // A host such as FL Studio can send MIDI Pitch Bend Sensitivity before its wheel data.
    WheelChanges rpn{std::array{Point(rpnMsb, 0.), Point(rpnLsb, 0.),
                                Point(dataEntry, 2. / 128.)}};
    player.render(false, &rpn);
    measure(wheel, 1., 200);
    measure(wheel, 0., -200);
    measure(bend, .75, 600);
    measure(bend, .25, -600);
    measure(bend, .5, 0);

    WheelChanges range{std::array{Point("bendrange", 1.)}};
    player.render(false, &range);
    measure(bend, 1., 2400);
    measure(bend, 0., -2400);
    measure(bend, .5, 0);

    // Route Mod Wheel to Pitch A to measure the result of host automation and MIDI CC1.
    WheelChanges route{std::array{Point("misc1source", 4. / 14.), Point("misc1dest1", 4. / 58.),
                                  Point("misc1amt1", .5 + (2. / 4.8) / 20.)}};
    player.render(false, &route);
    measure(mod, .5, 100);
    measure(mod, 0., 0);
    measure(modulation, 1., 200);
    measure(modulation, 0., 0);

    componentLink->disconnect(controllerLink);
    controllerLink->disconnect(componentLink);
    componentLink->release();
    controllerLink->release();
    controller->terminate();
    controller->release();
}

void offlineRestart(IPluginFactory &factory, const TUID &cid) {
    Player player(factory, cid);
    check(peak(player.render(true, nullptr, false, ClockReport::missing)) > .00001f,
          "realtime note audible before offline render");
    player.suspend();
    player.start(256, kOffline);
    check(peak(player.render(false, nullptr, false, ClockReport::missing)) == 0,
          "offline render inherited a realtime voice");
    check(peak(player.render(true, nullptr, false, ClockReport::missing)) > .00001f,
          "offline render accepts MIDI");
    player.suspend();
    player.start(256, kOffline);
    check(peak(player.render(false, nullptr, false, ClockReport::missing)) == 0,
          "offline render inherited a voice from the previous render");
}
} // namespace
int main(int argc, char **argv) {
    try {
        check(argc == 2, "supply VST3 module path");
        Module module(argv[1]);
        auto *factory = module.symbol<IPluginFactory *(PLUGIN_API *)()>("GetPluginFactory")();
        PFactoryInfo manufacturer{};
        check(factory && factory->getFactoryInfo(&manufacturer) == kResultOk &&
                  std::strcmp(manufacturer.vendor, "wavelogik") == 0,
              "VST3 factory identity");
        bool found = false;
        for (int32 i = 0; i < factory->countClasses(); ++i) {
            PClassInfo info{};
            if (factory->getClassInfo(i, &info) == kResultOk &&
                std::strcmp(info.category, "Audio Module Class") == 0 &&
                std::strcmp(info.name, "logisth") == 0) {
                found = true;
                for (int model = -1; model < 3; ++model) {
                    continuity(*factory, info.cid, model);
                    liveMidiContinuity(*factory, info.cid, model, ClockReport::missing);
                    liveMidiContinuity(*factory, info.cid, model, ClockReport::normal);
                }
                offlineRestart(*factory, info.cid);
                pitchBend(*factory, info.cid);
                pitchBend(*factory, info.cid, true);
            }
        }
        factory->release();
        check(found, "logisth VST3 instrument class");
        std::cout << "PASS VST3 Classic, Sampler, Cloud and Granulizer through realtime graph "
                     "restarts with playing, stopped and missing transport; note-off, seek, stop "
                     "and offline reset\n";
    } catch (const std::exception &error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
