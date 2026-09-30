#pragma once
#include <algorithm>

namespace guitarrack {
// JUCE ALSA/JACK callbacks pack enabled channels in increasing physical order.
// The adapter enables exactly ONE input and TWO outputs; channel indices must
// be derived from actual masks, never used directly against these packed arrays.
struct StereoRoute {
    int left = -1, right = -1;
    bool valid() const noexcept { return left >= 0 && right >= 0 && left != right; }
    int packedLeft() const noexcept { return left < right ? 0 : 1; }
    int packedRight() const noexcept { return right < left ? 0 : 1; }
};
inline void clearOutputs(float* const* outputs, int channels, int samples) noexcept {
    if (!outputs || samples <= 0) return;
    for (int ch = 0; ch < channels; ++ch)
        if (outputs[ch]) std::fill_n(outputs[ch], samples, 0.0f);
}
}
