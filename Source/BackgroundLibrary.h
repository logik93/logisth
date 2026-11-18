#pragma once
#include <array>
#include <juce_graphics/juce_graphics.h>
namespace logisth {
// Original, compressed artwork; decoded only when selected on the message thread.
struct BackgroundLibrary {
    struct Entry {
        const char *theme, *name, *filename;
    };
    inline static constexpr std::array<Entry, 20> entries{{
        {"00-light", "Paper Circuit", "00-light-paper.png"},
        {"00-light", "Sage Mist", "00-light-mist.png"},
        {"01-midnight-blue", "Blue Current", "01-midnight-blue-current.png"},
        {"01-midnight-blue", "Orbit", "01-midnight-blue-orbit.png"},
        {"02-cobalt", "Tide", "02-cobalt-tide.png"},
        {"02-cobalt", "Prism", "02-cobalt-prism.png"},
        {"03-graphite", "Carbon", "03-graphite-carbon.png"},
        {"03-graphite", "Contour", "03-graphite-contour.png"},
        {"04-forest", "Canopy", "04-forest-canopy.png"},
        {"04-forest", "Topography", "04-forest-topography.png"},
        {"05-ap-always-flexin", "Spectrum", "05-ap-always-flexin-spectrum.png"},
        {"05-ap-always-flexin", "Pulse", "05-ap-always-flexin-pulse.png"},
        {"bokkenmmb-Solarized", "Dunes", "bokkenmmb-Solarized-sand.png"},
        {"bokkenmmb-Solarized", "Signal", "bokkenmmb-Solarized-signal.png"},
        {"bokkenmmb-Tokyo Night", "City", "bokkenmmb-Tokyo-Night-city.png"},
        {"bokkenmmb-Tokyo Night", "Origami", "bokkenmmb-Tokyo-Night-origami.png"},
        {"bokkenmmb-Solarized Osaka", "Depth", "bokkenmmb-Solarized-Osaka-depth.png"},
        {"bokkenmmb-Solarized Osaka", "Circuit", "bokkenmmb-Solarized-Osaka-circuit.png"},
        {"bokkenmmb-gruvbox", "Ember", "bokkenmmb-gruvbox-ember.png"},
        {"bokkenmmb-gruvbox", "Linen", "bokkenmmb-gruvbox-linen.png"},
    }};
    static juce::MemoryBlock data(int index);
};
} // namespace logisth
