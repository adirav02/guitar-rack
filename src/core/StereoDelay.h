#pragma once
#include <array>
#include "Effects.h"
namespace guitarrack {
// Fixed catalog latency budget: every graph, dry rack path and transition agrees.
inline constexpr unsigned rackLatencySamples = 128;
struct StereoDelay {
    std::array<std::array<float,rackLatencySamples>,2> data{};
    unsigned length=0,position=0;
    void reset() noexcept { data={}; position=0; }
    void tick(float& l,float& r) noexcept {
        if(!length) return;
        auto dl=data[0][position],dr=data[1][position];
        data[0][position]=l; data[1][position]=r; l=dl; r=dr;
        if(++position==length) position=0;
    }
};
}
