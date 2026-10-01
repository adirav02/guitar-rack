#pragma once
#include <array>
#include "Effects.h"
namespace guitarrack {
// Fixed catalog latency budget: every graph, dry rack path and transition agrees.
inline constexpr unsigned rackLatencySamples = 128, creativeLatencySamples = 4096;
struct StereoDelay {
    std::array<std::array<float,creativeLatencySamples>,2> data{};
    unsigned length=0,position=0;
    void reset() noexcept { data={}; position=0; }
    // Only inherit short transport history. Long alignment rings warm from fresh
    // storage; never copy 4096-sample histories at the activation boundary.
    void inherit(const StereoDelay& previous) noexcept {
        if(length!=previous.length || length>128)return;
        position=previous.position;
        for(unsigned c=0;c<2;++c) for(unsigned i=0;i<length;++i)data[c][i]=previous.data[c][i];
    }
    void tick(float& l,float& r) noexcept {
        if(!length) return;
        auto dl=data[0][position],dr=data[1][position];
        data[0][position]=l; data[1][position]=r; l=dl; r=dr;
        if(++position==length) position=0;
    }
};
}
