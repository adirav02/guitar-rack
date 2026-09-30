#pragma once
#include "core/ToneState.h"
#include <filesystem>
#include <juce_core/juce_core.h>

namespace guitarrack {
inline constexpr std::size_t maxPresetBytes = 128 * 1024;
// All APIs below are off audio. Version 1 remains supported unchanged; version 2 adds IR references.
juce::var parseJson(const std::string&); // strict syntax, UTF-8, duplicate keys, resource limits
ToneState migratePreset(const juce::var&, const EffectRegistry&);
ToneState parsePreset(const std::string&, const EffectRegistry&);
std::string serializePreset(const ToneState&, const EffectRegistry&);
std::string readBounded(const std::filesystem::path&);
void atomicWrite(const std::filesystem::path&, const std::string&);
std::filesystem::path userDataDirectory();
std::filesystem::path userConfigDirectory();
std::vector<ToneState> factoryPresets(const EffectRegistry&);
// A library entry's identity is its UUID; names never become file paths.
class PresetStore {
public:
    explicit PresetStore(std::filesystem::path root) : directory(std::move(root)) {}
    void save(const ToneState&, const EffectRegistry&) const;
    // Removes only a validated UUID entry in this store. Updates matching local
    // recall before unlink; failed operations restore recall/file where possible.
    void erase(const std::string& id, const EffectRegistry&, const std::filesystem::path& recallFile,
               const ToneState* saveFirst = nullptr) const;
    std::vector<ToneState> list(const EffectRegistry&, std::vector<std::string>& errors) const;
    std::filesystem::path directory;
};
}
