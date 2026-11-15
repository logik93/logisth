#pragma once
#include "Parameters.h"
#include <algorithm>
#include <cmath>
namespace logisth {
inline constexpr std::array<Param, 44> macroTargets{
    Param::Count,        Param::filtercutoff, Param::filterres,    Param::keytrack,
    Param::o1position,   Param::o2position,   Param::o3position,   Param::o4position,
    Param::o1pulsewidth, Param::o2pulsewidth, Param::o3pulsewidth, Param::o4pulsewidth,
    Param::o1fmdepth,    Param::o2fmdepth,    Param::o3fmdepth,    Param::o4fmdepth,
    Param::o1amdepth,    Param::o2amdepth,    Param::o3amdepth,    Param::o4amdepth,
    Param::reverb,       Param::reverbdecay,  Param::delay,        Param::drive,
    Param::eqoutput,     Param::o1stretch,    Param::o2stretch,    Param::o3stretch,
    Param::o4stretch,    Param::lfo1rate,     Param::lfo2rate,     Param::o5position,
    Param::o6position,   Param::o5pulsewidth, Param::o6pulsewidth, Param::o5fmdepth,
    Param::o6fmdepth,    Param::o5amdepth,    Param::o6amdepth,    Param::o5stretch,
    Param::o6stretch,    Param::p3cutoff,     Param::p3res,        Param::mixC};
template <class Reader> float morphedValue(Param id, Reader read) {
    float value = read(size_t(id));
    if (std::find(macroTargets.begin() + 1, macroTargets.end(), id) == macroTargets.end())
        return value;
    for (int macro = 0; macro < 4; ++macro) {
        size_t base = size_t(Param::macro1) + size_t(macro) * 13;
        float amount = std::clamp(read(base), 0.f, 1.f);
        for (int slot = 0; slot < 4; ++slot) {
            size_t route = base + 1 + size_t(slot) * 3;
            int target = std::clamp(int(read(route)), 0, int(macroTargets.size()) - 1);
            if (macroTargets[size_t(target)] != id)
                continue;
            float from = read(route + 1), to = read(route + 2);
            float normalized = std::clamp(from + (to - from) * amount, 0.f, 1.f);
            const auto &spec = parameterSpecs[size_t(id)];
            value = spec.low + (spec.high - spec.low) * std::pow(normalized, 1.f / spec.skew);
            if (spec.integer || *spec.choices)
                value = std::round(value);
        }
    }
    return value;
}
} // namespace logisth
