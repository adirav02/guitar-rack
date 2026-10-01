#include "Amplifiers.h"
#include "TonePrimitives.h"
namespace guitarrack {
namespace {
using namespace tone;
class ToneEQ final:public EffectProcessor {
public:
    ToneEQ(bool param,const ParameterBank& b,SlotId s,std::vector<ParameterDescriptor> p):parametric(param),controls(b,s,std::move(p)){}
    bool prepare(double r,int chunk) override {if(!std::isfinite(r)||r<8000||r>768000||chunk<1)return false;
        rate=r;reset();return true;}
    void reset() noexcept override {bands={};controls.update(true,rate);configure(true);}
    void activate(const EffectProcessor* old) noexcept override {if(auto* e=dynamic_cast<const ToneEQ*>(old);e&&e->rate==rate&&e->parametric==parametric){bands=e->bands;controls.ramps=e->controls.ramps;}controls.update(!old,rate);configure(!old);}
    double tailSeconds() const noexcept override{return 1.;}
    void process(float* l,float* r,int n) noexcept override {
        if(n<=0)return;
        controls.update(false,rate);configure(false);
        for(int i=0;i<n;++i){const auto v=controls.next();const double gain=db(v[parametric?11:5]);for(unsigned c=0;c<2;++c){float& x=c?r[i]:l[i];double y=std::isfinite(x)?x:0;for(auto& b:bands[c])y=b.tick(y);x=static_cast<float>(y*gain);}}
    }
private:
    void configure(bool immediate) noexcept {
        const int ramp=immediate?0:int(rate*.02);
        for(auto& channel:bands) {
            if(parametric) {
                for(unsigned b=0;b<3;++b)channel[b].design(2,controls.target(b*3),controls.target(b*3+1),controls.target(b*3+2),rate,ramp);
                channel[3].design(1,controls.target(9),0,.707,rate,ramp);channel[4].design(0,controls.target(10),0,.707,rate,ramp);
            } else {const std::array<double,5> hz{80,240,750,2200,6600};for(unsigned b=0;b<5;++b)channel[b].design(2,hz[b],controls.target(b),.65,rate,ramp);}
        }
    }
    bool parametric;Controls controls;double rate=48000;std::array<std::array<Biquad,5>,2> bands{};
};
class Gate final:public EffectProcessor {
public:
    Gate(const ParameterBank& b,SlotId s,std::vector<ParameterDescriptor> p):controls(b,s,std::move(p)){}
    bool prepare(double r,int n) override {if(!std::isfinite(r)||r<8000||r>768000||n<1)return false;
        rate=r;reset();return true;}
    void reset() noexcept override {envelope=0;gain=0;hold=0;open=false;controls.update(true,rate);}
    void activate(const EffectProcessor* old) noexcept override {if(auto* g=dynamic_cast<const Gate*>(old);g&&g->rate==rate){envelope=g->envelope;gain=g->gain;hold=g->hold;open=g->open;controls.ramps=g->controls.ramps;}controls.update(!old,rate);}
    double tailSeconds() const noexcept override{return 2.;}
    void process(float* l,float* r,int n) noexcept override {
        if(n<=0)return;
        controls.update(false,rate);
        for(int i=0;i<n;++i){const auto v=controls.next();const double peak=std::max(std::isfinite(l[i])?std::abs(l[i]):0.f,std::isfinite(r[i])?std::abs(r[i]):0.f);
            const double p=std::exp(-1/(rate*(peak>envelope?.0003:.008)));envelope=clean(p*envelope+(1-p)*peak);
            const double threshold=db(v[0]),closeThreshold=db(v[0]-v[5]);
            if(envelope>=threshold){open=true;hold=static_cast<unsigned>(rate*v[3]*.001);}
            else if(open&&envelope<closeThreshold){if(hold)--hold;else open=false;}
            const double target=open?1:db(-v[1]);const double smooth=std::exp(-1/(rate*(open?v[2]:v[4])*.001));gain=clean(smooth*gain+(1-smooth)*target);
            l[i]=std::isfinite(l[i])?static_cast<float>(l[i]*gain):0;r[i]=std::isfinite(r[i])?static_cast<float>(r[i]*gain):0;
        }
    }
private:Controls controls;double rate=48000,envelope=0,gain=0;unsigned hold=0;bool open=false;
};
}
EffectDescriptor graphicEQDescriptor() {
    std::vector<ParameterDescriptor> p;const std::array<const char*,5> keys{"80_hz_db","240_hz_db","750_hz_db","2200_hz_db","6600_hz_db"};const std::array<const char*,5> labels{"80 Hz","240 Hz","750 Hz","2.2 kHz","6.6 kHz"};
    for(unsigned b=0;b<5;++b)p.push_back({keys[b],labels[b],"dB",b,-12,12,0});
    p.push_back({"output_db","Output","dB",5,-24,12,0});
    return {"graphic_eq","Five-Band Graphic EQ","Filter",1,p,[p](const ParameterBank& b,SlotId s){return std::make_unique<ToneEQ>(false,b,s,p);}};
}
EffectDescriptor parametricEQDescriptor() {
    std::vector<ParameterDescriptor> p;
    for(unsigned b=0;b<3;++b){const auto name=std::to_string(b+1);p.push_back({"band"+name+"_hz","Band "+name+" Freq","Hz",b*3,30,16000,float(std::array{160,750,3000}[b])});p.push_back({"band"+name+"_db","Band "+name+" Gain","dB",b*3+1,-12,12,0});p.push_back({"band"+name+"_q","Band "+name+" Q","Q",b*3+2,.3,5,.7});}
    p.push_back({"high_pass_hz","High Pass","Hz",9,20,500,20});p.push_back({"low_pass_hz","Low Pass","Hz",10,1000,20000,18000});p.push_back({"output_db","Output","dB",11,-24,12,0});
    return {"parametric_eq","Parametric EQ / HP-LP","Filter",1,p,[p](const ParameterBank& b,SlotId s){return std::make_unique<ToneEQ>(true,b,s,p);}};
}
EffectDescriptor noiseGateDescriptor() {
    std::vector<ParameterDescriptor> p{{"threshold_db","Threshold","dBFS",0,-90,-6,-54},{"range_db","Range","dB",1,0,90,60},{"attack_ms","Attack","ms",2,.1,50,2},{"hold_ms","Hold","ms",3,0,500,40},{"release_ms","Release","ms",4,5,2000,160},{"hysteresis_db","Hysteresis","dB",5,0,18,6}};
    return {"noise_gate","Noise Gate","Dynamics",1,p,[p](const ParameterBank& b,SlotId s){return std::make_unique<Gate>(b,s,p);}};
}
}
