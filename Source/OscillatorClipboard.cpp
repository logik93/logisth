#include "PluginProcessor.h"
using namespace juce;
using namespace logisth;
namespace {
struct OscillatorSettings {
    std::vector<std::pair<String, float>> values;
    std::shared_ptr<const SourceData> data;
};
CriticalSection clipboardLock;
std::shared_ptr<const OscillatorSettings> clipboard;
} // namespace
bool LogisthProcessor::hasOscillatorClipboard() {
    const ScopedLock lock(clipboardLock);
    return bool(clipboard);
}
Result LogisthProcessor::copyOscillator(int slot) {
    if (slot < 0 || slot >= playerCount)
        return Result::fail("Invalid oscillator slot");
    auto copy = std::make_shared<OscillatorSettings>();
    const String prefix = "o" + String(slot + 1);
    copy->data = sourceData(slot);
    for (const auto &spec : parameterSpecs) {
        String id(spec.id);
        if (!id.startsWith(prefix))
            continue;
        String suffix = id.substring(2);
        if (suffix == "fmsourcecx" || suffix == "amsourcecx")
            continue;
        float value = state.getRawParameterValue(id)->load();
        if (suffix == "fmsource" || suffix == "amsource")
            value = get(Param(playerParamIndex(
                suffix == "fmsource" ? Param::o1fmsource : Param::o1amsource, slot, 5)));
        copy->values.emplace_back(suffix, value);
    }
    const ScopedLock lock(clipboardLock);
    clipboard = std::move(copy);
    return Result::ok();
}
Result LogisthProcessor::pasteOscillator(int slot) {
    if (slot < 0 || slot >= playerCount)
        return Result::fail("Invalid oscillator slot");
    std::shared_ptr<const OscillatorSettings> copy;
    {
        const ScopedLock lock(clipboardLock);
        copy = clipboard;
    }
    if (!copy)
        return Result::fail("Copy an oscillator first.");
    const String prefix = "o" + String(slot + 1);
    std::vector<std::pair<RangedAudioParameter *, float>> writes;
    for (const auto &item : copy->values) {
        auto *parameter = state.getParameter(prefix + item.first);
        if (!parameter)
            return Result::fail("Incomplete oscillator settings.");
        if (item.first == "fmsource" || item.first == "amsource") {
            auto *extension = state.getParameter(prefix + item.first + "cx");
            if (extension) {
                writes.emplace_back(parameter, item.second > 4 ? 0 : item.second);
                writes.emplace_back(extension, item.second > 4 ? item.second - 4 : 0);
            } else
                writes.emplace_back(parameter, item.second);
        } else
            writes.emplace_back(parameter, item.second);
    }
    // Publish validates the complete active-source budget before changing controls.
    if (!publishSource(slot, copy->data))
        return Result::fail("Pasting this source would exceed the sample memory budget.");
    for (auto &item : writes) {
        item.first->beginChangeGesture();
        item.first->setValueNotifyingHost(item.first->convertTo0to1(item.second));
        item.first->endChangeGesture();
    }
    ensureSpectral(false);
    return Result::ok();
}
