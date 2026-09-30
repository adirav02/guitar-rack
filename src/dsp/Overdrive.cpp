#include "Overdrive.h"
#include <algorithm>
#include <cmath>
#include <numbers>

namespace guitarrack {
bool OverdriveProcessor::prepare(double sampleRate, int chunk) {
    if (!std::isfinite(sampleRate) || sampleRate < 8000 || sampleRate > 768000 || chunk <= 0) return false;
    rate = sampleRate; rampSamples = static_cast<int>(std::ceil(rate * 0.020));
    reset(); return true;
}
void OverdriveProcessor::controls(bool immediate) noexcept {
    const std::array<float, 4> targets{
        std::pow(10.0f, (36 * bank.value(slot, 0)) / 20),
        static_cast<float>(std::exp(-2 * std::numbers::pi * std::min(rate * 0.45, static_cast<double>(200 + 7800 * bank.value(slot, 1))) / rate)),
        std::pow(10.0f, (-24 + 30 * bank.value(slot, 2)) / 20), bank.value(slot, 3)};
    std::array<LinearRamp*, 4> ramps{&drive, &tonePole, &level, &mix};
    for (unsigned i = 0; i < ramps.size(); ++i) {
        if (immediate) ramps[i]->reset(targets[i]); else ramps[i]->aim(targets[i], rampSamples);
    }
}
void OverdriveProcessor::reset() noexcept { toneState = {}; controls(true); }
void OverdriveProcessor::activate(const EffectProcessor* previous) noexcept {
    if (const auto* old = dynamic_cast<const OverdriveProcessor*>(previous); old && old->rate == rate) {
        drive = old->drive; tonePole = old->tonePole; level = old->level; mix = old->mix; toneState = old->toneState;
    }
    controls(previous == nullptr);
}
void OverdriveProcessor::process(float* l, float* r, int count) noexcept {
    if (count <= 0) return;
    controls(false);
    for (int i = 0; i < count; ++i) {
        const float g = drive.next(), pole = tonePole.next(), out = level.next(), wet = mix.next();
        std::array<float*, 2> samples{l + i, r + i};
        for (unsigned channel = 0; channel < 2; ++channel) {
            const float dry = std::isfinite(*samples[channel]) ? *samples[channel] : 0;
            // Symmetric tanh clip, then a one-pole tone low-pass and wet-only level.
            const double clipped = std::tanh(static_cast<double>(dry) * g);
            toneState[channel] = (1 - pole) * clipped + pole * toneState[channel];
            if (std::abs(toneState[channel]) < 1.0e-20) toneState[channel] = 0;
            *samples[channel] = wet == 0 ? dry : (1 - wet) * dry + wet * static_cast<float>(toneState[channel] * out);
        }
    }
}
EffectDescriptor overdriveDescriptor() {
    return {"overdrive", "Overdrive", "Drive", 1,
        {{"drive_db", "Drive", "dB", 0, 0, 36, 6}, {"tone_hz", "Tone", "Hz", 1, 200, 8000, 3000},
         {"level_db", "Level", "dB", 2, -24, 6, -6}, {"mix_percent", "Mix", "%", 3, 0, 100, 25}},
        [](const ParameterBank& b, SlotId s) { return std::make_unique<OverdriveProcessor>(b, s); }};
}
}
