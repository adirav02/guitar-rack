#include "MonitorEngine.h"
#include <algorithm>
#include <cmath>

namespace guitarrack {
bool MonitorControls::setInputDb(float db) noexcept {
    if (!std::isfinite(db) || db < -24 || db > 24) return false;
    inGain.store(std::pow(10.0f, db / 20), std::memory_order_relaxed);
    inDb.store(db, std::memory_order_relaxed);
    return true;
}
bool MonitorControls::setOutputDb(float db) noexcept {
    if (!std::isfinite(db) || db < -60 || db > 6) return false;
    outGain.store(std::pow(10.0f, db / 20), std::memory_order_relaxed);
    outDb.store(db, std::memory_order_relaxed);
    return true;
}
void MonitorEngine::Ramp::aim(float value, int duration) noexcept {
    if (value == target) return;
    target = value;
    remaining = duration;
    step = (target - current) / static_cast<float>(duration);
}
float MonitorEngine::Ramp::next() noexcept {
    if (remaining > 0) {
        current += step;
        if (--remaining == 0) current = target;
    }
    return current;
}
bool MonitorEngine::prepare(double rate) noexcept {
    prepared = std::isfinite(rate) && rate >= 8000 && rate <= 768000;
    rampSamples = prepared ? static_cast<int>(std::ceil(rate * 0.020)) : 960;
    reset();
    return prepared;
}
void MonitorEngine::reset() noexcept {
    input = {controls.inputGain(), controls.inputGain(), 0, 0};
    output = {controls.outputGain(), controls.outputGain(), 0, 0};
    gate = {};
    inputPeak.store(0); outputPeak.store(0); flags.store(0); silent.store(true);
}
void MonitorEngine::process(const float* inL, const float* inR, float* outL,
                           float* outR, int count, bool running) noexcept {
    if (count <= 0) return;
    input.aim(controls.inputGain(), rampSamples);
    output.aim(controls.outputGain(), rampSamples);
    gate.aim(prepared && running && !controls.isMuted() ? 1.0f : 0.0f, rampSamples);
    float inPeak = 0, outPeak = 0;
    unsigned events = 0;
    for (int i = 0; i < count; ++i) {
        float l = inL ? inL[i] : 0, r = inR ? inR[i] : 0;
        if (!std::isfinite(l)) { l = 0; events |= nonFiniteInput; }
        if (!std::isfinite(r)) { r = 0; events |= nonFiniteInput; }
        const float rawPeak = std::max(std::abs(l), std::abs(r));
        if (rawPeak >= 1) events |= inputClipped;
        const float trim = input.next();
        // Protect against overflow even for extreme but finite input samples.
        l = std::clamp(l, -64.0f, 64.0f) * trim;
        r = std::clamp(r, -64.0f, 64.0f) * trim;
        inPeak = std::max(inPeak, std::max(std::abs(l), std::abs(r)));
        if (inPeak >= 1) events |= inputClipped;
        const float level = output.next();
        l *= level; r *= level;
        if (std::max(std::abs(l), std::abs(r)) > 0.98f) events |= outputProtected;
        // Zero-latency hard peak ceiling. Linear below threshold; audible when hit.
        l = std::clamp(l, -0.98f, 0.98f);
        r = std::clamp(r, -0.98f, 0.98f);
        const float fade = gate.next();
        l *= fade; r *= fade;
        if (outL) outL[i] = l;
        if (outR) outR[i] = r;
        outPeak = std::max(outPeak, std::max(std::abs(l), std::abs(r)));
    }
    // Exchange/restore is bounded; a GUI poll racing publication can delay a peak
    // by one poll but cannot cause an unbounded CAS loop in audio.
    inputPeak.store(std::max(inPeak, inputPeak.exchange(0)), std::memory_order_relaxed);
    outputPeak.store(std::max(outPeak, outputPeak.exchange(0)), std::memory_order_relaxed);
    flags.fetch_or(events, std::memory_order_relaxed);
    silent.store(gate.current == 0 && gate.remaining == 0, std::memory_order_release);
}
MeterSnapshot MonitorEngine::consumeMeters() noexcept {
    return {inputPeak.exchange(0), outputPeak.exchange(0), flags.exchange(0)};
}
}
