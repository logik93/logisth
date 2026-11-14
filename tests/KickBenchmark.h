#pragma once
#include "PluginProcessor.h"
#include <iostream>
namespace kick_benchmark {
inline void write(const juce::File &file, const juce::AudioBuffer<float> &audio) {
    auto output = file.createOutputStream();
    if (!output)
        throw std::runtime_error("kick benchmark output");
    juce::WavAudioFormat format;
    std::unique_ptr<juce::AudioFormatWriter> writer(
        format.createWriterFor(output.release(), 48000, 2, 32, {}, 0));
    if (!writer || !writer->writeFromAudioSampleBuffer(audio, 0, audio.getNumSamples()))
        throw std::runtime_error("kick benchmark WAV");
}
inline void run(const juce::File &folder) {
    using namespace logisth;
    folder.createDirectory();
    SourceData source;
    source.sampleRate = 48000;
    source.name = "Synthetic clipped kick / 52 Hz tail";
    int length = 38400;
    juce::AudioBuffer<float> reference(2, length);
    double phase = 0;
    uint32_t random = 1234567;
    for (auto &channel : source.samples)
        channel.resize(size_t(length));
    for (int i = 0; i < length; ++i) {
        double t = i / 48000.;
        phase += (52 + 190 * std::exp(-t * 42)) / 48000.;
        random ^= random << 13;
        random ^= random >> 17;
        random ^= random << 5;
        double body =
            (std::sin(2 * pi * phase) + .2 * std::sin(6 * pi * phase)) * std::exp(-t * 3.5);
        double click = (double(random) / 2147483648. - 1) * std::exp(-t * 900) * .3;
        float sample = float((std::tanh(body * 3) * .65 + click) * std::min(1., t / .0003));
        for (int ch = 0; ch < 2; ++ch) {
            source.samples[size_t(ch)][size_t(i)] = sample;
            reference.setSample(ch, i, sample);
        }
    }
    write(folder.getChildFile("kick-reference.wav"), reference);
    juce::String csv = "semitones,duration_seconds,peak,attack_max_error\n";
    for (int semitones : {-12, -7, 0, 7, 12}) {
        SpectralStream stream(48000);
        SamplerPlayer player;
        player.spectral = &stream;
        player.token = uint64_t(semitones + 13);
        juce::AudioBuffer<float> audio(2, length);
        double peak = 0, error = 0;
        for (int i = 0; i < length; ++i) {
            auto value =
                player.tick(source, std::exp2(semitones / 12.), 48000, 0, 1, false, false, 6, 1, 2);
            for (int ch = 0; ch < 2; ++ch) {
                audio.setSample(ch, i, value[size_t(ch)]);
                peak = std::max(peak, double(std::abs(value[size_t(ch)])));
            }
            if (i < 1200)
                error = std::max(error, double(std::abs(value[0] - reference.getSample(0, i))));
        }
        if (error > 1e-6 || !std::isfinite(peak) || peak > 3)
            throw std::runtime_error("kick benchmark attack/level");
        write(folder.getChildFile("kick-" + juce::String(semitones) + "st.wav"), audio);
        csv += juce::String(semitones) + ",0.8," + juce::String(peak, 8) + "," +
               juce::String(error, 10) + "\n";
    }
    folder.getChildFile("measurements.csv").replaceWithText(csv);
    std::cout << "PASS kick benchmark: protected 25 ms attack at -12/-7/0/+7/+12 semitones; WAV "
                 "comparisons exported\n";
}
} // namespace kick_benchmark
