#pragma once
#include "MultibandDistortion.h"
#include <filesystem>
#include <fstream>
#include <iomanip>
namespace multiband_test {
inline void run(const std::string &folder = {}) {
    std::ofstream reconstruction, isolation, summary;
    if (!folder.empty()) {
        std::filesystem::create_directories(folder);
        reconstruction.open(std::filesystem::path(folder) / "reconstruction.csv");
        isolation.open(std::filesystem::path(folder) / "isolation.csv");
        summary.open(std::filesystem::path(folder) / "summary.csv");
        reconstruction << std::setprecision(12)
                       << "sample_rate,bands,frequency_hz,magnitude_error_db\n";
        isolation << std::setprecision(12)
                  << "sample_rate,input_frequency_hz,selected_band,third_harmonic_amplitude\n";
        summary << std::setprecision(12)
                << "maximum_reconstruction_error_db,dc_automation_error,maximum_transition_jump,"
                   "legacy_exact\n";
    }
    using namespace logisth;
    auto require = [](bool ok, const char *message) {
        if (!ok)
            throw std::runtime_error(message);
    };
    double maximumError = 0;
    for (double rate : {44100., 48000., 96000.})
        for (int count = 1; count <= 4; ++count)
            for (double wanted :
                 {30., 90., 200., 500., 1200., 2000., 4000., 8000., 15000., 19000.}) {
                constexpr int samples = 32768;
                const double frequency = std::round(wanted * samples / rate) * rate / samples;
                DistortionCrossover split;
                split.prepare(rate);
                split.configure(count, {200, 2000, 8000});
                double input = 0, output = 0;
                for (int i = -8192; i < samples; ++i) {
                    const auto x = float(.25 * std::sin(2 * pi * frequency * i / rate));
                    const auto b = split.tick(x, 0);
                    float sum = 0;
                    for (int band = 0; band < count; ++band) {
                        sum += b[0][size_t(band)];
                        require(b[1][size_t(band)] == 0, "crossover never leaks left into right");
                    }
                    if (i >= 0) {
                        input += double(x) * x;
                        output += double(sum) * sum;
                    }
                }
                const auto error = std::abs(10 * std::log10(output / input));
                maximumError = std::max(maximumError, error);
                if (reconstruction.is_open())
                    reconstruction << rate << "," << count << "," << frequency << "," << error
                                   << "\n";
                require(error < .005, "phase-aligned LR24 runtime sum is flat within 0.005 dB");
            }
    // Steady DC remains steady while cutoff coefficients move (TPT state variables).
    DistortionCrossover dc;
    dc.prepare(48000);
    dc.configure(4, {200, 2000, 8000});
    for (int i = 0; i < 12000; ++i)
        dc.tick(.25f, -.25f);
    double dcError = 0;
    for (int block = 0; block < 100; ++block) {
        dc.configure(4, block % 2 == 0 ? std::array<float, 3>{20, 21, 22}
                                       : std::array<float, 3>{15000, 19000, 20000});
        for (int i = 0; i < 512; ++i) {
            const auto b = dc.tick(.25f, -.25f);
            float l = 0, r = 0;
            for (int band = 0; band < 4; ++band) {
                l += b[0][size_t(band)];
                r += b[1][size_t(band)];
            }
            dcError = std::max(dcError, double(std::abs(l - .25f)));
            require(std::isfinite(l) && std::abs(l + r) < 1e-6 && std::abs(l) < .3f,
                    "moving crossovers preserve stereo polarity, DC and numerical stability");
        }
    }
    require(dcError < 2e-5, "cutoff automation does not generate DC clicks");
    for (int model = 0; model < 5; ++model) {
        DistortionRack legacy;
        legacy.prepare(48000);
        MultibandDistortionRack router;
        router.prepare(48000);
        std::array<DistortionSettings, 4> p{{{true, model, .32f, .83f},
                                             {true, (model + 1) % 5, .17f, .54f},
                                             {false, 0, 0, 1},
                                             {true, 2, .1f, .3f}}};
        DistortionOrder order;
        order.move(0, 3);
        for (int block = 0; block < 32; ++block) {
            std::array<float, 257> a{}, b{}, c{}, d{}, mod{};
            for (int i = 0; i < 257; ++i) {
                a[size_t(i)] = c[size_t(i)] = .3f * std::sin(float(block * 257 + i) * .071f);
                b[size_t(i)] = d[size_t(i)] = -.7f * a[size_t(i)];
                mod[size_t(i)] = .08f * std::sin(float(i) * .03f);
            }
            legacy.process(a.data(), b.data(), 257, p, order, mod.data(), .002f);
            router.process(c.data(), d.data(), 257, p, order, mod.data(), .002f, false, 4,
                           {200, 2000, 8000});
            require(a == c && b == d,
                    "Legacy is sample-identical to the unchanged serial rack for every model");
        }
    }
    // Nonlinearity applied to the low band creates low-tone harmonics, not high-tone harmonics.
    for (double wanted : {100., 6000.}) {
        constexpr int count = 32768;
        const double rate = 48000, hz = std::round(wanted * count / rate) * rate / count;
        MultibandDistortionRack router;
        router.prepare(rate);
        std::array<DistortionSettings, 4> p{{{true, 2, .8f, 1}, {}, {}, {}}};
        double real = 0, imag = 0, energy = 0;
        for (int at = -8192; at < count; ++at) {
            float l = float(.4 * std::sin(2 * pi * hz * at / rate)), r = 0;
            router.process(&l, &r, 1, p, {}, nullptr, float(1 - std::exp(-1 / 480.)), true, 4,
                           {200, 2000, 8000});
            require(r == 0 && std::isfinite(l),
                    "multiband processing keeps stereo channels independent");
            if (at >= 0) {
                real += l * std::cos(2 * pi * 3 * hz * at / rate);
                imag -= l * std::sin(2 * pi * 3 * hz * at / rate);
                energy += l * l;
            }
        }
        const auto harmonic = 2 * std::hypot(real, imag) / count;
        if (isolation.is_open())
            isolation << rate << "," << hz << ",0," << harmonic << "\n";
        require(wanted < 200 ? harmonic > .02 : harmonic < .00001,
                "distortion harmonic generation follows the selected frequency band");
    }
    // Changes in topology/model/order happen under an output fade, not a discontinuity.
    MultibandDistortionRack moving;
    moving.prepare(48000);
    std::array<DistortionSettings, 4> p{
        {{true, 0, .3f, .8f}, {true, 0, .4f, .5f}, {true, 2, .2f, .7f}, {true, 4, .2f, .9f}}};
    float previous = .25f, largest = 0;
    for (int block = 0; block < 80; ++block) {
        const bool multi = block >= 10 && block < 70;
        const int bands = block < 30 ? 4 : block < 50 ? 2 : 3;
        DistortionOrder order;
        if (block >= 40)
            order.move(0, 2);
        for (int i = 0; i < 256; ++i) {
            float l = .25f, r = -.25f;
            moving.process(&l, &r, 1, p, order, nullptr, .002f, multi, bands, {200, 2000, 8000});
            if (block > 2)
                largest = std::max(largest, std::abs(l - previous));
            require(std::isfinite(l) && std::abs(l + r) < 1e-6,
                    "topology changes keep stereo and finite output");
            previous = l;
        }
    }
    require(largest < .08f, "routing, band count and order changes are click-smoothed");
    if (summary.is_open())
        summary << maximumError << "," << dcError << "," << largest << ",1\n";
    std::cout << "PASS multiband: 120 runtime reconstruction cases, max " << maximumError
              << " dB; DC automation " << dcError
              << "; exact Legacy, band isolation/stereo and transitions " << largest << '\n';
}
} // namespace multiband_test
