#pragma once
#include "Arpeggiator.h"
#include "Chords.h"
#include "Effects.h"
#include "Engine.h"
#include "ImpulseReverb.h"
#include "Morph.h"
#include "Parameters.h"
#include "UserSettings.h"
#include <bitset>
#include <juce_audio_utils/juce_audio_utils.h>
#include <optional>
namespace logisth {
struct ThemeDraft;
}
class LogisthProcessor : public juce::AudioProcessor {
  public:
    LogisthProcessor();
    int effectivePolyphony() const {
        return juce::jlimit(
            1, logisth::maxPolyphony,
            int(get(logisth::Param::polyphony) + get(logisth::Param::polyphonyextra)));
    }
    logisth::EffectOrder effectOrder() const noexcept {
        logisth::EffectOrder order;
        logisth::EffectOrder::unpack(effectOrderBits.load(), order);
        return order;
    }
    bool setEffectOrder(logisth::EffectOrder, bool notifyHost = true);
    bool moveEffect(int from, int to);
    logisth::DistortionOrder distortionOrder() const noexcept {
        logisth::DistortionOrder order;
        logisth::DistortionOrder::unpack(distortionOrderBits.load(), order);
        return order;
    }
    bool setDistortionOrder(logisth::DistortionOrder, bool notifyHost = true);
    bool moveDistortion(int from, int to);
    // GUI-only draft, deliberately excluded from sound presets and DAW audio state.
    std::shared_ptr<logisth::ThemeDraft> appearanceDraft;
    ~LogisthProcessor() override;
    juce::Result loadSource(int, const juce::File &, int frameSize = 0);
    void loadSourceAsync(int, const juce::File &, int frameSize = 0,
                         bool switchClassicToSampler = false);
    void clearSource(int);
    juce::Result loadImpulse(const juce::File &);
    void loadImpulseAsync(const juce::File &);
    void clearImpulse();
    void ensureImpulse(bool synchronous = false);
    bool impulseReady() const;
    std::shared_ptr<const logisth::ImpulseData> impulseData() const;
    std::atomic<uint64_t> impulseRevision{0};
    juce::Result copyOscillator(int);
    juce::Result pasteOscillator(int);
    static bool hasOscillatorClipboard();
    std::shared_ptr<const logisth::SourceData> sourceData(int) const;
    juce::String sourceStatus() const;
    std::shared_ptr<const logisth::ChordMap> chordMap() const;
    void setChord(int, int);
    void setChordNote(int, int, int);
    void learnChord(int);
    void finishLearn();
    int learning() const {
        return learnTarget.load();
    }
    void storePattern(int);
    void recallPattern(int);
    void randomisePattern();
    void serviceAssets();
    void ensureSpectral(bool synchronous = false);
    bool spectralReady(int slot) const;
    std::atomic<int> arpPlayhead{-1};
    logisth::ScopeBuffer scope;
    mutable logisth::ScopeBuffer eqSpectrum;
    std::atomic<bool> eqSpectrumEnabled{false};
    mutable logisth::ScopeBuffer distortionSpectrum;
    std::atomic<bool> distortionSpectrumEnabled{false};
    std::array<logisth::ScopeBuffer, logisth::playerCount> playerScopes;
    std::array<logisth::PlaybackTelemetry, logisth::playerCount> playback;
    void copyPattern(int, int);
    void setPatternName(int, const juce::String &);
    juce::String patternName(int) const;
    juce::File presetDirectory() const;
    void setPresetDirectory(const juce::File &);
    juce::Result savePresetFile(const juce::File &);
    juce::Result loadPresetFile(const juce::File &);
    void savePresetAsync(const juce::File &);
    void loadPresetAsync(const juce::File &);
    juce::String currentPresetName() const;
    juce::String presetComment() const;
    void setPresetComment(const juce::String &);
    std::atomic<int> commentRevision{0};
    juce::File currentPresetFile() const;
    int activePattern() const {
        return currentPattern.load();
    }
    std::atomic<int> presetRevision{0};
    bool applyState(const void *, int);
    const juce::String getName() const override {
        return "logisth";
    }
    void prepareToPlay(double, int) override;
    void releaseResources() override {}
    void reset() override;
    bool isBusesLayoutSupported(const BusesLayout &l) const override {
        return l.getMainOutputChannelSet() == juce::AudioChannelSet::stereo() &&
               l.getMainInputChannelSet().isDisabled();
    }
    void processBlock(juce::AudioBuffer<float> &, juce::MidiBuffer &) override;
    juce::AudioProcessorEditor *createEditor() override;
    bool hasEditor() const override {
        return true;
    }
    bool acceptsMidi() const override {
        return true;
    }
    bool producesMidi() const override {
        return true;
    }
    bool isMidiEffect() const override {
        return false;
    }
    double getTailLengthSeconds() const override {
        const bool convolution = raw[size_t(logisth::Param::reverbon)]->load() > 0 &&
                                 raw[size_t(logisth::Param::reverbtype)]->load() == 5;
        return 60 + (convolution ? impulseDuration.load() : 0);
    }
    int getNumPrograms() override {
        return 1;
    }
    int getCurrentProgram() override {
        return 0;
    }
    void setCurrentProgram(int) override {}
    const juce::String getProgramName(int) override {
        return "Default";
    }
    void changeProgramName(int, const juce::String &) override {}
    void getStateInformation(juce::MemoryBlock &) override;
    void setStateInformation(const void *, int) override;
    void loadPreset(int);
    juce::String macroName(int) const;
    void setMacroName(int, const juce::String &);
    void mutate(uint64_t seed = 0);
    void undoMutation();
    bool canUndoMutation() const {
        const juce::ScopedLock lock(assetLock);
        return mutationUndo.has_value();
    }
    int mutationLocks() const {
        return mutateLocks;
    }
    void setMutationLocks(int bits) {
        mutateLocks = bits & 15;
    }
    std::atomic<int> macroRevision{0};
    float effectiveValue(logisth::Param id) const {
        return get(id);
    }
    juce::AudioProcessorValueTreeState state;
    juce::MidiKeyboardState keyboard;
    void setUiWheel(bool pitch, float value) {
        if (pitch) {
            uiPitchPosition = value;
            uiChanges.fetch_or(1);
        } else {
            wheelPosition = value;
            uiChanges.fetch_or(2);
        }
    }
    std::atomic<float> pitchPosition{0}, wheelPosition{0};
    std::atomic<float> uiPitchPosition{0};
    int lastPitchChannel = 1;
    std::atomic<float> peak{0};
    std::array<std::atomic<float>, 2> channelPeaks{};
    std::atomic<int> voices{0};
    std::array<logisth::DynamicsMeters, 3> dynamicsMeters;
    logisth::ScopeBuffer dynamicsScope;
    std::atomic<int> dynamicsView{-1};
    std::atomic<uint64_t> effectSamples{0};

  private:
    static juce::AudioProcessorValueTreeState::ParameterLayout layout();
    float get(logisth::Param id) const {
        return logisth::morphedValue(id, [this](size_t i) {
            return logisth::expandedChoiceValue(logisth::Param(i),
                                                [this](size_t at) { return raw[at]->load(); });
        });
    }
    logisth::Clipper clipper;
    logisth::OutputLimiter limiter;
    bool wasMidiOnly = false;
    int lastDynamicsDisplay = -1;
    logisth::Params snapshot() const;
    logisth::EffectsParams effectsSnapshot() const;
    logisth::ArpParams arpSnapshot() const;
    std::array<std::atomic<float> *, logisth::parameterCount> raw{};
    bool publishSource(int, std::shared_ptr<const logisth::SourceData>);
    juce::Result loadSourceRequested(int, const juce::File &, int frameSize, uint64_t request,
                                     bool switchClassicToSampler);
    std::array<uint64_t, logisth::playerCount> sourceImportRequest{};
    void publishChords(std::shared_ptr<const logisth::ChordMap>);
    void publishImpulse(std::shared_ptr<const logisth::ImpulseData>);
    juce::Result loadImpulseRequested(const juce::File &, uint64_t);
    uint64_t impulseImportRequest = 0;
    void serviceImpulse();
    void saveImpulse(juce::ValueTree &) const;
    std::shared_ptr<const logisth::ImpulseData> readImpulse(const juce::ValueTree &) const;
    mutable logisth::ImpulseRack impulseRack;
    std::shared_ptr<const logisth::ImpulseData> impulseOwner = logisth::ImpulseData::factory();
    std::vector<std::shared_ptr<logisth::PreparedImpulse>> impulseEngines;
    std::atomic<double> impulseRate{44100}, impulseDuration{1};
    std::atomic<uint64_t> impulseGeneration{0};
    uint64_t impulsePendingGeneration = 0, impulseRejectedGeneration = 0,
             impulseDesiredRevision = 0;
    double impulseDesiredRate = 0;
    std::atomic<bool> impulsePending{false};
    void saveAssets(juce::ValueTree &);
    void saveMacroMetadata(juce::ValueTree &) const;
    bool restoreAssets(const juce::ValueTree &);
    mutable juce::CriticalSection assetLock;
    std::shared_ptr<logisth::SpectralBank> spectralOwner;
    std::vector<std::shared_ptr<logisth::SpectralBank>> retiredSpectral;
    std::atomic<logisth::SpectralBank *> spectralPointer{nullptr};
    std::atomic<bool> spectralPending{false};
    std::atomic<uint64_t> spectralGeneration{0};
    uint64_t spectralPendingGeneration = 0, spectralRejectedGeneration = 0;
    std::array<logisth::SpectralConfig, logisth::playerCount> spectralDesiredConfigs{};
    std::array<int, logisth::playerCount> spectralDesiredUnisons{};
    unsigned spectralDesiredMask = 0;
    int spectralDesiredVoices = 0;
    double spectralDesiredRate = 0;
    std::atomic<double> spectralRate{44100};
    std::array<std::shared_ptr<const logisth::SourceData>, logisth::playerCount> sourceOwners;
    std::array<std::atomic<const logisth::SourceData *>, logisth::playerCount> sourcePointers{};
    std::vector<std::shared_ptr<const logisth::SourceData>> retiredSources;
    std::shared_ptr<const logisth::ChordMap> chordOwner;
    std::vector<std::shared_ptr<const logisth::ChordMap>> retiredChords;
    std::atomic<const logisth::ChordMap *> chordPointer{nullptr};
    std::atomic<int> assetReaders{0}, learnTarget{-1};
    std::array<std::atomic<uint32_t>, 4> learned{};
    juce::String assetStatus;
    std::array<juce::ValueTree, 12> patterns;
    juce::File userPresetFolder, activePresetFile;
    juce::String presetName, comment;
    std::array<juce::String, 4> macroNames{"Macro 1", "Macro 2", "Macro 3", "Macro 4"};
    std::atomic<int> mutateLocks{0};
    std::optional<std::array<float, logisth::parameterCount>> mutationUndo;
    std::atomic<int> currentPattern{0};
    juce::ThreadPool importWorkers{1};
    logisth::ChordMapper chordMapper;
    bool chordsWereEnabled = false, midiWasEnabled = false;
    struct OutputChannel {
        std::bitset<128> keys;
        bool sustain = false;
        void release(juce::MidiBuffer &, int channel, int offset);
    };
    std::array<OutputChannel, 16> outputChannels{};
    juce::MidiBuffer generatedMidi;
    double preparedSampleRate = 0;
    bool pendingVstRestart = false, releaseOutputNotes = false;
    void resetDsp();
    logisth::Engine engine;
    logisth::Effects effects;
    std::atomic<uint32_t> effectOrderBits{logisth::EffectOrder{}.packed()};
    std::atomic<unsigned> distortionOrderBits{logisth::DistortionOrder{}.packed()};
    bool lastEqCapture = false, lastDistortionCapture = false;
    logisth::RackTransition rackTransition;
    logisth::Arpeggiator arp;
    std::array<std::array<float, 512>, logisth::playerCount> playerTraces{};
    std::array<float, 512> driveMod{}, phaserMod{}, partScopeA{}, partScopeB{}, partScopeC{};
    std::atomic<int> uiChanges{0};
    class PreparationThread final : public juce::Thread {
        LogisthProcessor &owner;

      public:
        explicit PreparationThread(LogisthProcessor &p)
            : juce::Thread("logisth preparation"), owner(p) {}
        void run() override {
            while (!threadShouldExit()) {
                owner.serviceAssets();
                wait(100);
            }
        }
    };
    PreparationThread preparation{*this};
    float masterSmooth = 0;
    double bpm = 120;
    bool wasPlaying = false, clockAvailable = false;
    double hostPpq = 0;
    bool hostPpqValid = false;
    juce::int64 expectedSample = -1;
    double expectedPpq = std::numeric_limits<double>::quiet_NaN();
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(LogisthProcessor)
};
