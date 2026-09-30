#pragma once
#include "Presets.h"
#include "core/Impulse.h"
namespace guitarrack {
inline constexpr std::size_t maxBundleBytes=17*1024*1024;
class AssetStore {
public:
    explicit AssetStore(std::filesystem::path root) : directory(std::move(root)) {}
    std::string importIR(const std::filesystem::path&) const;
    std::string install(std::span<const std::uint8_t>) const;
    std::vector<std::uint8_t> bytes(const std::string&) const;
    ImpulseResponse resolve(const std::string& id) const { return decodeIR(bytes(id)); }
    void validateAssets(const ToneState&) const;
    void exportBundle(const std::filesystem::path&,const ToneState&,const EffectRegistry&) const;
    ToneState importPreset(const std::filesystem::path&,const EffectRegistry&) const;
    std::filesystem::path directory;
};
}
