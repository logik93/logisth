#include "Sampler.h"
#include <iostream>
#include <stdexcept>
#include <vector>
using namespace logisth;
struct Material {
    std::array<std::vector<float>, 2> samples;
    double sampleRate = 48000;
    uint64_t revision = 17;
};
static void require(bool ok, const char *message) {
    if (!ok)
        throw std::runtime_error(message);
}
int main() {
    try {
        Material data;
        for (auto &c : data.samples)
            c.assign(1001, .1f);
        data.sampleRate = 1000;
        data.samples[0][900] = .73f;
        TailLoopSettings tail{true, .3f, .6f, 0};
        SamplerPlayer p;
        for (int i = 0; i < 5000; ++i) {
            auto y = p.tick(data, 1, 1000, 0, 1, false, false, 0, 1, 0, .5f, 0, 25, {}, tail);
            if (i == 900)
                require(y[0] == .73f,
                        "first pass reaches the complete selection end before looping");
            if (i >= 1000) {
                require(y[0] == .1f, "tail does not replay the attack or post-loop region");
                require(p.displayCursor >= 299.99 && p.displayCursor < 600.01,
                        "cursor remains inside the tail region");
            }
        }
        for (int reverse = 0; reverse < 2; ++reverse)
            for (int quality = 0; quality < 4; ++quality) {
                SamplerPlayer v;
                for (int i = 0; i < 4000; ++i) {
                    auto y = v.tick(data, 1, 1000, 0, 1, false, reverse, 0, 1, quality, .5f, 0, 25,
                                    {}, tail);
                    require(std::isfinite(y[0]), "tail interpolation is finite");
                    if (i > 2000)
                        require(std::abs(y[0] - .1f) < 1e-4,
                                "reverse and interpolation honour independent loop points");
                }
            }
        Material wave;
        for (auto &c : wave.samples)
            c.resize(9601);
        for (int i = 0; i < 9601; ++i) {
            float x = i < 480                 ? .9f
                      : i >= 3840 && i < 7680 ? .25f * std::sin(float(2 * pi * i / 48))
                                              : .03f;
            wave.samples[0][i] = x;
            wave.samples[1][i] = x * .5f;
        }
        for (int mode = 0; mode < 8; ++mode) {
            SamplerPlayer player;
            std::unique_ptr<SpectralStream> stream;
            if (mode >= 6) {
                stream = std::make_unique<SpectralStream>(48000);
                player.spectral = stream.get();
                player.token = 99;
            }
            TailLoopSettings loop{true, .4f, .8f, 5};
            double energy = 0, energyR = 0, peak = 0;
            for (int i = 0; i < 64000; ++i) {
                auto y = player.tick(wave, 1, 48000, 0, 1, false, false, mode, 1, 1, .5f, 0, 25, {},
                                     loop);
                require(std::isfinite(y[0]) && std::isfinite(y[1]),
                        "all stretch modes remain finite in tail playback");
                if (i > 40000) {
                    energy += y[0] * y[0];
                    energyR += y[1] * y[1];
                    peak = std::max(peak, double(std::abs(y[0])));
                    require(player.displayCursor >= 3839 && player.displayCursor < 7681,
                            "all backends publish the physical tail cursor");
                }
            }
            double rms = std::sqrt(energy / 23999);
            if (!(rms > .04 && rms < .3 && peak < .55))
                std::cerr << "mode " << mode << " rms " << rms << " peak " << peak << '\n';
            require(rms > .04 && rms < .3 && peak < .55,
                    "all eight modes sustain the tail without replaying the attack");
            require(std::abs(std::sqrt(energyR / energy) - .5) < .03,
                    "tail processing preserves stereo channels");
        }
        // Overlapping linear fades remove hard discontinuities in a deliberately mismatched loop.
        for (auto &c : data.samples)
            for (int i = 0; i < 1001; ++i)
                c[size_t(i)] = i < 450 ? .5f : -.5f;
        double jumps[2]{};
        for (int fade = 0; fade < 2; ++fade) {
            SamplerPlayer v;
            TailLoopSettings loop{true, .3f, .6f, fade ? 10.f : 0.f};
            float previous = 0;
            double previousCursor = 0;
            for (int i = 0; i < 3500; ++i) {
                float y =
                    v.tick(data, 1, 1000, 0, 1, false, false, 0, 1, 0, .5f, 0, 25, {}, loop)[0];
                if (i > 1001 && (v.displayCursor > 580 || previousCursor > 580))
                    jumps[fade] = std::max(jumps[fade], double(std::abs(y - previous)));
                previous = y;
                previousCursor = v.displayCursor;
            }
        }
        require(jumps[1] < .12 && jumps[0] > .9, "loop crossfade smooths the wrap edge");
        auto tiny = TailLoopBounds::make(2, 1, 0, true, 48000, {true, 1, 0, 200});
        for (int i = 0; i < 20; ++i)
            require(std::isfinite(tiny.mapped(i)), "short/inverted ranges stay bounded and finite");
        std::cout << "PASS tail loop: complete intro once, independent points, reverse, four "
                     "interpolators, eight backends, stereo, cursor and crossfade\n";
    } catch (const std::exception &e) {
        std::cerr << e.what() << '\n';
        return 1;
    }
}
