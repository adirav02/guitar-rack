#pragma once
#include "core/Effects.h"
#include "core/LinearRamp.h"

namespace guitarrack {
class OverdriveProcessor final : public EffectProcessor {
public:
    OverdriveProcessor(const ParameterBank& authority, SlotId identity) : bank(authority), slot(identity) {}
    bool prepare(double, int) override;
    void reset() noexcept override;
    void activate(const EffectProcessor*) noexcept override;
    void process(float*, float*, int) noexcept override;
    double tailSeconds() const noexcept override { return 0.02; }
private:
    void controls(bool immediate) noexcept;
    const ParameterBank& bank;
    SlotId slot;
    double rate = 48000;
    int rampSamples = 960;
    LinearRamp drive, tonePole, level, mix;
    std::array<double, 2> toneState{};
};
EffectDescriptor overdriveDescriptor();
}
