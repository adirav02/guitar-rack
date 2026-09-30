#pragma once
namespace guitarrack {
struct LinearRamp {
    float current = 0, target = 0, step = 0;
    int remaining = 0;
    void reset(float value) noexcept { current = target = value; step = 0; remaining = 0; }
    void aim(float value, int duration) noexcept {
        if (value == target) return;
        target = value; remaining = duration;
        step = (target - current) / static_cast<float>(duration);
    }
    float next() noexcept {
        if (remaining > 0) {
            current += step;
            if (--remaining == 0) current = target;
        }
        return current;
    }
};
}
