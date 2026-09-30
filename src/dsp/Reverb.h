#pragma once
#include "core/Effects.h"
#include "core/LinearRamp.h"

namespace guitarrack {
// Project-owned stereo Schroeder network. Each input has its own eight damped
// combs/four allpasses; independent stereo inputs are never summed to mono.
class ReverbProcessor final : public EffectProcessor {
public:
    ReverbProcessor(const ParameterBank& authority, SlotId identity) : bank(authority), slot(identity) {}
    bool prepare(double, int) override;
    void reset() noexcept override; // Off audio: clear prepared rings.
    void activate(const EffectProcessor*) noexcept override; // Small controls only.
    void process(float*, float*, int) noexcept override;
    double tailSeconds() const noexcept override { return 10.0; }
private:
    struct Line {
        std::vector<float> samples;
        std::size_t cursor = 0;
        float lowpass = 0;
    };
    struct Channel {
        std::array<Line, 8> combs;
        std::array<Line, 4> allpasses;
    };
    void controls(bool) noexcept;
    const ParameterBank& bank;
    SlotId slot;
    std::array<Channel, 2> channels;
    double rate = 48000;
    int rampSamples = 960;
    bool prepared = false;
    LinearRamp feedback, damping, mix, ingress;
};
EffectDescriptor reverbDescriptor();
}
