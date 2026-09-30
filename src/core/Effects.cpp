#include "Effects.h"
#include "dsp/Gain.h"
#include "dsp/Overdrive.h"
#include "dsp/EQ.h"
#include "dsp/Delay.h"
#include <cmath>
#include <cstdio>
#include <random>
#include <unordered_set>

namespace guitarrack {
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
    values[slot][index].store(value); return true;
}
bool ParameterBank::bypassed(SlotId slot) const noexcept { return bypass[slot].load(); }
void ParameterBank::setBypassed(SlotId slot, bool value) noexcept { if (slot < rackSlots) bypass[slot].store(value); }
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
            || p.defaultValue < p.minimum || p.defaultValue > p.maximum
            || !std::isfinite(p.smoothingMs) || p.smoothingMs < 0) return false;
        indices |= 1u << p.bankIndex;
    }
    entries.push_back(std::move(d)); return true;
}
const EffectDescriptor* EffectRegistry::find(const std::string& type, unsigned version) const noexcept {
    for (const auto& entry : entries) if (entry.typeId == type && entry.version == version) return &entry;
    return nullptr;
}
EffectRegistry EffectRegistry::builtIns() { EffectRegistry result; result.add(gainDescriptor()); result.add(overdriveDescriptor()); result.add(eqDescriptor()); result.add(delayDescriptor()); return result; }
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
    }
    for (const auto& id : d.order) if (!ids.contains(id) || !order.insert(id).second) return "Invalid or duplicate order reference.";
    return {};
}
}
