#include "ThemeSettings.h"
#include "BinaryData.h"
namespace logisth {
std::vector<ThemeDefinition> ThemeSettings::bundledThemes() {
    // Read the exact supplied JSON, rather than maintaining a second colour table.
    static const auto themes = [] {
        std::vector<ThemeDefinition> result;
        struct Resource {
            const char *key, *data;
            int size;
        };
        const Resource files[]{{"bokkenmmb-Solarized", BinaryData::bokkenmmbSolarized_json,
                                BinaryData::bokkenmmbSolarized_jsonSize},
                               {"bokkenmmb-Tokyo Night", BinaryData::bokkenmmbTokyo_Night_json,
                                BinaryData::bokkenmmbTokyo_Night_jsonSize},
                               {"bokkenmmb-Solarized Osaka",
                                BinaryData::bokkenmmbSolarized_Osaka_json,
                                BinaryData::bokkenmmbSolarized_Osaka_jsonSize},
                               {"bokkenmmb-gruvbox", BinaryData::bokkenmmbgruvbox_json,
                                BinaryData::bokkenmmbgruvbox_jsonSize}};
        for (const auto &file : files) {
            ThemeDefinition theme;
            const bool valid =
                decode(juce::JSON::parse(juce::String::fromUTF8(file.data, file.size)), theme);
            jassert(valid);
            if (valid) {
                theme.key = file.key;
                theme.factory = true;
                result.push_back(std::move(theme));
            }
        }
        return result;
    }();
    return themes;
}
} // namespace logisth
