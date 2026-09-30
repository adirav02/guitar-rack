#pragma once
#include "core/Effects.h"

namespace guitarrack {
class EQProcessor final : public EffectProcessor {
public:
    EQProcessor(const ParameterBank& authority, SlotId identity) : bank(authority), slot(identity) {}
    bool prepare(double, int) override;
    void reset() noexcept override;
    void activate(const EffectProcessor*) noexcept override;
    void process(float*, float*, int) noexcept override;
    double tailSeconds() const noexcept override { return 0.25; }
private:
    using Coefficients = std::array<double, 5>; // b0, b1, b2, a1, a2; a0 = 1
    struct History { double x1 = 0, x2 = 0, y1 = 0, y2 = 0; };
    struct Band {
        Coefficients current{}, target{}, step{};
        std::array<History, 2> history{};
        int remaining = 0;
        float db = 1000; // Invalid sentinel, only changed on the DSP owner thread.
    };
    Coefficients coefficients(unsigned band, float db) const noexcept;
    void controls(bool immediate) noexcept;
    const ParameterBank& bank;
    SlotId slot;
    double rate = 48000;
    int rampSamples = 960;
    std::array<Band, 3> bands{};
};
EffectDescriptor eqDescriptor();
}
