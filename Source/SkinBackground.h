#pragma once
#include <cstring>
#include <juce_gui_basics/juce_gui_basics.h>
namespace logisth {
// Portable, bounded PNG/JPEG data. No image access occurs on the audio thread.
class SkinBackground {
    static bool dimensions(const juce::MemoryBlock &block) {
        const auto *p = static_cast<const uint8_t *>(block.getData());
        const auto n = block.getSize();
        auto bounded = [](unsigned w, unsigned h) {
            return w > 0 && h > 0 && w <= 4096 && h <= 4096;
        };
        auto big = [&](size_t at) {
            return (unsigned(p[at]) << 24) | (unsigned(p[at + 1]) << 16) |
                   (unsigned(p[at + 2]) << 8) | p[at + 3];
        };
        const uint8_t png[]{137, 80, 78, 71, 13, 10, 26, 10};
        if (n >= 24 && std::memcmp(p, png, 8) == 0 && std::memcmp(p + 12, "IHDR", 4) == 0)
            return bounded(big(16), big(20));
        if (n < 4 || p[0] != 255 || p[1] != 216)
            return false;
        for (size_t at = 2; at + 4 <= n;) {
            if (p[at++] != 255)
                return false;
            while (at < n && p[at] == 255)
                ++at;
            if (at >= n)
                return false;
            const auto marker = p[at++];
            if (marker == 217 || marker == 218)
                return false;
            if (marker == 1 || (marker >= 208 && marker <= 216))
                continue;
            if (at + 2 > n)
                return false;
            const auto size = (unsigned(p[at]) << 8) | p[at + 1];
            if (size < 2 || at + size > n)
                return false;
            if (marker == 192 || marker == 193 || marker == 194)
                return size >= 8 && p[at + 2] == 8 &&
                       bounded((unsigned(p[at + 5]) << 8) | p[at + 6],
                               (unsigned(p[at + 3]) << 8) | p[at + 4]);
            at += size;
        }
        return false;
    }
    static bool bytes(const juce::String &text, juce::MemoryBlock &result) {
        if (text.isEmpty() || text.length() > maxEncoded)
            return false;
        juce::MemoryOutputStream stream(result, false);
        if (!juce::Base64::convertFromBase64(stream, text))
            return false;
        stream.flush();
        return result.getSize() <= maxBytes && dimensions(result);
    }

  public:
    static constexpr size_t maxBytes = 16 * 1024 * 1024;
    static constexpr int maxEncoded = int((maxBytes + 2) / 3 * 4);
    static bool validEncoding(const juce::String &text) {
        juce::MemoryBlock block;
        return text.isEmpty() || bytes(text, block);
    }
    static juce::Image decode(const juce::String &text) {
        juce::MemoryBlock block;
        return bytes(text, block)
                   ? juce::ImageFileFormat::loadFrom(block.getData(), block.getSize())
                   : juce::Image{};
    }
    static juce::Result import(const juce::File &file, juce::String &encoded, juce::Image &image) {
        juce::MemoryBlock block;
        if (!file.existsAsFile() || file.getSize() <= 0 || file.getSize() > juce::int64(maxBytes) ||
            !file.loadFileAsData(block) || !dimensions(block))
            return juce::Result::fail("Choose a PNG/JPEG up to 4096 x 4096 pixels and 16 MiB.");
        auto next = juce::ImageFileFormat::loadFrom(block.getData(), block.getSize());
        if (!next.isValid())
            return juce::Result::fail("The background image could not be decoded.");
        encoded = juce::Base64::toBase64(block.getData(), block.getSize());
        image = next;
        return juce::Result::ok();
    }
};
} // namespace logisth
