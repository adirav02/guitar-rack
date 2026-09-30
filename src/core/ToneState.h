#pragma once
#include "Effects.h"

namespace guitarrack {
// Configuration only; no device choices, transport, mute or DSP histories.
struct ToneControls {
    std::array<std::array<float, controlsPerSlot>, rackSlots> values{};
    std::array<bool, rackSlots> bypass{};
    float inputDb = 0, outputDb = -12;
    bool rackBypass = false;
    bool operator==(const ToneControls&) const = default;
};
struct ToneState {
    unsigned schemaVersion = 1;
    std::string id, name;
    RackDefinition rack;
    ToneControls controls;
    bool operator==(const ToneState&) const = default;
};
std::string validateTone(const ToneState&, const EffectRegistry&);
}
