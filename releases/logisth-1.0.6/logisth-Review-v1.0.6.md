# logisth 1.0.6 — Windows Standalone ASIO

The Windows Standalone executable and ASIO smoke test share the same CMake
target settings for JUCE's ASIO device backend. The smoke test checks that
JUCE creates an audio device type named ASIO. Windows native builds require
this test before packaging. Playback through a particular interface requires
that interface's installed ASIO driver and hardware.

The source archive carries the three Steinberg interface headers used by
JUCE and their license. The upstream revision and exact blob hashes are
recorded in [the SDK provenance](../Source/vendor/steinberg-asio/UPSTREAM.md).

[Master Pitch and wheel automation measurements](https://github.com/logik93/logisth/blob/v1.0.5/docs/REVIEW-v1.0.5.md)
