#include "Effects.h"
#include "dsp/Gain.h"
#include "dsp/Overdrive.h"
#include "dsp/EQ.h"
#include "dsp/Delay.h"
#include "dsp/Cabinet.h"
#include "dsp/Distortion.h"
#include "dsp/Reverb.h"
#include "dsp/Experimental.h"
#include "Impulse.h"
#include <cmath>
#include <cstdio>
#include <random>
#include <unordered_set>

namespace guitarrack {
bool validUtf8(const std::string& s) noexcept {
    for (std::size_t i = 0; i < s.size();) {
        const unsigned c = static_cast<unsigned char>(s[i++]);
        if (c < 128) { if (c == 0) return false; continue; }
        unsigned n = 0, v = 0, minimum = 0;
        if (c >= 0xc2 && c <= 0xdf) { n=1; v=c&31; minimum=128; }
        else if (c >= 0xe0 && c <= 0xef) { n=2; v=c&15; minimum=2048; }
        else if (c >= 0xf0 && c <= 0xf4) { n=3; v=c&7; minimum=65536; }
        else return false;
        while (n--) { if (i == s.size()) return false; unsigned b=static_cast<unsigned char>(s[i++]); if ((b&0xc0)!=0x80) return false; v=(v<<6)|(b&63); }
        if (v < minimum || v > 0x10ffff || (v >= 0xd800 && v <= 0xdfff)) return false;
    }
    return true;
}
InstanceId newInstanceId() {
    std::random_device random;
    std::array<unsigned char, 16> bytes{};
    for (unsigned i = 0; i < 16; i += 4) {
        auto word = random();
        for (unsigned j = 0; j < 4; ++j) bytes[i + j] = static_cast<unsigned char>(word >> (8 * j));
    }
    bytes[6] = (bytes[6] & 0x0f) | 0x40; bytes[8] = (bytes[8] & 0x3f) | 0x80;
    const char hex[] = "0123456789abcdef";
    std::string id; id.reserve(36);
    for (unsigned i = 0; i < 16; ++i) {
        if (i == 4 || i == 6 || i == 8 || i == 10) id.push_back('-');
        id.push_back(hex[bytes[i] >> 4]); id.push_back(hex[bytes[i] & 15]);
    }
    return id;
}
bool validInstanceId(const InstanceId& id) noexcept {
    if (id.size() != 36) return false;
    for (unsigned i = 0; i < 36; ++i) {
        if (i == 8 || i == 13 || i == 18 || i == 23) { if (id[i] != '-') return false; }
        else if (!((id[i] >= '0' && id[i] <= '9') || (id[i] >= 'a' && id[i] <= 'f'))) return false;
    }
    return id != "00000000-0000-0000-0000-000000000000";
}
bool ParameterDescriptor::accepts(float value) const noexcept {
    return std::isfinite(value) && value >= minimum && value <= maximum
        && (choices.empty() || value == std::round(value));
}
float ParameterDescriptor::normalize(float value) const noexcept { return (value - minimum) / (maximum - minimum); }
float ParameterDescriptor::denormalize(float value) const noexcept { return minimum + value * (maximum - minimum); }
ParameterBank::ParameterBank() {
    for (auto& row : values) for (auto& value : row) value.store(0);
    for (auto& value : bypass) value.store(false);
    for (auto& value : reservations) value.store(0);
}
float ParameterBank::value(SlotId slot, unsigned index) const noexcept { return values[slot][index].load(); }
bool ParameterBank::setValue(SlotId slot, unsigned index, float value) noexcept {
    if (slot >= rackSlots || index >= controlsPerSlot || !std::isfinite(value) || value < 0 || value > 1) return false;
    sequences[slot].fetch_add(1);
    if (values[slot][index].exchange(value) != value) edits.fetch_add(1);
    sequences[slot].fetch_add(1);
    return true;
}
bool ParameterBank::bypassed(SlotId slot) const noexcept { return bypass[slot].load(); }
void ParameterBank::setBypassed(SlotId slot, bool value) noexcept {
    if (slot >= rackSlots) return;
    sequences[slot].fetch_add(1);
    if (bypass[slot].exchange(value) != value) edits.fetch_add(1);
    sequences[slot].fetch_add(1);
}
bool ParameterBank::setValues(SlotId slot, const std::array<float, controlsPerSlot>& row) noexcept {
    if (slot >= rackSlots) return false;
    for (float x : row) if (!std::isfinite(x) || x < 0 || x > 1) return false;
    sequences[slot].fetch_add(1);
    bool changed = false;
    for (unsigned p=0; p<controlsPerSlot; ++p) changed |= values[slot][p].exchange(row[p]) != row[p];
    if (changed) edits.fetch_add(1);
    sequences[slot].fetch_add(1);
    return true;
}
bool ParameterBank::readSlot(SlotId slot, std::array<float, controlsPerSlot>& row, bool& state) const noexcept {
    if (slot >= rackSlots) return false;
    // Two attempts, then retain the previous DSP view. Atomic payload plus seq_cst
    // ordering makes this a bounded coherent transfer, not a blocking seqlock.
    for (unsigned attempt=0; attempt<2; ++attempt) {
        const auto before=sequences[slot].load();
        if (before & 1) continue;
        std::array<float, controlsPerSlot> candidate{};
        for (unsigned p=0;p<controlsPerSlot;++p) candidate[p]=values[slot][p].load();
        const bool candidateBypass=bypass[slot].load();
        if (sequences[slot].load()==before) { row=candidate; state=candidateBypass; return true; }
    }
    return false;
}
std::string ParameterBank::parameterId(SlotId slot, unsigned index) {
    char id[40]; std::snprintf(id, sizeof(id), "slot_%02u_param_%02u", slot, index); return id;
}
std::string ParameterBank::parameterName(SlotId slot, unsigned index) {
    char name[48]; std::snprintf(name, sizeof(name), "Slot %02u Control %02u", slot, index + 1); return name;
}
std::string ParameterBank::bypassId(SlotId slot) {
    char id[32]; std::snprintf(id, sizeof(id), "slot_%02u_bypass", slot); return id;
}
void ParameterBank::reserve(unsigned mask) noexcept {
    for (unsigned slot = 0; slot < rackSlots; ++slot) if (mask & (1u << slot)) reservations[slot].fetch_add(1);
}
void ParameterBank::unreserve(unsigned mask) noexcept {
    for (unsigned slot = 0; slot < rackSlots; ++slot) if (mask & (1u << slot)) reservations[slot].fetch_sub(1);
}
bool EffectRegistry::add(EffectDescriptor d) {
    if (d.typeId.empty() || d.typeId.size() > 64 || d.version == 0 || !d.factory || find(d.typeId, d.version)
        || d.parameters.size() > controlsPerSlot) return false;
    unsigned indices = 0; std::unordered_set<std::string> keys;
    for (const auto& p : d.parameters) {
        if (p.key.empty() || p.bankIndex >= controlsPerSlot || (indices & (1u << p.bankIndex))
            || !keys.insert(p.key).second || !std::isfinite(p.minimum) || !std::isfinite(p.maximum)
            || !std::isfinite(p.defaultValue) || p.maximum <= p.minimum
            || !p.accepts(p.defaultValue)
            || (!p.choices.empty() && (p.minimum != 0 || p.maximum != p.choices.size()-1))
            || !std::isfinite(p.smoothingMs) || p.smoothingMs < 0) return false;
        indices |= 1u << p.bankIndex;
    }
    entries.push_back(std::move(d)); return true;
}
const EffectDescriptor* EffectRegistry::find(const std::string& type, unsigned version) const noexcept {
    for (const auto& entry : entries) if (entry.typeId == type && entry.version == version) return &entry;
    return nullptr;
}
const EffectDescriptor* EffectRegistry::latest(const std::string& type) const noexcept {
    const EffectDescriptor* result=nullptr;
    for (const auto& d:entries) if (d.typeId==type && (!result || d.version>result->version)) result=&d;
    return result;
}
EffectRegistry EffectRegistry::builtIns() {
    EffectRegistry result;
    result.add(gainDescriptor()); result.add(overdriveDescriptor()); result.add(eqDescriptor());
    result.add(delayDescriptor()); result.add(delayV2Descriptor()); result.add(distortionDescriptor());
    result.add(reverbDescriptor()); result.add(cabinetDescriptor()); result.add(fuzzDescriptor());
    result.add(ampDescriptor()); result.add(compressorDescriptor()); result.add(chorusDescriptor());
    result.add(tremoloDescriptor()); result.add(filterDescriptor()); result.add(pitchDescriptor());
    result.add(ringDescriptor()); return result;
}
bool sameIdentity(const RackInstance& a,const RackInstance& b) noexcept { return a.id==b.id&&a.slot==b.slot&&a.effectType==b.effectType&&a.effectVersion==b.effectVersion; }
const RackInstance* RackDefinition::find(const InstanceId& id) const noexcept {
    for (const auto& instance : instances) if (instance.id == id) return &instance;
    return nullptr;
}
std::string validateRack(const RackDefinition& d, const EffectRegistry& registry) {
    if (d.instances.size() > rackSlots || d.order.size() != d.instances.size()) return "Rack capacity/order is invalid (maximum 16).";
    unsigned slots = 0; std::unordered_set<std::string> ids, order;
    for (const auto& i : d.instances) {
        if (!validInstanceId(i.id) || !ids.insert(i.id).second) return "Invalid or duplicate instance UUID.";
        if (i.slot >= rackSlots || (slots & (1u << i.slot))) return "Invalid or duplicate slot ID.";
        slots |= 1u << i.slot;
        if (!registry.find(i.effectType, i.effectVersion)) return "Unknown effect or unsupported effect version.";
        if(i.effectType=="cabinet" ? !validAssetId(i.irAsset) : !i.irAsset.empty()) return "Cabinet needs a valid IR checksum identity; other effects cannot reference an IR.";
    }
    for (const auto& id : d.order) if (!ids.contains(id) || !order.insert(id).second) return "Invalid or duplicate order reference.";
    return {};
}
}

