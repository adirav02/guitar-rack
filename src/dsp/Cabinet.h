#pragma once
#include "core/Effects.h"
#include "core/Impulse.h"
#include "core/LinearRamp.h"
#include <complex>

namespace guitarrack {
class CabinetProcessor final : public EffectProcessor {
public:
    CabinetProcessor(const ParameterBank& b, SlotId s) : bank(b),slot(s) {}
    void setImpulse(ImpulseResponse ir) { impulse=std::move(ir); configured=true; }
    bool prepare(double, int) override;
    void reset() noexcept override;
    void activate(const EffectProcessor*) noexcept override;
    void process(float*,float*,int) noexcept override;
    double tailSeconds() const noexcept override { return duration; }
private:
    static constexpr std::size_t hop=256, fftSize=hop*2;
    using Complex=std::complex<float>;
    using Block=std::array<Complex,fftSize>;
    struct Channel {
        std::array<float,hop> direct{}, input{}, tail{}, overlap{};
        std::vector<Block> history;
    };
    void fft(Block&,bool inverse) const noexcept;
    void finishBlock(Channel&) noexcept;
    void controls(bool immediate) noexcept;
    const ParameterBank& bank; SlotId slot;
    ImpulseResponse impulse;
    bool configured=false;
    double rate=48000,duration=0;
    int rampSamples=960;
    std::array<float,hop> head{};
    std::size_t headLength=0,phase=0,historyPosition=0;
    std::array<Channel,2> channels;
    std::vector<Block> kernels;
    std::array<Complex,fftSize/2> twiddles{};
    std::array<unsigned,fftSize> reversed{};
    Block scratch{}, sum{};
    LinearRamp level,ingress;
};
EffectDescriptor cabinetDescriptor();
}
