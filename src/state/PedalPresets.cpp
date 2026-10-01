#include "PedalPresets.h"
#include <algorithm>
#include <cmath>
#include <set>
#include <stdexcept>

namespace guitarrack {
namespace {
void expectKeys(const juce::var& object,const std::vector<std::string>& keys) {
    const auto* value=object.getDynamicObject();
    if (!value || value->getProperties().size()!=static_cast<int>(keys.size()))
        throw std::runtime_error("Missing or unknown pedal preset fields.");
    for (const auto& key:keys) if (!value->hasProperty(key.c_str()))
        throw std::runtime_error("Missing pedal preset field: "+key);
}
std::string stringValue(const juce::var& v) {
    if (!v.isString()) throw std::runtime_error("Expected a pedal preset string.");
    return v.toString().toStdString();
}
double numberValue(const juce::var& v) {
    if (!v.isInt() && !v.isInt64() && !v.isDouble()) throw std::runtime_error("Expected a pedal preset number.");
    return static_cast<double>(v);
}
}
EffectPreset parsePedalPreset(const std::string& text,const EffectRegistry& registry) {
    const auto root=parseJson(text);
    expectKeys(root,{"schema_version","preset_id","name","effect_type","effect_version","parameters"});
    if (numberValue(root["schema_version"])!=1) throw std::runtime_error("Unsupported pedal preset schema.");
    const auto version=numberValue(root["effect_version"]);
    if (version<1 || version>100000 || version!=std::floor(version)) throw std::runtime_error("Invalid effect version.");
    EffectPreset p{stringValue(root["preset_id"]),stringValue(root["name"]),stringValue(root["effect_type"]),static_cast<unsigned>(version),{}};
    const auto* d=registry.find(p.effectType,p.effectVersion);
    if (!d) throw std::runtime_error("Unknown pedal effect or version.");
    std::vector<std::string> keys;
    for(const auto& control:d->parameters) keys.push_back(control.key);
    const auto params=root["parameters"]; expectKeys(params,keys);
    for(const auto& control:d->parameters) p.values.push_back(static_cast<float>(numberValue(params[control.key.c_str()])));
    if(auto error=validateEffectPreset(p,registry);!error.empty()) throw std::runtime_error(error);
    return p;
}
std::string serializePedalPreset(const EffectPreset& p,const EffectRegistry& registry) {
    if(auto error=validateEffectPreset(p,registry);!error.empty()) throw std::runtime_error(error);
    auto object=std::make_unique<juce::DynamicObject>();
    object->setProperty("schema_version",1);
    object->setProperty("preset_id",juce::String(p.id));
    object->setProperty("name",juce::String::fromUTF8(p.name.c_str()));
    object->setProperty("effect_type",juce::String(p.effectType));
    object->setProperty("effect_version",static_cast<int>(p.effectVersion));
    auto params=std::make_unique<juce::DynamicObject>();
    const auto* d=registry.find(p.effectType,p.effectVersion);
    for(std::size_t j=0;j<p.values.size();++j) params->setProperty(d->parameters[j].key.c_str(),p.values[j]);
    object->setProperty("parameters",juce::var(params.release()));
    return juce::JSON::toString(juce::var(object.release())).toStdString()+"\n";
}
void PedalPresetStore::save(const EffectPreset& p,const EffectRegistry& registry) const {
    for(const auto& factory:effectPresets(registry)) if(factory.id==p.id)
        throw std::runtime_error("Use a new identity to save a factory pedal.");
    atomicWrite(directory/(p.id+".json"),serializePedalPreset(p,registry));
}
std::vector<EffectPreset> PedalPresetStore::list(const EffectRegistry& registry,std::vector<std::string>& errors) const {
    std::vector<EffectPreset> result;
    try {
        if(!std::filesystem::exists(directory)) return result;
        unsigned count=0;
        for(const auto& entry:std::filesystem::directory_iterator(directory)) {
            if(++count>512) { errors.push_back("Pedal library exceeds 512 entries."); break; }
            if(entry.path().extension()!=".json") continue;
            try {
                const auto id=entry.path().stem().string();
                if(!validInstanceId(id) || entry.is_symlink()) throw std::runtime_error("Invalid pedal preset entry.");
                auto p=parsePedalPreset(readBounded(entry.path()),registry);
                if(p.id!=id) throw std::runtime_error("Pedal filename and identity disagree.");
                result.push_back(std::move(p));
            } catch(const std::exception& e) { errors.push_back(entry.path().filename().string()+": "+e.what()); }
        }
        std::sort(result.begin(),result.end(),[](const auto& a,const auto& b){return a.name<b.name;});
    } catch(const std::exception& e) { errors.push_back(e.what()); }
    return result;
}
}
