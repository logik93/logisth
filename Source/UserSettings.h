#pragma once
#include <array>
#include <atomic>
#include <cmath>
#include <juce_core/juce_core.h>

namespace logisth {
// Application preferences, independent of sound presets and project state.
// Call only from UI/import workers; the audio callback never accesses this store.
class UserSettings {
  public:
    enum class Folder { PresetOpen, PresetSave, Sample, Impulse, Library, Count };
    struct Text {
        double size = 100;
        int style = 0; // Theme, Regular, Bold, Italic, Bold italic.
        bool antialias = true;
    };
    struct Analyzer {
        int mode = 3, tilt = 2; // Off, Pre, Post, Both; 0/1.5/3/4.5/6 dB/oct.
    };
    static Text text() {
        auto &s = store();
        const juce::ScopedLock lock(s.lock);
        s.read();
        return s.typography;
    }
    static Analyzer analyzer() {
        auto &s = store();
        const juce::ScopedLock lock(s.lock);
        s.read();
        return s.spectrum;
    }
    static bool setText(Text value) {
        if (!std::isfinite(value.size) || value.size < 80 || value.size > 125 || value.style < 0 ||
            value.style > 4)
            return false;
        auto &s = store();
        const juce::ScopedLock lock(s.lock);
        s.read();
        s.typography = value;
        ++s.revision;
        return s.write();
    }
    static bool setAnalyzer(Analyzer value) {
        if (value.mode < 0 || value.mode > 3 || value.tilt < 0 || value.tilt > 4)
            return false;
        auto &s = store();
        const juce::ScopedLock lock(s.lock);
        s.read();
        s.spectrum = value;
        ++s.revision;
        return s.write();
    }
    static juce::File folder(Folder kind, const juce::File &fallback = {}) {
        auto &s = store();
        const juce::ScopedLock lock(s.lock);
        s.read();
        auto path = s.folders[size_t(kind)];
        while (path != juce::File{} && !path.isDirectory()) {
            auto parent = path.getParentDirectory();
            if (parent == path)
                break;
            path = parent;
        }
        return path.isDirectory() ? path : fallback;
    }
    static bool remember(Folder kind, const juce::File &directory) {
        if (directory == juce::File{} || !directory.isDirectory())
            return false;
        auto &s = store();
        const juce::ScopedLock lock(s.lock);
        s.read();
        if (s.folders[size_t(kind)] == directory)
            return true;
        s.folders[size_t(kind)] = directory;
        ++s.revision;
        return s.write();
    }
    static uint64_t revision() {
        return store().revision.load();
    }
    static juce::File preferencesFile() {
        auto &s = store();
        const juce::ScopedLock lock(s.lock);
        return s.file();
    }
    static void useFileForTests(const juce::File &file) {
        auto &s = store();
        const juce::ScopedLock lock(s.lock);
        s.overrideFile = file;
        s.loaded = false;
        s.typography = {};
        s.spectrum = {};
        s.folders = {};
        ++s.revision;
    }

  private:
    inline static const std::array<const char *, size_t(Folder::Count)> keys{
        "presetOpen", "presetSave", "sampleOpen", "impulseOpen", "presetLibrary"};
    struct Store {
        juce::CriticalSection lock;
        juce::File overrideFile;
        Text typography;
        Analyzer spectrum;
        std::array<juce::File, size_t(Folder::Count)> folders;
        bool loaded = false;
        std::atomic<uint64_t> revision{0};
        juce::File file() const {
            return overrideFile != juce::File{}
                       ? overrideFile
                       : juce::File::getSpecialLocation(juce::File::userApplicationDataDirectory)
                             .getChildFile("logisth/preferences.json");
        }
        void read() {
            if (loaded)
                return;
            loaded = true;
            auto path = file();
            if (!path.existsAsFile() || path.getSize() > 65536)
                return;
            auto json = juce::JSON::parse(path.loadFileAsString());
            auto *o = json.getDynamicObject();
            if (!o)
                return;
            const auto size = o->getProperty("textSize"), style = o->getProperty("textStyle"),
                       aa = o->getProperty("textAntialias");
            if ((size.isInt() || size.isDouble()) && std::isfinite(double(size)) &&
                double(size) >= 80 && double(size) <= 125)
                typography.size = double(size);
            if (style.isInt() && int(style) >= 0 && int(style) <= 4)
                typography.style = int(style);
            if (aa.isBool())
                typography.antialias = bool(aa);
            auto mode = o->getProperty("analyzerMode"), tilt = o->getProperty("analyzerTilt");
            if (mode.isInt() && int(mode) >= 0 && int(mode) <= 3)
                spectrum.mode = int(mode);
            if (tilt.isInt() && int(tilt) >= 0 && int(tilt) <= 4)
                spectrum.tilt = int(tilt);
            for (size_t i = 0; i < keys.size(); ++i) {
                const auto value = o->getProperty(keys[i]);
                if (value.isString() && value.toString().isNotEmpty() &&
                    juce::File::isAbsolutePath(value.toString()))
                    folders[i] = juce::File(value.toString());
            }
        }
        bool write() const {
            auto *o = new juce::DynamicObject;
            const juce::var json(o);
            o->setProperty("version", 1);
            o->setProperty("textSize", typography.size);
            o->setProperty("textStyle", typography.style);
            o->setProperty("textAntialias", typography.antialias);
            o->setProperty("analyzerMode", spectrum.mode);
            o->setProperty("analyzerTilt", spectrum.tilt);
            for (size_t i = 0; i < keys.size(); ++i)
                o->setProperty(keys[i], folders[i].getFullPathName());
            const auto target = file();
            if (target.getParentDirectory().createDirectory().failed())
                return false;
            juce::TemporaryFile temporary(target);
            return temporary.getFile().replaceWithText(juce::JSON::toString(json) + "\n") &&
                   temporary.overwriteTargetFileWithTemporary();
        }
    };
    static Store &store() {
        static Store value;
        return value;
    }
};
} // namespace logisth
