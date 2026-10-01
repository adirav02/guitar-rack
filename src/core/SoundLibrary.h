#pragma once
#include "ToneState.h"

namespace guitarrack {
// Pedal voicings change controls only: identity, slot, bypass and selected IR stay.
struct EffectPreset {
    std::string id, name, effectType;
    unsigned effectVersion=1;
    std::vector<float> values; // Physical values in descriptor order.
};
std::string validateEffectPreset(const EffectPreset&,const EffectRegistry&);
std::vector<EffectPreset> effectPresets(const EffectRegistry&);
std::vector<ToneState> experimentalFactoryPresets(const EffectRegistry&);
}
