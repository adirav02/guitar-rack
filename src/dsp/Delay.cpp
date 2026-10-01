#include "Delay.h"
#include <algorithm>
#include <cmath>
#include <numbers>

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
    if (extended) {
        const std::array<float,3> targets{2*bank.value(slot,3),
            static_cast<float>(std::exp(-2*std::numbers::pi*std::min(rate*.45,500.0+11500*bank.value(slot,4))/rate)),bank.value(slot,5)};
        const std::array<LinearRamp*,3> ramps{&character,&colorPole,&wow};
        for (unsigned j=0;j<3;++j) { if(immediate) ramps[j]->reset(targets[j]); else ramps[j]->aim(targets[j],rampSamples); }
    }
    const float fb = 0.85f * bank.value(slot, 1), wet = bank.value(slot, 2);
    if (immediate) { feedback.reset(fb); mix.reset(wet); }
    else { feedback.aim(fb, rampSamples); mix.aim(wet, rampSamples); }
}
void DelayProcessor::reset() noexcept {
    for (auto& channel : storage) std::fill(channel.begin(), channel.end(), 0);
    write = 0; phase=0; lowpass={}; ingress.reset(1); controls(true);
}
void DelayProcessor::activate(const EffectProcessor* previous) noexcept {
    // Prepared graph has an empty ring. Copying seconds of history in a callback
    // would violate bounded activation work; structural swaps fade that history out.
    if (const auto* old = dynamic_cast<const DelayProcessor*>(previous); old && old->rate == rate) {
        currentTime = old->currentTime; nextTime = old->nextTime; timeMix = old->timeMix;
        feedback = old->feedback; mix = old->mix;
        if (extended==old->extended) { character=old->character; colorPole=old->colorPole; wow=old->wow; phase=old->phase; }
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
        const float mode=extended?character.next():0, pole=extended?colorPole.next():0, depth=extended?wow.next():0;
        phase+=.7/rate; phase-=std::floor(phase);
        const float modulation=mode>1 ? static_cast<float>((mode-1)*depth*.0008*rate*std::sin(2*std::numbers::pi*phase)) : 0;
        std::array<float*, 2> samples{l+i, r+i};
        for (unsigned channel = 0; channel < 2; ++channel) {
            const float dry = std::isfinite(*samples[channel]) ? std::clamp(*samples[channel], -1.0e6f, 1.0e6f) : 0;
            float delayed = (1-crossfade)*readAt(storage[channel], currentTime+modulation) + crossfade*readAt(storage[channel], nextTime+modulation);
            if (extended) {
                lowpass[channel]=(1-pole)*delayed+pole*lowpass[channel];
                if (std::abs(lowpass[channel])<1e-20f) lowpass[channel]=0;
                const float amount=std::min(mode,1.f);
                delayed=(1-amount)*delayed+amount*lowpass[channel];
                const float tape=std::max(0.f,mode-1);
                delayed=(1-tape)*delayed+tape*std::tanh(delayed*1.2f)/1.2f;
            }
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
EffectDescriptor delayV2Descriptor() {
    auto d=delayDescriptor(); d.version=2;
    d.parameters.push_back({"mode","Mode","",3,0,2,0,20,{"Digital","Analog","Tape"}});
    d.parameters.push_back({"color_hz","Color","Hz",4,500,12000,4500});
    d.parameters.push_back({"wow_percent","Wow","%",5,0,100,15});
    d.factory=[](const ParameterBank& b,SlotId s) { return std::make_unique<DelayProcessor>(b,s,true); };
    return d;
}

}

