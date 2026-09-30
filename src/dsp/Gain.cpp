#include "Gain.h"
#include <cmath>

namespace guitarrack {
bool GainProcessor::prepare(double rate, int chunk) {
    if (!std::isfinite(rate) || rate < 8000 || rate > 768000 || chunk <= 0) return false;
    rampSamples = static_cast<int>(std::ceil(rate * 0.020));
    reset(); return true;
}
float GainProcessor::targetGain() const noexcept {
    // Gain v1 Control 00: linear dB normalization [-60,+24]; never change silently.
    const float db = -60.0f + 84.0f * bank.value(slot, 0);
    return std::pow(10.0f, db / 20.0f);
}
void GainProcessor::reset() noexcept { gain.reset(targetGain()); }
void GainProcessor::activate(const EffectProcessor* previous) noexcept {
    if (const auto* old = dynamic_cast<const GainProcessor*>(previous)) gain = old->gain;
    else reset();
    gain.aim(targetGain(), rampSamples);
}
void GainProcessor::process(float* l, float* r, int count) noexcept {
    gain.aim(targetGain(), rampSamples);
    for (int i = 0; i < count; ++i) {
        const float level = gain.next(); l[i] *= level; r[i] *= level;
    }
}
EffectDescriptor gainDescriptor() {
    return {"gain", "Gain", "Level", 1, {{"level_db", "Level", "dB", 0, -60, 24, 0}},
        [](const ParameterBank& bank, SlotId slot) { return std::make_unique<GainProcessor>(bank, slot); }};
}
}
