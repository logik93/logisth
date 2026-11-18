#include "BackgroundLibrary.h"
#include "BackgroundData.h"
namespace logisth {
juce::MemoryBlock BackgroundLibrary::data(int index) {
    if (index < 0 || index >= int(entries.size()))
        return {};
    for (int resource = 0; resource < LogisthBackgrounds::namedResourceListSize; ++resource) {
        if (juce::String(LogisthBackgrounds::originalFilenames[resource]) ==
            entries[size_t(index)].filename) {
            int bytes = 0;
            auto *source = LogisthBackgrounds::getNamedResource(
                LogisthBackgrounds::namedResourceList[resource], bytes);
            if (source && bytes > 0)
                return {source, size_t(bytes)};
        }
    }
    return {};
}
} // namespace logisth
