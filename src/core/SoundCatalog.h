#pragma once
#include <string>
#include <vector>
namespace guitarrack {
struct SoundMetadata {
    std::string id, artist, style, gain, character, tuning, pickups, input, amp, cabinet, notes, evidence, listening;
    std::vector<std::string> tags;
};
const std::vector<SoundMetadata>& soundCatalog(); // Message/offline thread only.
const SoundMetadata* soundMetadata(const std::string&);
}
