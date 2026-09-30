#include "ToneState.h"
#include <cmath>
namespace guitarrack {
std::string validateTone(const ToneState& t, const EffectRegistry& r) {
    if (t.schemaVersion != 1 && t.schemaVersion != 2) return "Unsupported preset schema version.";
    if(t.schemaVersion==1) for(const auto& i:t.rack.instances) if(!i.irAsset.empty()) return "IR assets require preset schema version 2.";
    if (!validInstanceId(t.id)) return "Invalid preset UUID.";
    if (!validUtf8(t.name) || t.name.empty() || t.name.size() > 256 || t.name.find('\0') != std::string::npos) return "Preset name must contain 1–256 UTF-8 bytes.";
    if (auto e = validateRack(t.rack, r); !e.empty()) return e;
    if (!std::isfinite(t.controls.inputDb) || t.controls.inputDb < -24 || t.controls.inputDb > 24
        || !std::isfinite(t.controls.outputDb) || t.controls.outputDb < -60 || t.controls.outputDb > 6) return "Invalid global gain.";
    for (const auto& row : t.controls.values) for (float v : row)
        if (!std::isfinite(v) || v < 0 || v > 1) return "Invalid normalized parameter.";
    return {};
}
}
