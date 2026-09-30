#pragma once
#include "core/Effects.h"
#include "core/LinearRamp.h"

namespace guitarrack {
class GainProcessor final : public EffectProcessor {
public:
    GainProcessor(const ParameterBank& bank, SlotId slot) : bank(bank), slot(slot) {}
    bool prepare(double sampleRate, int maximumChunk) override;
    void reset() noexcept override;
    void activate(const EffectProcessor* previous) noexcept override;
    void process(float*, float*, int) noexcept override;
private:
    float targetGain() const noexcept;
    const ParameterBank& bank;
    SlotId slot;
    int rampSamples = 1;
    LinearRamp gain;
};
EffectDescriptor gainDescriptor();
}
