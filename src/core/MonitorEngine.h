#pragma once
#include <atomic>
#include <cstdint>

namespace guitarrack {
static_assert(std::atomic<float>::is_always_lock_free);
static_assert(std::atomic<bool>::is_always_lock_free);
static_assert(std::atomic<unsigned>::is_always_lock_free);
static_assert(std::atomic<int>::is_always_lock_free);

struct MeterSnapshot {
    float inputPeak = 0, outputPeak = 0;
    unsigned flags = 0;
};

// Sole authority for editable monitoring controls; no mirrored GUI/DSP model.
class MonitorControls {
public:
    bool setInputDb(float db) noexcept;
    bool setOutputDb(float db) noexcept;
    void setMuted(bool value) noexcept { muted.store(value, std::memory_order_relaxed); }
    float inputDb() const noexcept { return inDb.load(std::memory_order_relaxed); }
    float outputDb() const noexcept { return outDb.load(std::memory_order_relaxed); }
    bool isMuted() const noexcept { return muted.load(std::memory_order_relaxed); }
    float inputGain() const noexcept { return inGain.load(std::memory_order_relaxed); }
    float outputGain() const noexcept { return outGain.load(std::memory_order_relaxed); }
private:
    std::atomic<float> inDb{0}, outDb{-12};
    std::atomic<float> inGain{1}, outGain{0.2511886432f};
    std::atomic<bool> muted{false};
};

class MonitorEngine {
public:
    enum : unsigned { inputClipped = 1, outputProtected = 2, nonFiniteInput = 4 };
    explicit MonitorEngine(MonitorControls& authority) noexcept : controls(authority) {}
    // Lifecycle only, serialized against processing. Starts at silence.
    bool prepare(double sampleRate) noexcept;
    void reset() noexcept;
    // No scratch buffers: streaming per-sample processing accepts arbitrary sizes.
    // Inputs may alias corresponding outputs; read both before either write.
    void process(const float* inL, const float* inR, float* outL, float* outR,
                 int samples, bool running = true) noexcept;
    MeterSnapshot consumeMeters() noexcept;
    bool isSilent() const noexcept { return silent.load(std::memory_order_acquire); }
private:
    struct Ramp {
        float current = 0, target = 0, step = 0;
        int remaining = 0;
        void aim(float value, int duration) noexcept;
        float next() noexcept;
    } input, output, gate;
    MonitorControls& controls;
    int rampSamples = 960;
    bool prepared = false;
    std::atomic<float> inputPeak{0}, outputPeak{0};
    std::atomic<unsigned> flags{0};
    std::atomic<bool> silent{true};
};
}
