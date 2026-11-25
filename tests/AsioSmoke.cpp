#include <juce_audio_devices/juce_audio_devices.h>
#include <iostream>
#include <memory>

int main() {
    static_assert(JUCE_ASIO, "Standalone ASIO support must be enabled for this test");
    std::unique_ptr<juce::AudioIODeviceType> asio(
        juce::AudioIODeviceType::createAudioIODeviceType_ASIO());
    if (asio == nullptr || asio->getTypeName() != "ASIO") {
        std::cerr << "JUCE could not create the Windows ASIO device type\n";
        return 1;
    }
    std::cout << "PASS Windows ASIO backend is available to the device manager\n";
}
