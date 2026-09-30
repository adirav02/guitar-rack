#include "Delay.h"
#include <algorithm>
#include <cmath>

namespace guitarrack {
bool DelayProcessor::prepare(double sampleRate, int chunk) {
    if (!std::isfinite(sampleRate) || sampleRate < 8000 || sampleRate > 768000 || chunk <= 0) return false;
    rate = sampleRate; rampSamples = static_cast<int>(std::ceil(rate * 0.020)); timeRampSamples = static_cast<int>(std::ceil(rate * 0.100));
    for (auto& channel : storage) channel.assign(static_cast<std::size_t>(std::ceil(rate * 2)) + 2, 0);
    reset(); return true;
}
void DelayProcessor::controls(bool immediate) noexcept {
    const float samples = static_cast<float>((1 + 1999 * static_cast<double>(bank.value(slot, 0))) * rate / 1000);
    targetTime = samples;
    if (immediate) { currentTime = nextTime = targetTime; timeMix.reset(1); }
    const float fb = 0.85f * bank.value(slot, 1), wet = bank.value(slot, 2);
    if (immediate) { feedback.reset(fb); mix.reset(wet); }
    else { feedback.aim(fb, rampSamples); mix.aim(wet, rampSamples); }
}
void DelayProcessor::reset() noexcept {
    for (auto& channel : storage) std::fill(channel.begin(), channel.end(), 0);
    write = 0; ingress.reset(1); controls(true);
}
void DelayProcessor::activate(const EffectProcessor* previous) noexcept {
    // Prepared graph has an empty ring. Copying seconds of history in a callback
    // would violate bounded activation work; structural swaps fade that history out.
    if (const auto* old = dynamic_cast<const DelayProcessor*>(previous); old && old->rate == rate) {
        currentTime = old->currentTime; nextTime = old->nextTime; timeMix = old->timeMix;
        feedback = old->feedback; mix = old->mix;
    }
    controls(previous == nullptr);
    // Fade input written to the fresh history, avoiding a hard leading edge
    // when replacement echoes arrive after the structural output fade ends.
    ingress.reset(0); ingress.aim(1, rampSamples);
}
void DelayProcessor::process(float* l, float* r, int count) noexcept {
    if (count <= 0) return;
    controls(false);
    const auto size = storage[0].size();
    for (int i = 0; i < count; ++i) {
        // At most two fixed read heads. Finish a fade before accepting the newest
        // time request; fast edits coalesce rather than growing transition work.
        if (timeMix.remaining == 0 && currentTime != targetTime) {
            nextTime = targetTime; timeMix.reset(0); timeMix.aim(1, timeRampSamples);
        }
        const float crossfade = timeMix.next();
        const auto readAt = [&](const std::vector<float>& channel, float delay) noexcept {
            double read = static_cast<double>(write) - std::clamp(static_cast<double>(delay), 1.0, static_cast<double>(size-2));
            if (read < 0) read += static_cast<double>(size);
            const auto first = static_cast<std::size_t>(read), second = first + 1 == size ? 0 : first + 1;
            const float fraction = static_cast<float>(read - static_cast<double>(first));
            return (1-fraction)*channel[first] + fraction*channel[second];
        };
        const float fb = std::clamp(feedback.next(), 0.0f, 0.85f), wet = mix.next(), inputFade = ingress.next();
        std::array<float*, 2> samples{l+i, r+i};
        for (unsigned channel = 0; channel < 2; ++channel) {
            const float dry = std::isfinite(*samples[channel]) ? std::clamp(*samples[channel], -1.0e6f, 1.0e6f) : 0;
            const float delayed = (1-crossfade)*readAt(storage[channel], currentTime) + crossfade*readAt(storage[channel], nextTime);
            float next = std::clamp(dry*inputFade + fb*delayed, -1.0e6f, 1.0e6f);
            if (std::abs(next) < 1.0e-20f) next = 0;
            storage[channel][write] = next;
            *samples[channel] = wet == 0 ? dry : (1-wet)*dry + wet*delayed;
        }
        if (timeMix.remaining == 0) currentTime = nextTime;
        if (++write == size) write = 0;
    }
}
EffectDescriptor delayDescriptor() {
    return {"delay", "Delay", "Time", 1,
        {{"time_ms", "Time", "ms", 0, 1, 2000, 350, 100}, {"feedback_percent", "Feedback", "%", 1, 0, 85, 25},
         {"mix_percent", "Mix", "%", 2, 0, 100, 25}},
        [](const ParameterBank& b, SlotId s) { return std::make_unique<DelayProcessor>(b, s); }};
}
}
