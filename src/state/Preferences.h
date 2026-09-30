#pragma once
#include "Presets.h"
namespace guitarrack {
struct Preferences {
    std::string backend, inputDevice, outputDevice;
    int input=-1, left=-1, right=-1, buffer=256, width=920, height=1080;
    double rate=48000;
    bool operator==(const Preferences&) const = default;
};
int exactDeviceIndex(const juce::StringArray&, const std::string& rememberedName);
std::string serializePreferences(const Preferences&);
Preferences parsePreferences(const std::string&);
}
