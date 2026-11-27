#include "Engine.h"
#include <iostream>
#include <stdexcept>
using namespace logisth;
void check(bool value, const char *message) {
    if (!value)
        throw std::runtime_error(message);
}
float material(double frame, int channel, double rate) {
    double phase = 2 * pi * 220 * frame / rate;
    return float(.25 * std::sin(phase) + .13 * std::sin(3 * phase + channel * .2) +
                 .07 * std::sin(11 * phase) + .04 * std::sin(27 * phase));
}
void upstreamParity(double rate, SpectralConfig config, float speed, float pitch, float limit,
                    float formant) {
    SpectralStream adapter(rate, config);
    signalsmith::stretch::SignalsmithStretch<float> native{0x4c677374};
    native.configure(2, config.window(rate), config.hop(rate));
    native.setTransposeFactor(pitch, limit / float(rate));
    native.setFormantSemitones(formant);
    native.reset();
    std::array<std::vector<float>, 2> input;
    for (auto &channel : input)
        channel.resize(size_t(std::max(4096, native.blockSamples())));
    float *in[]{input[0].data(), input[1].data()};
    double at = native.outputLatency() * speed - native.inputLatency();
    for (int i = 0; i < native.blockSamples(); ++i, ++at)
        for (int ch = 0; ch < 2; ++ch)
            input[size_t(ch)][size_t(i)] = material(at, ch, rate);
    native.seek(in, native.blockSamples(), speed);
    StretchControls controls;
    controls.blockMs = config.blockMs;
    controls.overlap = config.overlap;
    controls.tonalityHz = limit;
    controls.formant = formant;
    std::array<std::array<float, 64>, 2> reference{};
    float *out[]{reference[0].data(), reference[1].data()};
    for (int block = 0; block < 160; ++block) {
        int count = int(std::floor((block + 1) * 64 * speed)) - int(std::floor(block * 64 * speed));
        for (int i = 0; i < count; ++i, ++at)
            for (int ch = 0; ch < 2; ++ch)
                input[size_t(ch)][size_t(i)] = material(at, ch, rate);
        native.process(in, count, out, 64);
        for (int i = 0; i < 64; ++i) {
            auto actual =
                adapter.tick([&](double frame, int ch) { return material(frame, ch, rate); }, 0, 1,
                             pitch, 1 / speed, 1, 1, false, controls);
            for (int ch = 0; ch < 2; ++ch)
                check(std::abs(actual[size_t(ch)] - reference[size_t(ch)][size_t(i)]) < 1e-6f,
                      "adapter output must agree with the unchanged upstream processor");
        }
    }
}
double power(const std::vector<float> &v, double hz, int from = 6000) {
    double a = 0, b = 0, coefficient = 2 * std::cos(2 * pi * hz / 48000);
    for (size_t i = size_t(from); i < v.size(); ++i) {
        double x = v[i] + coefficient * a - b;
        b = a;
        a = x;
    }
    return std::max(0., a * a + b * b - coefficient * a * b) /
           std::pow(double(v.size() - size_t(from)), 2);
}
static void checkDiffusionAndGrainRate() {
    const double rate = 48000.;
    auto render = [rate](std::array<float, 3> gains) {
        PhaseDiffuseStretch player(4096);
        player.reset(0, 42);
        std::vector<float> output;
        for (int frame = 0; frame < 24000; ++frame) {
            auto x = player.tick([rate](double at, int) {
                return .4f * float(std::sin(2 * pi * 375 * at / rate));
            }, 1., 1.f, 8.f, gains);
            check(std::isfinite(x[0]) && x[0] == x[1] && std::abs(x[0]) < 2.f,
                  "diffusion preserves stereo coherence and finite output");
            output.push_back(x[0]);
        }
        return output;
    };
    const auto unity = render({0, 1, 0}), octave = render({0, 0, 1}), muted = render({0, 0, 0});
    const auto band = [](const std::vector<float> &v, double centre) {
        double energy = 0;
        for (auto hz = centre - 65; hz <= centre + 65; hz += 5)
            energy += power(v, hz);
        return energy;
    };
    check(band(unity, 375) > band(unity, 750) * 10 + 1e-5 &&
              band(octave, 750) > band(octave, 375) * 10 + 1e-5 &&
              *std::max_element(muted.begin(), muted.end()) == 0,
          "ratio mixer moves the occupied band up one octave and can mute all layers");

    SourceData source = *SourceData::factory();
    GranularParams slow, fast;
    slow.timeRate = .5f;
    fast.timeRate = 2.f;
    slow.hold = fast.hold = .002f;
    GranulizerPlayback a, b;
    for (int i = 0; i < 192; ++i) {
        a.tick(source, slow, rate, 1., 0, 1, true, false, 60, 60, 120);
        b.tick(source, fast, rate, 1., 0, 1, true, false, 60, 60, 120);
    }
    check(a.generated == b.generated && b.head > 3.9 * a.head,
          "Granulizer time rate changes traversal independently of grain interval");
}
static void checkParallelStretch() {
    constexpr double rate = 48000;
    StretchControls phase;
    phase.diffuse = true;
    phase.phaseFftOrder = minPhaseFftOrder;
    const auto config = SpectralConfig::from(phase);
    check(config.window(rate) == 256, "FFT control selects the exact transform length");
    phase.blockMs = 120;
    check(config == SpectralConfig::from(phase), "Signalsmith's block does not change the PaulXStretch FFT");
    phase.phaseFftOrder = maxPhaseFftOrder;
    check(SpectralConfig::from(phase).window(rate) == 262144, "largest selectable transform is exact");

    for (float amount : {minPhaseStretch, maxPhaseStretch}) {
        PhaseDiffuseStretch stream(256);
        stream.reset(0, 42);
        int reads = 0;
        double secondFrame = 0;
        for (int i = 0; i <= 64; ++i)
            stream.tick([&](double at, int) {
                if (reads++ == 512)
                    secondFrame = at;
                return .25f;
            }, 1., 1.f, amount, {0, 1, 0});
        check(std::abs(secondFrame - 64. / amount) < .0001, "Stretch Amount changes real FFT traversal over the full range");
    }

    std::array<SpectralConfig, playerCount> phaseConfigs{};
    phaseConfigs[0] = config;
    SpectralBank bank(rate, 1, {1, 1, 1, 1, 1, 1}, {}, {}, 1, 1, phaseConfigs);
    check(bank.get(0, 0, 0, 0) != bank.get(0, 0, 0, 1) &&
              bank.getPhase(0, 0, 0) && bank.getPhase(0, 0, 0)->config == config,
          "Kick, Signalsmith and PaulXStretch have independent FFT streams");
    auto sample = SourceData::factory();
    SourcePlayback tape, grains, parallel;
    double difference = 0;
    for (int i = 0; i < 2048; ++i) {
        const auto next = [&](SourcePlayback &player, unsigned layers) {
            return player.tick(*sample, 2, 1., rate, 0, 1, true, false, .04f, 16, 0, 0, 0,
                               0, 2, 0, .5f, 0, 25, 0, {}, {}, layers);
        };
        const auto a = next(tape, 1u << 1), b = next(grains, 1u << 2);
        const auto sum = next(parallel, (1u << 1) | (1u << 2));
        for (int ch = 0; ch < 2; ++ch) {
            difference += std::abs(a[size_t(ch)] - b[size_t(ch)]);
            check(std::abs(sum[size_t(ch)] - (a[size_t(ch)] + b[size_t(ch)]) * .5f) < 1e-6f,
                  "parallel engines render independently and mix both outputs");
        }
    }
    check(difference > 1, "parallel layers produce measurably different output");
}
int main() {
    try {
        checkDiffusionAndGrainRate();
        checkParallelStretch();
        for (double rate : {44100., 48000., 96000.})
            for (float speed : {.5f, 1.f, 2.f}) {
                upstreamParity(rate, {}, speed, 1.5f, 3000, 7);
                upstreamParity(rate, {48, 4}, speed, .75f, 0, -5);
            }
        SourceData source;
        source.sampleRate = 48000;
        for (auto &channel : source.samples) {
            channel.resize(48000);
            for (int i = 0; i < 48000; ++i)
                channel[size_t(i)] = .4f * float(std::sin(2 * pi * 220 * i / 48000));
        }
        for (int mode = 0; mode < 8; ++mode) {
            StretchControls controls;
            controls.rate = 2;
            controls.semitones = 12;
            SpectralStream stream(48000);
            SamplerPlayer player;
            player.spectral = &stream;
            std::vector<float> output;
            for (int i = 0; i < 60000 && !player.finished; ++i) {
                auto x = player.tick(source, 1, 48000, 0, 1, false, false, mode, 1, 1, .5f, 0, 25,
                                     controls);
                check(std::isfinite(x[0]) && std::abs(x[0]) < 2,
                      "new controls produce bounded audio");
                output.push_back(x[0]);
            }
            int expected = mode < 2 ? 12000 : mode == 6 ? 24600 : 24000;
            check(std::abs(int(output.size()) - expected) < 4,
                  "Rate changes duration in the intended algorithm");
            double frequency = mode < 2 ? 880 : 440;
            check(power(output, frequency) > power(output, 220) * 10 + 1e-7,
                  "Semitones changes note pitch while independent modes retain Rate separation");
        }
        for (int overlap : {2, 4, 8, 16}) {
            StretchControls controls;
            controls.blockMs = 35;
            controls.overlap = overlap;
            SamplerPlayer player;
            double energy = 0;
            for (int i = 0; i < 24000; ++i) {
                float x = player.tick(source, 1, 48000, 0, 1, true, false, 2, 1, 1, .5f, 0, 25,
                                      controls)[0];
                if (i > 6000)
                    energy += x * x;
            }
            double rms = std::sqrt(energy / 17999);
            check(rms > .25 && rms < .31, "Overlap changes retain unity window gain");
        }
        std::array<int, 4> unison{8, 8, 8, 8};
        std::array<SpectralConfig, 4> settings;
        settings.fill({200, 16});
        bool refused = false;
        try {
            SpectralBank excessive(96000, 15, unison, {}, settings);
        } catch (const std::length_error &) {
            refused = true;
        }
        check(refused, "oversized spectral requests are rejected before allocating their pools");
        SpectralBank small(48000, 1, {1, 1, 1, 1}, {}, {}, 1);
        check(small.get(0, 0, 0, 0) && !small.get(1, 0, 0, 0) && !small.get(0, 1, 0, 0),
              "voice limits and inactive source pools are respected");
        std::array<int, playerCount> extendedUnison{1, 0, 0, 0, 0, 0};
        std::array<SpectralConfig, playerCount> compactConfig{};
        compactConfig[0] = {20, 4};
        auto extended = std::make_unique<SpectralBank>(48000, 1, extendedUnison, nullptr,
                                                       compactConfig, maxPolyphony);
        check(extended->get(31, 0, 0, 0) && extended->get(31, 0, 0, 1) &&
                  !extended->get(32, 0, 0, 0),
              "spectral preparation serves voice 32 in both lanes and retains its bounds");
        std::cout << "PASS Signalsmith 1.3.2 parity: stereo, 3 sample rates, Rate, Semitones, "
                     "Tonality, Formant, Block, Overlap and memory bounds\n";
    } catch (const std::exception &error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
