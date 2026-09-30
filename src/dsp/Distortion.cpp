#include "Distortion.h"
#include <algorithm>
#include <cmath>
#include <numbers>

namespace guitarrack {
bool DistortionProcessor::prepare(double sampleRate, int chunk) {
    if (!std::isfinite(sampleRate) || sampleRate < 8000 || sampleRate > 768000 || chunk <= 0) return false;
    rate = sampleRate;
    rampSamples = static_cast<int>(std::ceil(rate * 0.020));
    bassPole = std::exp(-2 * std::numbers::pi * 90 / rate);
    // Blackman-windowed sinc, cutoff 0.1125 cycles/high-rate sample = 90%
    // of base Nyquist. This short bounded FIR deliberately trades bandwidth
    // and stop-band rejection against CPU cost and eight-sample latency.
    double sum = 0;
    for (unsigned i = 0; i < taps; ++i) {
        const double x = static_cast<double>(i) - (taps - 1) * 0.5;
        const double window = 0.42 - 0.5 * std::cos(2 * std::numbers::pi * i / (taps - 1))
            + 0.08 * std::cos(4 * std::numbers::pi * i / (taps - 1));
        coefficients[i] = window * (x == 0 ? 0.225 : std::sin(2 * std::numbers::pi * 0.1125 * x) / (std::numbers::pi * x));
        sum += coefficients[i];
    }
    for (auto& coefficient : coefficients) coefficient /= sum;
    reset();
    return true;
}
void DistortionProcessor::controls(bool immediate) noexcept {
    const std::array<float, 4> targets{
        std::pow(10.0f, (48 * bank.value(slot, 0)) / 20),
        static_cast<float>(std::exp(-2 * std::numbers::pi * std::min(rate * 0.45, static_cast<double>(400 + 7600 * bank.value(slot, 1))) / rate)),
        std::pow(10.0f, (-30 + 36 * bank.value(slot, 2)) / 20), bank.value(slot, 3)};
    const std::array<LinearRamp*, 4> ramps{&drive, &tonePole, &level, &mix};
    for (unsigned i = 0; i < ramps.size(); ++i) {
        if (immediate) ramps[i]->reset(targets[i]); else ramps[i]->aim(targets[i], rampSamples);
    }
}
void DistortionProcessor::reset() noexcept { channels = {}; controls(true); }
void DistortionProcessor::activate(const EffectProcessor* previous) noexcept {
    if (const auto* old = dynamic_cast<const DistortionProcessor*>(previous); old && old->rate == rate) {
        drive = old->drive; tonePole = old->tonePole; level = old->level; mix = old->mix;
        channels = old->channels; // Fixed ~1 KB, never copy dynamically allocated resources.
    }
    controls(previous == nullptr);
}
void DistortionProcessor::process(float* l, float* r, int count) noexcept {
    if (count <= 0) return;
    controls(false);
    for (int i = 0; i < count; ++i) {
        const float gain = drive.next(), pole = tonePole.next(), output = level.next(), wet = mix.next();
        const std::array<float*, 2> samples{l + i, r + i};
        for (unsigned channel = 0; channel < channels.size(); ++channel) {
            auto& state = channels[channel];
            const float input = std::isfinite(*samples[channel]) ? std::clamp(*samples[channel], -1000000.f, 1000000.f) : 0;
            const float dry = state.dry[state.dryWrite];
            state.dry[state.dryWrite] = input;
            if (++state.dryWrite == latency) state.dryWrite = 0;
            state.highPass = bassPole * (state.highPass + input - state.previousInput);
            state.previousInput = input;
            if (std::abs(state.highPass) < 1.0e-20) state.highPass = 0;
            state.input[state.inputWrite] = state.highPass;
            double decimated = 0;
            for (unsigned phase = 0; phase < factor; ++phase) {
                double interpolated = 0;
                unsigned read = state.inputWrite;
                for (unsigned tap = phase; tap < taps; tap += factor) {
                    interpolated += coefficients[tap] * state.input[read];
                    read = read == 0 ? inputHistory - 1 : read - 1;
                }
                state.clipped[state.clippedWrite] = std::clamp(interpolated * factor * gain, -1.0, 1.0);
                // Decimate phase zero: the input and output sampling grids share
                // their origin, making the total FIR group delay exactly 8.
                if (phase == 0) {
                    read = state.clippedWrite;
                    for (unsigned tap = 0; tap < taps; ++tap) {
                        decimated += coefficients[tap] * state.clipped[read];
                        read = read == 0 ? taps - 1 : read - 1;
                    }
                }
                if (++state.clippedWrite == taps) state.clippedWrite = 0;
            }
            if (++state.inputWrite == inputHistory) state.inputWrite = 0;
            state.tone = (1 - pole) * decimated + pole * state.tone;
            if (std::abs(state.tone) < 1.0e-20) state.tone = 0;
            *samples[channel] = wet == 0 ? dry : (1 - wet) * dry + wet * static_cast<float>(state.tone * output);
        }
    }
}
EffectDescriptor distortionDescriptor() {
    return {"distortion", "Distortion", "Drive", 1,
        {{"drive_db", "Drive", "dB", 0, 0, 48, 12}, {"tone_hz", "Tone", "Hz", 1, 400, 8000, 2800},
         {"level_db", "Level", "dB", 2, -30, 6, -12}, {"mix_percent", "Mix", "%", 3, 0, 100, 50}},
        [](const ParameterBank& b, SlotId s) { return std::make_unique<DistortionProcessor>(b, s); }};
}
}
