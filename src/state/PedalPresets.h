#pragma once
#include "Presets.h"
#include "core/SoundLibrary.h"

namespace guitarrack {
// Control-only voicings. Cabinet IR assets remain part of full rack presets.
class PedalPresetStore {
public:
    explicit PedalPresetStore(std::filesystem::path path) : directory(std::move(path)) {}
    void save(const EffectPreset&,const EffectRegistry&) const;
    std::vector<EffectPreset> list(const EffectRegistry&,std::vector<std::string>& errors) const;
    std::filesystem::path directory;
};
std::string serializePedalPreset(const EffectPreset&,const EffectRegistry&);
EffectPreset parsePedalPreset(const std::string&,const EffectRegistry&);
}
