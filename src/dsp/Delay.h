#pragma once
#include "core/Effects.h"
#include "core/LinearRamp.h"

namespace guitarrack {
class DelayProcessor final : public EffectProcessor {
public:
    DelayProcessor(const ParameterBank& authority, SlotId identity, bool extended=false) : bank(authority), slot(identity), extended(extended) {}
    bool prepare(double, int) override;
    void reset() noexcept override; // Off audio only: clears preallocated temporal storage.
    void activate(const EffectProcessor*) noexcept override; // Never copies/clears the ring.
    void process(float*, float*, int) noexcept override;
    double tailSeconds() const noexcept override { return 116.0; } // 2 s, .85 feedback, -80 dB threshold.
private:
    void controls(bool immediate) noexcept;
    const ParameterBank& bank;
    SlotId slot;
    double rate = 48000;
    int rampSamples = 960, timeRampSamples = 4800;
    std::array<std::vector<float>, 2> storage;
    std::size_t write = 0;
    float currentTime = 1, nextTime = 1, targetTime = 1;
    LinearRamp timeMix, feedback, mix, ingress, character, colorPole, wow;
    bool extended=false;
    double phase=0;
    std::array<float,2> lowpass{};
};
EffectDescriptor delayDescriptor();
EffectDescriptor delayV2Descriptor();
}

