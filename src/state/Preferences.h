#pragma once
#include "Presets.h"
namespace guitarrack {
struct Preferences {
    std::string backend, inputDevice, outputDevice;
    int input=-1, left=-1, right=-1, buffer=256, width=1280, height=720;
    double rate=48000;
    bool audioCollapsed=true;
    int browserX=100, browserY=80, browserWidth=1000, browserHeight=640, boardScroll=0, latencyProfile=128;
    std::string search, source="All", artist="All", category="All";
    std::vector<std::string> favorites, expanded;

    bool operator==(const Preferences&) const = default;
};
int exactDeviceIndex(const juce::StringArray&, const std::string& rememberedName);
std::string serializePreferences(const Preferences&);
Preferences parsePreferences(const std::string&);
}
