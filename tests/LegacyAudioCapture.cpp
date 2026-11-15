#include "Engine.h"
#include <filesystem>
#include <fstream>
#include <iostream>
using namespace logisth;
int main(int argc, char **argv) {
    std::filesystem::create_directories(argv[1]);
    for (int fixture = 0; fixture < 20; ++fixture) {
        auto owner = std::make_unique<Engine>();
        auto &e = *owner;
        Params p;
        const int rate = fixture % 3 == 0 ? 44100 : fixture % 3 == 1 ? 48000 : 96000;
        e.prepare(rate);
        for (int i = 0; i < 4; ++i) {
            auto &o = p.osc[size_t(i)];
            o.data = SourceData::factory().get();
            o.stereo =
                0; // 0.9.6.1 deliberately recalibrates stereo; retain mono regression coverage.
            o.level = i < 2 ? .35f : .17f;
            o.unison = fixture % 4 + 1;
            o.detune = fixture % 2 ? 100 : 35;
            o.wave = fixture % 12;
            o.engine = fixture >= 12 ? (fixture - 12) % 4 : 0;
            o.retrigger = fixture % 3 == 0;
            o.loop = fixture % 2;
            o.stretchMode = fixture % 8;
        }
        p.part[1].amp.release = fixture % 2 ? .003f : .19f;
        p.part[0].amp.release = fixture % 2 ? .002f : .13f;
        p.lfo[0].routes[0] = {Destination::PitchAB, .03f};
        p.misc[0] = {Source::Velocity, {{{Destination::CutoffAB, .2f}, {Destination::PanB, .1f}}}};
        std::ofstream out(std::string(argv[1]) + "/" + std::to_string(fixture) + ".bin",
                          std::ios::binary);
        std::array<float, 2 * 256> block{};
        for (int b = 0; b < 40; ++b) {
            if (b == 0 || b == 11 || b == 21)
                e.noteOn(48 + b / 11 * 7, .8f, 1, p);
            if (b == 8 || b == 19 || b == 29)
                e.noteOff(48 + (b - 8) / 11 * 7, 1, p);
            e.render(block.data(), block.data() + 256, 256, p);
            out.write((char *)block.data(), sizeof(block));
            int n = e.activeVoices();
            out.write((char *)&n, sizeof(n));
        }
    }
    std::cout << "20 legacy A/B audio and voice-count fixtures captured\n";
}
