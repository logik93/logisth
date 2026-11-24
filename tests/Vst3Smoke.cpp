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
    Player(IPluginFactory &factory, const TUID &cid) {
        check(factory.createInstance(cid, IComponent_iid, reinterpret_cast<void **>(&component)) ==
                      kResultOk &&
                  component->initialize(nullptr) == kResultOk,
              "create VST3 instrument");
        check(component->queryInterface(IAudioProcessor_iid, reinterpret_cast<void **>(&audio)) ==
                  kResultOk,
              "VST3 processor interface");
        check(component->activateBus(kAudio, kOutput, 0, true) == kResultOk &&
                  component->activateBus(kEvent, kInput, 0, true) == kResultOk,
              "VST3 audio and event buses");
        start(256);
    }
    void start(int block) {
        ProcessSetup setup{kRealtime, kSample32, block, 48000.};
        check(audio->setupProcessing(setup) == kResultOk &&
                  component->setActive(true) == kResultOk &&
                  audio->setProcessing(true) == kResultOk,
              "activate VST3 playback");
    }
    void suspend() {
        check(audio->setProcessing(false) == kResultOk && component->setActive(false) == kResultOk,
              "suspend VST3 playback");
    }
    Audio render(bool note = false, SourceMode *mode = nullptr, bool playing = true,
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
        data.numSamples = 256;
        data.numOutputs = 1;
        data.outputs = &bus;
        data.processContext = clock == ClockReport::missing ? nullptr : &transport;
        data.inputEvents = note || releaseKey ? &input : nullptr;
        data.inputParameterChanges = mode;
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
                for (int model = -1; model < 3; ++model)
                    continuity(*factory, info.cid, model);
            }
        }
        factory->release();
        check(found, "logisth VST3 instrument class");
        std::cout << "PASS VST3 Classic, Sampler, Cloud and Granulizer held notes, clock gaps, "
                     "sample-counter rebase, graph restart, note-off, seek and stop\n";
    } catch (const std::exception &error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
