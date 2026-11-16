#include "Dynamics.h"
#include <filesystem>
#include <fstream>
#include <iostream>
using namespace logisth;
int main(int argc, char **argv) {
    const auto folder = std::filesystem::path(argc > 1 ? argv[1] : ".");
    std::filesystem::create_directories(folder);
    std::ofstream comp(folder / "compressor-curves.csv"), clip(folder / "clipper-curves.csv");
    comp << "knee_db,input_db,output_db\n";
    for (float knee : {0.f, 6.f, 12.f, 24.f})
        for (int i = -120; i <= 24; ++i) {
            float input = i * .5f;
            comp << knee << ',' << input << ',' << input - compressionReduction(input, -18, 4, knee)
                 << '\n';
        }
    clip << "soft,knee,input,output\n";
    for (int soft : {0, 1})
        for (float knee : {0.f, .25f, .5f, 1.f})
            for (int i = -200; i <= 200; ++i) {
                double x = i * .01;
                clip << soft << ',' << knee << ',' << x << ',' << clipCurve(x, 1, knee, soft > 0)
                     << '\n';
            }
    std::cout << "Shared DSP/plot transfer curves exported\n";
}
