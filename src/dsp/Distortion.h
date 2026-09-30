#pragma once
#include "core/Effects.h"
#include "core/LinearRamp.h"

namespace guitarrack {
// Project-owned hard clipper, with a fixed four-times polyphase FIR pair.
// Both 33-tap filters have 16 high-rate samples of group delay: eight base
// samples combined. Tone/voicing filters additionally have ordinary frequency-
// dependent phase; the reported latency is the FIR transport delay.
class DistortionProcessor final : public EffectProcessor {
public:
    DistortionProcessor(const ParameterBank& authority, SlotId identity) : bank(authority), slot(identity) {}
    bool prepare(double, int) override;
    void reset() noexcept override;
    void activate(const EffectProcessor*) noexcept override;
    void process(float*, float*, int) noexcept override;
    int latencySamples() const noexcept override { return 8; }
    double tailSeconds() const noexcept override { return 0.1; }
private:
    static constexpr unsigned factor = 4, taps = 33, inputHistory = 9, latency = 8;
    struct Channel {
        std::array<double, inputHistory> input{};
        std::array<double, taps> clipped{};
        std::array<float, latency> dry{};
        unsigned inputWrite = 0, clippedWrite = 0, dryWrite = 0;
        double previousInput = 0, highPass = 0, tone = 0;
    };
    void controls(bool immediate) noexcept;
    const ParameterBank& bank;
    SlotId slot;
    double rate = 48000, bassPole = 0;
    int rampSamples = 960;
    LinearRamp drive, tonePole, level, mix;
    std::array<double, taps> coefficients{};
    std::array<Channel, 2> channels{};
};
EffectDescriptor distortionDescriptor();
}
