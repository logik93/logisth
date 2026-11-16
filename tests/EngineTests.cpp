#include "Engine.h"
#include "EnvelopeClickTests.h"
#include "EnvelopePlot.h"
#include <iostream>
#include <stdexcept>
#include <vector>
using namespace logisth;
void require(bool ok, const char *message) {
    if (!ok)
        throw std::runtime_error(message);
}
float energy(const std::vector<float> &a) {
    float e = 0;
    for (auto x : a) {
        require(std::isfinite(x), "non-finite audio");
        e += x * x;
    }
    return e;
}
int main() {
    try {
        envelope_click_test::run();
        auto instant = envelopePlot({0, .2f, .7f, .3f});
        require(instant[0].x == instant[1].x && instant[0].level == 0 && instant[1].level == 1,
                "zero-ms ADSR attack is vertical with no artificial diagonal segment");
        auto slow = envelopePlot({8, .2f, .7f, .3f});
        require(slow[1].x > .8f && slow[1].x > instant[1].x,
                "long ADSR attack occupies proportionally more time");
        auto zeroDecayRelease = envelopePlot({0, 0, .5f, 0});
        require(zeroDecayRelease[1].x == zeroDecayRelease[2].x &&
                    zeroDecayRelease[3].x == zeroDecayRelease[4].x,
                "zero decay/release draw vertical transitions at their true times");
        for (double sr : {44100., 48000., 96000.}) {
            Engine engine;
            engine.prepare(sr);
            Params p;
            std::vector<float> l(512), r(512);
            engine.render(l.data(), r.data(), 512, p);
            require(energy(l) == 0, "silence before notes");
            engine.noteOn(69, 1, 1, p);
            engine.render(l.data(), r.data(), 512, p);
            require(energy(l) > .0001f, "note must sound");
            engine.controller(1, 64, 127, p);
            engine.noteOff(69, 1, p);
            for (int i = 0; i < 100; ++i)
                engine.render(l.data(), r.data(), 512, p);
            require(engine.activeVoices() == 1, "sustain pedal");
            engine.controller(1, 64, 0, p);
            for (int i = 0; i < 250; ++i)
                engine.render(l.data(), r.data(), 512, p);
            require(engine.activeVoices() == 0, "release ends voice");
            require(energy(l) < 1e-12f, "release reaches silence");
            for (int n = 0; n < 40; ++n)
                engine.noteOn(40 + n, .8f, 1, p);
            require(engine.activeVoices() == 16, "bounded voice stealing");
            for (auto &o : p.osc) {
                o.unison = 8;
                o.level = 1;
            }
            for (int wave = 0; wave < 8; ++wave) {
                for (auto &o : p.osc)
                    o.wave = wave;
                for (auto &part : p.part) {
                    part.resonance = 1;
                    part.cutoff = 20000;
                }
                engine.pitchWheel(1, 1);
                engine.render(l.data(), r.data(), 512, p);
                require(std::isfinite(energy(l)), "stress finite");
            }
            engine.controller(1, 120, 0, p);
            engine.render(l.data(), r.data(), 512, p);
            require(energy(l) == 0, "all sound off");
            engine.reset();
            p = Params{};
            p.polyphony = maxPolyphony;
            for (int n = 0; n < maxPolyphony; ++n)
                engine.noteOn(40 + n, .1f, 1, p);
            require(engine.activeVoices() == 32, "all 32 note voices are independently allocated");
            engine.noteOn(90, .1f, 1, p);
            engine.render(l.data(), r.data(), 512, p);
            require(engine.activeVoices() == 32 && std::isfinite(energy(l)),
                    "voice stealing and rendering respect the extended 32-voice limit");
            p.polyphony = 16;
            engine.render(l.data(), r.data(), 512, p);
            require(engine.activeVoices() == 16, "reducing polyphony retires excess voices");
            for (int mode = 0; mode < 4; ++mode) {
                engine.reset();
                p = Params{};
                p.polyphony = maxPolyphony;
                for (auto &osc : p.osc) {
                    osc.level = 0;
                    osc.unison = 0;
                }
                auto &source = p.osc[0];
                source.level = .1f;
                source.unison = 1;
                source.engine = mode;
                source.data = SourceData::factory().get();
                source.gran.enabled = true;
                p.part[0].filterType = 3;
                p.part[0].amp = {0, 0, 1, .001f};
                for (int note = 0; note < maxPolyphony; ++note)
                    engine.noteOn(40 + note, .1f, 1, p);
                engine.render(l.data(), r.data(), 512, p);
                require(engine.activeVoices() == 32 && energy(l) > 0 && std::isfinite(energy(l)),
                        "all four source engines render all 32 note voices");
            }
            // A sine at MIDI 69 must cross zero approximately 440 times/second.
            engine.reset();
            p = Params{};
            p.osc[0].wave = 0;
            p.osc[0].level = 1;
            p.osc[1].level = 0;
            p.part[0].cutoff = 20000;
            engine.noteOn(69, 1, 1, p);
            std::vector<float> a(static_cast<size_t>(sr)), b(static_cast<size_t>(sr));
            engine.render(a.data(), b.data(), int(sr), p);
            int crossings = 0;
            for (size_t i = 1; i < a.size(); ++i)
                if (a[i - 1] < 0 && a[i] >= 0)
                    ++crossings;
            require(std::abs(crossings - 440) <= 2, "A4 tuning");
            std::cout << "PASS sample rate " << sr
                      << ": MIDI, sustain, release, polyphony, waveforms, tuning\n";
        }
        return 0;
    } catch (const std::exception &e) {
        std::cerr << e.what() << '\n';
        return 1;
    }
}
