#include "Cabinet.h"
#include <algorithm>
#include <cmath>
#include <numbers>

namespace guitarrack {
bool CabinetProcessor::prepare(double sampleRate,int chunk) {
    if(chunk<=0||!std::isfinite(sampleRate)||sampleRate<8000||sampleRate>768000) return false;
    if(!configured) { impulse=decodeIR(syntheticIRBytes()); configured=true; }
    auto ir=resampleIR(impulse,sampleRate);
    rate=sampleRate; duration=ir.samples.size()/rate; rampSamples=static_cast<int>(std::ceil(rate*.020));
    for(unsigned i=0;i<fftSize/2;++i) {
        auto angle=-2*std::numbers::pi*i/fftSize;
        twiddles[i]={static_cast<float>(std::cos(angle)),static_cast<float>(std::sin(angle))};
    }
    for(unsigned i=0;i<fftSize;++i) { unsigned v=i,r=0; for(int j=0;j<9;++j) {r=(r<<1)|(v&1); v>>=1;} reversed[i]=r; }
    head.fill(0); headLength=std::min(hop,ir.samples.size()); std::copy_n(ir.samples.begin(),headLength,head.begin());
    auto parts=ir.samples.size()>hop?(ir.samples.size()-hop+hop-1)/hop:0;
    kernels.assign(parts,{});
    for(std::size_t p=0;p<parts;++p) {
        for(std::size_t i=0;i<hop&&hop*(p+1)+i<ir.samples.size();++i) kernels[p][i]=ir.samples[hop*(p+1)+i];
        fft(kernels[p],false);
    }
    for(auto& c:channels) c.history.assign(parts,{});
    reset(); return true;
}
void CabinetProcessor::fft(Block& b,bool inverse) const noexcept {
    for(unsigned i=0;i<fftSize;++i) if(i<reversed[i]) std::swap(b[i],b[reversed[i]]);
    for(unsigned length=2;length<=fftSize;length*=2) {
        auto half=length/2; auto step=fftSize/length;
        for(unsigned start=0;start<fftSize;start+=length) for(unsigned j=0;j<half;++j) {
            auto t=twiddles[j*step]; if(inverse) t=std::conj(t);
            auto a=b[start+j],v=t*b[start+j+half]; b[start+j]=a+v; b[start+j+half]=a-v;
        }
    }
    if(inverse) for(auto& x:b) x*=1.0f/fftSize;
}
void CabinetProcessor::controls(bool immediate) noexcept {
    auto gain=std::pow(10.0f,(-24+30*bank.value(slot,0))/20);
    if(immediate) level.reset(gain); else level.aim(gain,rampSamples);
}
void CabinetProcessor::reset() noexcept {
    phase=historyPosition=0;
    for(auto& c:channels) {
        c.direct.fill(0); c.input.fill(0); c.tail.fill(0); c.overlap.fill(0);
        for(auto& h:c.history) h.fill({});
    }
    scratch.fill({}); sum.fill({}); ingress.reset(1); controls(true);
}
void CabinetProcessor::activate(const EffectProcessor* previous) noexcept {
    if(auto* old=dynamic_cast<const CabinetProcessor*>(previous);old&&old->rate==rate) level=old->level;
    controls(previous==nullptr); ingress.reset(0); ingress.aim(1,rampSamples);
}
void CabinetProcessor::finishBlock(Channel& c) noexcept {
    if(kernels.empty()) return;
    scratch.fill({}); for(std::size_t i=0;i<hop;++i) scratch[i]=c.input[i]; fft(scratch,false);
    c.history[historyPosition]=scratch; sum.fill({});
    for(std::size_t p=0;p<kernels.size();++p) {
        auto index=historyPosition>=p?historyPosition-p:historyPosition+kernels.size()-p;
        for(std::size_t i=0;i<fftSize;++i) sum[i]+=kernels[p][i]*c.history[index][i];
    }
    fft(sum,true);
    for(std::size_t i=0;i<hop;++i) {
        c.tail[i]=sum[i].real()+c.overlap[i]; c.overlap[i]=sum[i+hop].real();
        if(std::abs(c.tail[i])<1e-20f) c.tail[i]=0;
        if(std::abs(c.overlap[i])<1e-20f) c.overlap[i]=0;
    }
}
void CabinetProcessor::process(float* l,float* r,int count) noexcept {
    if(count<=0) return;
    controls(false);
    for(int i=0;i<count;++i) {
        auto gain=level.next(),fade=ingress.next(); std::array<float*,2> in{l+i,r+i};
        for(unsigned ch=0;ch<2;++ch) {
            auto& c=channels[ch];
            auto x=std::isfinite(*in[ch])?std::clamp(*in[ch],-1e4f,1e4f)*fade:0;
            c.direct[phase]=c.input[phase]=x; double y=c.tail[phase];
            for(std::size_t k=0;k<headLength;++k) y+=head[k]*c.direct[phase>=k?phase-k:phase+hop-k];
            *in[ch]=std::isfinite(y)?static_cast<float>(std::clamp(y*gain,-1e6,1e6)):0;
        }
        if(++phase==hop) {
            for(auto& c:channels) finishBlock(c);
            if(!kernels.empty()&&++historyPosition==kernels.size()) historyPosition=0;
            phase=0;
        }
    }
}
EffectDescriptor cabinetDescriptor() {
    return {"cabinet","Cabinet / IR","Cabinet",1,{{"level_db","Level","dB",0,-24,6,0}},
        [](const ParameterBank& b,SlotId s){return std::make_unique<CabinetProcessor>(b,s);}};
}
}
