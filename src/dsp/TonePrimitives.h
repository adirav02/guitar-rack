#pragma once
#include "core/Effects.h"
#include "core/LinearRamp.h"
#include <algorithm>
#include <cmath>
#include <numbers>
namespace guitarrack::tone {
constexpr double tau=2*std::numbers::pi;
inline double clean(double x) noexcept {return std::isfinite(x)&&std::abs(x)>1e-24?x:0;}
inline double db(double x) noexcept {return std::pow(10.,x/20);}
struct Biquad {
    std::array<double,5> c{1,0,0,0,0},target{1,0,0,0,0},step{};
    double x1=0,x2=0,y1=0,y2=0;int remaining=0;
    void reset() noexcept {x1=x2=y1=y2=0;}
    void set(std::array<double,5> next,int duration=0) noexcept {
        if(next==target)return;
        target=next;
        if(!duration){c=target;remaining=0;}else {remaining=duration;for(unsigned i=0;i<5;++i)step[i]=(target[i]-c[i])/duration;}
    }
    void design(int kind,double hz,double gain,double q,double rate,int duration=0) noexcept {
        hz=std::clamp(hz,10.,rate*.45);q=std::clamp(q,.2,12.);
        const double w=tau*hz/rate,co=std::cos(w),si=std::sin(w),a=si/(2*q),A=std::pow(10.,gain/40);
        double b0=1,b1=0,b2=0,a0=1,a1=0,a2=0;
        if(kind==0){b0=(1-co)/2;b1=1-co;b2=b0;a0=1+a;a1=-2*co;a2=1-a;}
        if(kind==1){b0=(1+co)/2;b1=-1-co;b2=b0;a0=1+a;a1=-2*co;a2=1-a;}
        if(kind==2){b0=1+a*A;b1=-2*co;b2=1-a*A;a0=1+a/A;a1=b1;a2=1-a/A;}
        if(kind==3||kind==4) {
            const double alpha=si/2*std::sqrt(2.),t=2*std::sqrt(A)*alpha;
            if(kind==3) {b0=A*((A+1)-(A-1)*co+t);b1=2*A*((A-1)-(A+1)*co);b2=A*((A+1)-(A-1)*co-t);a0=(A+1)+(A-1)*co+t;a1=-2*((A-1)+(A+1)*co);a2=(A+1)+(A-1)*co-t;}
            else {b0=A*((A+1)+(A-1)*co+t);b1=-2*A*((A-1)+(A+1)*co);b2=A*((A+1)+(A-1)*co-t);a0=(A+1)-(A-1)*co+t;a1=2*((A-1)-(A+1)*co);a2=(A+1)-(A-1)*co-t;}
        }
        set({b0/a0,b1/a0,b2/a0,a1/a0,a2/a0},duration);
    }
    double tick(double x) noexcept {
        if(remaining){for(unsigned i=0;i<5;++i)c[i]+=step[i];if(--remaining==0)c=target;}
        const double y=clean(c[0]*x+c[1]*x1+c[2]*x2-c[3]*y1-c[4]*y2);
        x2=x1;x1=x;y2=y1;y1=y;return y;
    }
};
struct Pole {
    double state=0;
    double cachedHz=-1,cachedRate=0,coefficient=0;
    double low(double x,double hz,double rate) noexcept {
        hz=std::min(hz,rate*.45);
        if(hz!=cachedHz||rate!=cachedRate){coefficient=std::exp(-tau*hz/rate);cachedHz=hz;cachedRate=rate;}
        state=clean((1-coefficient)*x+coefficient*state);return state;
    }
    double high(double x,double hz,double rate) noexcept {return x-low(x,hz,rate);}
};
// 8x, 129-tap Blackman sinc pair, 16 base samples transport latency.
// Model filters/nonlinearities execute at actual 8x internal rate. Antialias
// filters are distinct from the model's tone/voicing filters.
struct Oversampler {
    static constexpr unsigned factor=8,taps=129,history=17,latency=16;
    std::array<double,taps> coefficients{};
    struct Channel {std::array<double,history> input{};std::array<double,taps> output{};unsigned iw=0,ow=0;};
    std::array<Channel,2> channels{};
    void prepare() noexcept {
        double sum=0;
        for(unsigned i=0;i<taps;++i){double x=double(i)-(taps-1)*.5;double win=.42-.5*std::cos(tau*i/(taps-1))+.08*std::cos(2*tau*i/(taps-1));const double cutoff=.45/factor;coefficients[i]=win*(x==0?2*cutoff:std::sin(tau*cutoff*x)/(std::numbers::pi*x));sum+=coefficients[i];}
        for(auto& c:coefficients)c/=sum;
        channels={};
    }
    template<class Model> double tick(unsigned channel,double input,Model&& model) noexcept {
        auto& s=channels[channel];s.input[s.iw]=input;double y=0;
        for(unsigned phase=0;phase<factor;++phase) {
            double x=0;unsigned r=s.iw;
            for(unsigned j=phase;j<taps;j+=factor){x+=coefficients[j]*s.input[r];r=r?r-1:history-1;}
            s.output[s.ow]=model(x*factor);
            if(phase==0){r=s.ow;for(unsigned j=0;j<taps;++j){y+=coefficients[j]*s.output[r];r=r?r-1:taps-1;}}
            if(++s.ow==taps)s.ow=0;
        }
        if(++s.iw==history)s.iw=0;
        return clean(y);
    }
};
class Controls {
public:
    Controls(const ParameterBank& b,SlotId s,std::vector<ParameterDescriptor> d):bank(b),slot(s),descriptors(std::move(d)){}
    void update(bool immediate,double rate) noexcept {
        for(const auto& p:descriptors){const float v=p.denormalize(bank.value(slot,p.bankIndex));if(immediate)ramps[p.bankIndex].reset(v);else ramps[p.bankIndex].aim(v,std::max(1,int(rate*.02)));}
    }
    std::array<float,16> next() noexcept {std::array<float,16> v{};for(const auto& p:descriptors)v[p.bankIndex]=ramps[p.bankIndex].next();return v;}
    float target(unsigned i) const noexcept {return ramps[i].target;}
    const ParameterBank& bank;SlotId slot;std::vector<ParameterDescriptor> descriptors;std::array<LinearRamp,16> ramps{};
};
}
