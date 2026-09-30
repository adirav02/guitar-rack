#include "Reverb.h"
#include <algorithm>
#include <cmath>

namespace guitarrack {
namespace {
// Fixed musical delays at 44.1 kHz. They scale once during preparation; Size
// changes decay feedback, never delay length, avoiding moving read heads.
// Nominal prime lengths spread the resonances; these tunings/implementation
// are project-owned, with no copied external reverb source.
constexpr std::array<int, 8> combLengths{1091, 1181, 1279, 1361, 1429, 1499, 1559, 1621};
constexpr std::array<int, 4> allpassLengths{557, 443, 347, 227};
float guarded(float value) noexcept {
    if (!std::isfinite(value)) return 0;
    if (std::abs(value) < 1.0e-20f) return 0;
    return std::clamp(value, -1.0e6f, 1.0e6f);
}
}
bool ReverbProcessor::prepare(double sampleRate, int chunk) {
    if (!std::isfinite(sampleRate) || sampleRate < 8000 || sampleRate > 768000 || chunk <= 0) return false;
    prepared = false;
    rate = sampleRate;
    rampSamples = static_cast<int>(std::ceil(rate * 0.020));
    for (unsigned c = 0; c < channels.size(); ++c) {
        // Different left/right timings widen duplicated mono guitar, while each
        // network still receives only its own input. No channel crosstalk.
        const int stereoOffset = c == 0 ? 0 : 23;
        for (unsigned i = 0; i < combLengths.size(); ++i)
            channels[c].combs[i].samples.assign(static_cast<std::size_t>(std::max(1.0, std::round((combLengths[i]+stereoOffset)*rate/44100.0))), 0);
        for (unsigned i = 0; i < allpassLengths.size(); ++i)
            channels[c].allpasses[i].samples.assign(static_cast<std::size_t>(std::max(1.0, std::round((allpassLengths[i]+stereoOffset)*rate/44100.0))), 0);
    }
    prepared = true;
    reset();
    return true;
}
void ReverbProcessor::controls(bool immediate) noexcept {
    const float fb = 0.45f + 0.45f*bank.value(slot, 0);
    const float damp = 0.90f*bank.value(slot, 1);
    const float wet = bank.value(slot, 2);
    if (immediate) { feedback.reset(fb); damping.reset(damp); mix.reset(wet); }
    else { feedback.aim(fb, rampSamples); damping.aim(damp, rampSamples); mix.aim(wet, rampSamples); }
}
void ReverbProcessor::reset() noexcept {
    for (auto& channel : channels) {
        for (auto& line : channel.combs) { std::fill(line.samples.begin(), line.samples.end(), 0); line.cursor = 0; line.lowpass = 0; }
        for (auto& line : channel.allpasses) { std::fill(line.samples.begin(), line.samples.end(), 0); line.cursor = 0; }
    }
    ingress.reset(1);
    controls(true);
}
void ReverbProcessor::activate(const EffectProcessor* previous) noexcept {
    if (const auto* old = dynamic_cast<const ReverbProcessor*>(previous); old && old->rate == rate) {
        feedback = old->feedback; damping = old->damping; mix = old->mix;
    }
    // All rings belong to the fresh prepared graph; never copy/clear histories
    // on audio. Old tails fade out with the existing structural crossfade.
    controls(previous == nullptr);
    ingress.reset(0); ingress.aim(1, rampSamples);
}
void ReverbProcessor::process(float* left, float* right, int count) noexcept {
    if (count <= 0) return;
    if (!prepared) {
        for (int i = 0; i < count; ++i) left[i] = right[i] = 0;
        return;
    }
    controls(false);
    for (int i = 0; i < count; ++i) {
        const float fb = std::clamp(feedback.next(), 0.45f, 0.90f);
        const float damp = std::clamp(damping.next(), 0.0f, 0.90f);
        const float wet = std::clamp(mix.next(), 0.0f, 1.0f), inputFade = ingress.next();
        const std::array<float*, 2> outputs{left+i, right+i};
        for (unsigned c = 0; c < channels.size(); ++c) {
            // Keep the zero-Mix path exact for normal finite samples, including
            // tiny input values; flush only temporal state to prevent denormals.
            const float dry = std::isfinite(*outputs[c]) ? std::clamp(*outputs[c], -1.0e6f, 1.0e6f) : 0;
            const float input = dry*inputFade*0.12f;
            float reverberant = 0;
            for (auto& line : channels[c].combs) {
                const float delayed = line.samples[line.cursor];
                line.lowpass = guarded((1-damp)*delayed + damp*line.lowpass);
                line.samples[line.cursor] = guarded(input + fb*line.lowpass);
                reverberant += delayed*0.125f;
                if (++line.cursor == line.samples.size()) line.cursor = 0;
            }
            for (auto& line : channels[c].allpasses) {
                const float delayed = line.samples[line.cursor];
                const float output = guarded(delayed - 0.5f*reverberant);
                line.samples[line.cursor] = guarded(reverberant + 0.5f*output);
                reverberant = output;
                if (++line.cursor == line.samples.size()) line.cursor = 0;
            }
            *outputs[c] = wet == 0 ? dry : guarded((1-wet)*dry + wet*reverberant);
        }
    }
}
EffectDescriptor reverbDescriptor() {
    return {"reverb", "Reverb", "Time", 1,
        {{"size_percent", "Size", "%", 0, 0, 100, 35},
         {"damping_percent", "Damping", "%", 1, 0, 100, 55},
         {"mix_percent", "Mix", "%", 2, 0, 100, 15}},
        [](const ParameterBank& b, SlotId s) { return std::make_unique<ReverbProcessor>(b, s); }};
}
}
