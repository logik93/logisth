#include "PluginProcessor.h"
#include <algorithm>
#include <cmath>
using namespace juce;
using namespace logisth;
namespace {
bool fitsImpulse(uint64_t frames, double sourceRate, double hostRate) {
    return frames > 0 && frames <= impulseMemoryBudget / 8 && std::isfinite(sourceRate) &&
           sourceRate >= 8000 && sourceRate <= 192000 &&
           double(frames) * std::max(8000., hostRate) / sourceRate <=
               double(impulseMemoryBudget / 8);
}
} // namespace
std::shared_ptr<const ImpulseData> LogisthProcessor::impulseData() const {
    const ScopedLock lock(assetLock);
    return impulseOwner;
}
void LogisthProcessor::publishImpulse(std::shared_ptr<const ImpulseData> data) {
    const ScopedLock lock(assetLock);
    ++impulseImportRequest;
    impulseDuration = double(data->samples[0].size()) / data->sampleRate;
    impulseOwner = std::move(data);
    ++impulseRevision;
    ++impulseGeneration;
    impulseRejectedGeneration = 0;
}
void LogisthProcessor::clearImpulse() {
    publishImpulse(ImpulseData::factory());
    ensureImpulse(false);
}
Result LogisthProcessor::loadImpulse(const File &file) {
    uint64_t request;
    {
        const ScopedLock lock(assetLock);
        request = ++impulseImportRequest;
    }
    return loadImpulseRequested(file, request);
}
Result LogisthProcessor::loadImpulseRequested(const File &file, uint64_t request) try {
    AudioFormatManager formats;
    formats.registerBasicFormats();
    std::unique_ptr<AudioFormatReader> reader(formats.createReaderFor(file));
    if (!reader)
        return Result::fail("Cannot open the impulse response. Use WAV, AIFF or FLAC.");
    if (reader->numChannels < 1 || reader->numChannels > 2)
        return Result::fail("Use a mono or stereo impulse response.");
    if (!fitsImpulse(uint64_t(reader->lengthInSamples), reader->sampleRate, impulseRate.load()))
        return Result::fail("Impulse response exceeds the 32 MiB decoded/resampled memory budget.");
    AudioBuffer<float> audio(2, int(reader->lengthInSamples));
    if (!reader->read(&audio, 0, audio.getNumSamples(), 0, true, true))
        return Result::fail("Cannot decode the impulse response.");
    if (reader->numChannels == 1)
        audio.copyFrom(1, 0, audio, 0, 0, audio.getNumSamples());
    auto data = std::make_shared<ImpulseData>();
    data->name = file.getFileName().substring(0, 128);
    data->sampleRate = reader->sampleRate;
    data->channels = int(reader->numChannels);
    float peak = 0;
    for (int ch = 0; ch < 2; ++ch) {
        auto &dst = data->samples[size_t(ch)];
        dst.assign(audio.getReadPointer(ch), audio.getReadPointer(ch) + audio.getNumSamples());
        for (float x : dst) {
            if (!std::isfinite(x) || std::abs(x) > 8)
                return Result::fail("Impulse response contains invalid samples.");
            peak = std::max(peak, std::abs(x));
        }
    }
    if (peak == 0)
        return Result::fail("The impulse response is silent.");
    data->buildPeaks();
    // Prepare and validate before replacing the audible or saved response.
    auto engine = std::make_shared<PreparedImpulse>(*data, impulseRate.load());
    const ScopedLock lock(assetLock);
    if (request != impulseImportRequest)
        return Result::fail("Impulse import was superseded by a newer patch or request.");
    if (engine->rate != impulseRate.load())
        return Result::fail("Audio sample rate changed during IR import. Please retry.");
    publishImpulse(data);
    impulseDesiredRevision = data->revision;
    impulseDesiredRate = engine->rate;
    impulseEngines.push_back(engine);
    impulseRack.publish(engine.get());
    assetStatus = "Impulse response ready: " + data->name;
    return Result::ok();
} catch (const std::exception &e) {
    return Result::fail("Cannot load impulse response: " + String(e.what()));
}
void LogisthProcessor::loadImpulseAsync(const File &file) {
    uint64_t request;
    {
        const ScopedLock lock(assetLock);
        request = ++impulseImportRequest;
        assetStatus = "Loading impulse response...";
    }
    importWorkers.addJob([this, file, request] {
        auto result = loadImpulseRequested(file, request);
        const ScopedLock lock(assetLock);
        if (result.failed() && request == impulseImportRequest)
            assetStatus = result.getErrorMessage();
    });
}
void LogisthProcessor::ensureImpulse(bool synchronous) {
    std::shared_ptr<const ImpulseData> data;
    double rate;
    uint64_t generation;
    {
        const ScopedLock lock(assetLock);
        if (raw[size_t(Param::reverbtype)]->load() != 5)
            return;
        data = impulseOwner;
        rate = impulseRate.load();
        if (!data)
            return;
        if (auto *ready = impulseRack.wanted();
            ready && ready->revision == data->revision && ready->rate == rate)
            return;
        if (impulseDesiredRevision != data->revision || impulseDesiredRate != rate) {
            impulseDesiredRevision = data->revision;
            impulseDesiredRate = rate;
            ++impulseGeneration;
        }
        generation = impulseGeneration.load();
        if (impulseRejectedGeneration == generation)
            return;
        if (!fitsImpulse(data->samples[0].size(), data->sampleRate, rate)) {
            impulseRejectedGeneration = generation;
            assetStatus = "IR too large at the current sample rate.";
            return;
        }
        if (impulsePending.load() && !synchronous)
            return;
        if (impulsePending.load() && synchronous)
            generation = ++impulseGeneration;
        impulsePending = true;
        impulsePendingGeneration = generation;
        assetStatus = "Preparing convolution reverb...";
    }
    auto build = [this, data, rate, generation] {
        try {
            auto engine = std::make_shared<PreparedImpulse>(*data, rate);
            const ScopedLock lock(assetLock);
            if (impulseGeneration.load() == generation && impulseRate.load() == rate) {
                impulseEngines.push_back(engine);
                impulseRack.publish(engine.get());
                assetStatus = "Impulse response ready: " + data->name;
            }
        } catch (const std::exception &e) {
            const ScopedLock lock(assetLock);
            if (impulseGeneration.load() == generation) {
                impulseRejectedGeneration = generation;
                assetStatus = "Cannot prepare convolution: " + String(e.what());
            }
        }
        const ScopedLock lock(assetLock);
        if (impulsePendingGeneration == generation)
            impulsePending = false;
    };
    if (synchronous)
        build();
    else
        importWorkers.addJob(std::move(build));
}
void LogisthProcessor::serviceImpulse() {
    ensureImpulse(false);
    std::vector<std::shared_ptr<PreparedImpulse>> retired;
    {
        const ScopedLock lock(assetLock);
        if (assetReaders.load() == 0) {
            auto it = impulseEngines.begin();
            while (it != impulseEngines.end())
                if (!impulseRack.referenced(it->get())) {
                    retired.push_back(std::move(*it));
                    it = impulseEngines.erase(it);
                } else
                    ++it;
        }
    }
}
bool LogisthProcessor::impulseReady() const {
    const ScopedLock lock(assetLock);
    auto *p = impulseRack.wanted();
    return p && impulseOwner && p->revision == impulseOwner->revision &&
           p->rate == impulseRate.load();
}
void LogisthProcessor::saveImpulse(ValueTree &tree) const {
    auto data = impulseData();
    if (!data || (data == ImpulseData::factory() && raw[size_t(Param::reverbtype)]->load() != 5))
        return;
    MemoryOutputStream compressed;
    {
        GZIPCompressorOutputStream zip(compressed);
        zip.writeDouble(data->sampleRate);
        zip.writeInt(int(data->samples[0].size()));
        zip.writeInt(data->channels);
        zip.writeString(data->name);
        for (int ch = 0; ch < data->channels; ++ch) {
#if JUCE_BIG_ENDIAN
            for (float sample : data->samples[size_t(ch)])
                zip.writeFloat(sample);
#else
            zip.write(data->samples[size_t(ch)].data(),
                      data->samples[size_t(ch)].size() * sizeof(float));
#endif
        }
    }
    ValueTree node("IMPULSE");
    node.setProperty("format", 1, nullptr);
    node.setProperty("audio", var(compressed.getMemoryBlock()), nullptr);
    tree.addChild(node, -1, nullptr);
}
std::shared_ptr<const ImpulseData> LogisthProcessor::readImpulse(const ValueTree &tree) const try {
    std::shared_ptr<const ImpulseData> result = ImpulseData::factory();
    bool found = false;
    for (auto node : tree)
        if (node.hasType("IMPULSE")) {
            if (found || int(node.getProperty("format", 0)) != 1)
                return {};
            found = true;
            auto *blob = node["audio"].getBinaryData();
            if (!blob || blob->getSize() > impulseMemoryBudget + 4096)
                return {};
            MemoryInputStream input(*blob, false);
            GZIPDecompressorInputStream zip(input);
            auto data = std::make_shared<ImpulseData>();
            data->sampleRate = zip.readDouble();
            int n = zip.readInt();
            data->channels = zip.readInt();
            if (n < 1 || data->channels < 1 || data->channels > 2 ||
                !fitsImpulse(uint64_t(n), data->sampleRate, impulseRate.load()))
                return {};
            char name[513]{};
            bool ended = false;
            for (int i = 0; i < 512; ++i) {
                if (zip.read(name + i, 1) != 1)
                    return {};
                if (name[i] == 0) {
                    ended = true;
                    break;
                }
            }
            if (!ended)
                return {};
            data->name = String::fromUTF8(name).substring(0, 128);
            float peak = 0;
            for (int ch = 0; ch < data->channels; ++ch) {
                auto &channel = data->samples[size_t(ch)];
                channel.resize(size_t(n));
#if JUCE_BIG_ENDIAN
                for (float &sample : channel)
                    sample = zip.readFloat();
#else
                const int bytes = n * int(sizeof(float));
                if (zip.read(channel.data(), bytes) != bytes)
                    return {};
#endif
                for (float x : channel) {
                    if (!std::isfinite(x) || std::abs(x) > 8)
                        return {};
                    peak = std::max(peak, std::abs(x));
                }
            }
            if (data->channels == 1)
                data->samples[1] = data->samples[0];
            char extra;
            if (peak == 0 || zip.read(&extra, 1) != 0)
                return {};
            data->buildPeaks();
            result = data;
        }
    return result;
} catch (const std::exception &) {
    return {};
}
