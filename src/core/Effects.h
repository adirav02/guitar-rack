#pragma once
#include <array>
#include <atomic>
#include <cstddef>
#include <functional>
#include <memory>
#include <string>
#include <vector>

namespace guitarrack {
inline constexpr std::size_t rackSlots = 16, controlsPerSlot = 16;
inline constexpr int rackChunkSize = 512;
using InstanceId = std::string; // Canonical UUID, persisted unchanged by future M4 state.
using SlotId = unsigned;
InstanceId newInstanceId();
bool validInstanceId(const InstanceId&) noexcept;

struct ParameterDescriptor {
    std::string key, label, units;
    unsigned bankIndex = 0;
    float minimum = 0, maximum = 1, defaultValue = 0;
    float smoothingMs = 20; // v1 linear amplitude ramp for Gain; versioned contract
    float normalize(float value) const noexcept;
    float denormalize(float normalized) const noexcept;
};

// Fixed bank, one authority. Graphs read it, never write snapshot values back.
class ParameterBank {
public:
    ParameterBank();
    float value(SlotId slot, unsigned index) const noexcept;
    bool setValue(SlotId slot, unsigned index, float normalized) noexcept;
    bool bypassed(SlotId slot) const noexcept;
    void setBypassed(SlotId slot, bool bypass) noexcept;
    void setRackBypassed(bool bypass) noexcept { rackBypass.store(bypass); }
    bool rackBypassed() const noexcept { return rackBypass.load(); }
    unsigned references(SlotId slot) const noexcept { return reservations[slot].load(); }
    static std::string parameterId(SlotId, unsigned);
    static std::string parameterName(SlotId, unsigned);
    static std::string bypassId(SlotId);
    void reserve(unsigned mask) noexcept;
    void unreserve(unsigned mask) noexcept;
private:
    std::array<std::array<std::atomic<float>, controlsPerSlot>, rackSlots> values;
    std::array<std::atomic<bool>, rackSlots> bypass;
    std::array<std::atomic<unsigned>, rackSlots> reservations;
    std::atomic<bool> rackBypass{false};
};

class EffectProcessor {
public:
    virtual ~EffectProcessor() = default; // Only the builder/reclaimer destroys processors.
    virtual bool prepare(double sampleRate, int maximumChunk) = 0;
    virtual void reset() noexcept = 0;
    // RT hook: resolve current controls, optionally inherit matching-instance state.
    virtual void activate(const EffectProcessor* previous) noexcept = 0;
    virtual void process(float* left, float* right, int samples) noexcept = 0;
    virtual int latencySamples() const noexcept { return 0; }
    virtual double tailSeconds() const noexcept { return 0; }
};
struct EffectDescriptor {
    std::string typeId, displayName, category;
    unsigned version = 1;
    std::vector<ParameterDescriptor> parameters;
    std::function<std::unique_ptr<EffectProcessor>(const ParameterBank&, SlotId)> factory;
};
class EffectRegistry {
public:
    bool add(EffectDescriptor descriptor);
    const EffectDescriptor* find(const std::string& type, unsigned version) const noexcept;
    const std::vector<EffectDescriptor>& descriptors() const noexcept { return entries; }
    static EffectRegistry builtIns();
private:
    std::vector<EffectDescriptor> entries;
};
struct RackInstance {
    InstanceId id;
    SlotId slot = 0;
    std::string effectType;
    unsigned effectVersion = 1;
    bool operator==(const RackInstance&) const = default;
};
struct RackDefinition {
    std::vector<RackInstance> instances;
    std::vector<InstanceId> order;
    const RackInstance* find(const InstanceId&) const noexcept;
    bool operator==(const RackDefinition&) const = default;
};
std::string validateRack(const RackDefinition&, const EffectRegistry&);
}
