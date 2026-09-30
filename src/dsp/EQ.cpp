#include "EQ.h"
#include <algorithm>
#include <cmath>
#include <numbers>

namespace guitarrack {
bool EQProcessor::prepare(double sampleRate, int chunk) {
    if (!std::isfinite(sampleRate) || sampleRate < 8000 || sampleRate > 768000 || chunk <= 0) return false;
    rate = sampleRate; rampSamples = static_cast<int>(std::ceil(rate * 0.020)); reset(); return true;
}
EQProcessor::Coefficients EQProcessor::coefficients(unsigned band, float db) const noexcept {
    if (db == 0) return {1, 0, 0, 0, 0}; // Exact wire, including repeated resets.
    // RBJ/W3C Audio EQ Cookbook: shelves S=1; peaking Q=0.70710678.
    constexpr std::array<double, 3> frequency{150, 1000, 4000};
    const double w = 2 * std::numbers::pi * std::min(frequency[band], rate * 0.45) / rate;
    const double c = std::cos(w), s = std::sin(w), A = std::pow(10.0, db / 40.0);
    const double alpha = s / std::sqrt(2.0), beta = 2 * std::sqrt(A) * alpha;
    double b0, b1, b2, a0, a1, a2;
    if (band == 0) {
        b0 = A * ((A+1) - (A-1)*c + beta); b1 = 2*A*((A-1) - (A+1)*c); b2 = A*((A+1) - (A-1)*c - beta);
        a0 = (A+1) + (A-1)*c + beta; a1 = -2*((A-1) + (A+1)*c); a2 = (A+1) + (A-1)*c - beta;
    } else if (band == 1) {
        b0 = 1 + alpha*A; b1 = -2*c; b2 = 1-alpha*A;
        a0 = 1+alpha/A; a1 = -2*c; a2 = 1-alpha/A;
    } else {
        b0 = A*((A+1) + (A-1)*c + beta); b1 = -2*A*((A-1) + (A+1)*c); b2 = A*((A+1) + (A-1)*c - beta);
        a0 = (A+1) - (A-1)*c + beta; a1 = 2*((A-1) - (A+1)*c); a2 = (A+1) - (A-1)*c - beta;
    }
    return {b0/a0, b1/a0, b2/a0, a1/a0, a2/a0};
}
void EQProcessor::controls(bool immediate) noexcept {
    for (unsigned i = 0; i < bands.size(); ++i) {
        auto& b = bands[i]; const float db = -12 + 24 * bank.value(slot, i);
        if (!immediate && db == b.db) continue;
        b.db = db; b.target = coefficients(i, db);
        if (immediate) { b.current = b.target; b.remaining = 0; }
        else {
            b.remaining = rampSamples;
            for (unsigned j = 0; j < b.step.size(); ++j) b.step[j] = (b.target[j] - b.current[j]) / rampSamples;
        }
    }
}
void EQProcessor::reset() noexcept { bands = {}; controls(true); }
void EQProcessor::activate(const EffectProcessor* previous) noexcept {
    if (const auto* old = dynamic_cast<const EQProcessor*>(previous); old && old->rate == rate) bands = old->bands;
    controls(previous == nullptr);
}
void EQProcessor::process(float* l, float* r, int count) noexcept {
    if (count <= 0) return;
    controls(false);
    for (int i = 0; i < count; ++i) {
        for (auto& b : bands) {
            if (b.remaining > 0) {
                for (unsigned j = 0; j < b.current.size(); ++j) b.current[j] += b.step[j];
                if (--b.remaining == 0) b.current = b.target;
            }
            const auto& c = b.current;
            std::array<float*, 2> samples{l+i, r+i};
            for (unsigned channel = 0; channel < 2; ++channel) {
                auto& h = b.history[channel];
                const double x = std::isfinite(*samples[channel]) ? std::clamp(static_cast<double>(*samples[channel]), -1.0e6, 1.0e6) : 0;
                double y = c[0]*x + c[1]*h.x1 + c[2]*h.x2 - c[3]*h.y1 - c[4]*h.y2;
                y = std::isfinite(y) ? std::clamp(y, -1.0e6, 1.0e6) : 0;
                if (std::abs(y) < 1.0e-20) y = 0;
                h.x2 = h.x1; h.x1 = x; h.y2 = h.y1; h.y1 = y;
                *samples[channel] = static_cast<float>(y);
            }
        }
    }
}
EffectDescriptor eqDescriptor() {
    return {"eq", "EQ", "Filter", 1,
        {{"low_db", "Low", "dB", 0, -12, 12, 0}, {"mid_db", "Mid", "dB", 1, -12, 12, 0}, {"high_db", "High", "dB", 2, -12, 12, 0}},
        [](const ParameterBank& b, SlotId s) { return std::make_unique<EQProcessor>(b, s); }};
}
}
