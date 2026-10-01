#include "Amplifiers.h"
#include "TonePrimitives.h"
namespace guitarrack {
namespace {
using namespace tone;
class Drive final:public EffectProcessor {
public:
    Drive(int k,const ParameterBank& b,SlotId s,std::vector<ParameterDescriptor> p):kind(k),controls(b,s,std::move(p)){}
    bool prepare(double r,int chunk) override {if(!std::isfinite(r)||r<8000||r>768000||chunk<1)return false;
        rate=r;oversampler.prepare();reset();return true;}
    void reset() noexcept override {states={};oversampler.channels={};controls.update(true,rate);configure(true);}
    void activate(const EffectProcessor* old) noexcept override {
        if(auto* d=dynamic_cast<const Drive*>(old);d&&d->rate==rate&&d->kind==kind){states=d->states;oversampler.channels=d->oversampler.channels;controls.ramps=d->controls.ramps;}
        controls.update(!old,rate);configure(!old);
    }
    int latencySamples() const noexcept override{return 16;}
    double tailSeconds() const noexcept override{return .5;}
    void process(float* l,float* r,int count) noexcept override {
        if(count<=0)return;
        controls.update(false,rate);configure(false);
        for(int i=0;i<count;++i) {
            const auto v=controls.next();const double gain=db(v[0]);
            const unsigned output=kind==4?5:kind==1?4:3;
            const double level=db(v[output]);
            for(unsigned c=0;c<2;++c) {
                float& sample=c?r[i]:l[i];const double input=std::isfinite(sample)?std::clamp(double(sample),-1e3,1e3):0;
                auto& s=states[c];
                sample=static_cast<float>(oversampler.tick(c,input,[&](double x) {
                    const double internal=rate*Oversampler::factor;double y=0;
                    if(kind==0) { // Mid boost before two unequal soft rails; tighten pre-clip.
                        x=s.p[0].high(x,v[1],internal);x=s.eq[0].tick(x);
                        const double z=x*gain;y=z>=0?std::tanh(z):.65*std::tanh(z/.65);
                        y=s.p[1].low(y,v[2],internal);
                    } else if(kind==1) { // Oxford: early bass shelf, cascade, post-clip contour notch.
                        x=s.p[0].high(x,55,internal);x=s.eq[0].tick(x);
                        y=std::tanh(x*gain*.85);y=s.p[1].high(y,85,internal);
                        y=std::clamp(y*2.3,-.8,.95);y=s.eq[1].tick(y);y=s.eq[2].tick(y);
                        y=s.p[2].low(y,6500,internal);
                    } else if(kind==2) { // Opamp hard rails, clipping and continuously useful post filter.
                        x=s.p[0].high(x,v[1],internal);x=s.eq[0].tick(x);
                        y=std::clamp(x*gain,-.85,.85);y=s.p[1].low(y,v[2],internal);
                    } else if(kind==3) { // Sustaining fuzz: envelope compression plus two diode stages.
                        const double envPole=std::exp(-1/(internal*.045));s.envelope=clean(envPole*s.envelope+(1-envPole)*std::abs(x));
                        x=s.p[0].low(s.p[1].high(x,40,internal),4500,internal);
                        const double compress=1/(1+v[1]*.10*s.envelope);
                        y=std::tanh(x*gain*compress*1.3);y=s.p[2].high(y,35,internal);
                        y=std::tanh(y*3.2);y=s.p[3].low(y,v[2],internal);
                    } else if(kind==4) { // Bias-sensitive cutoff and bounded nonlinear feedback, no oscillator.
                        const double envPole=std::exp(-1/(internal*.004));s.envelope=clean(envPole*s.envelope+(1-envPole)*std::abs(x));
                        const double threshold=db(v[1]);
                        const double target=s.envelope>threshold?1:0;
                        s.gate=clean(s.gate+(target-s.gate)*(1-std::exp(-1/(internal*.003))));
                        const double bias=v[3]*.009,compression=1+v[2]*.04;
                        const double feed=.65*(1-v[4]*.01);
                        y=std::tanh(x*gain*compression+bias+feed*s.feedback)-std::tanh(bias);
                        // Cutoff grows asymmetrically with bias. Feedback <=.65 is contractive.
                        y=std::max(y,-.95+v[3]*.003);s.feedback=clean(s.p[0].high(y,25,internal));
                        y=s.p[1].low(s.feedback*s.gate,6000,internal);
                    } else { // Tight low-drive boost; attack shelf before gentle soft rail.
                        x=s.p[0].high(x,v[1],internal);x=s.eq[0].tick(x);y=2*std::tanh(x*gain/2);
                    }
                    const double p=std::exp(-tau*18/internal);s.dc=clean(p*(s.dc+y-s.previous));s.previous=y;return s.dc;
                })*level);
            }
        }
    }
private:
    struct State {std::array<Pole,4> p{};std::array<Biquad,3> eq{};double envelope=0,gate=0,feedback=0,dc=0,previous=0;};
    void configure(bool immediate) noexcept {
        const int duration=immediate?0:int(rate*.02*Oversampler::factor);
        for(auto& s:states) {
            if(kind==0)s.eq[0].design(2,850,6,.7,rate*Oversampler::factor,duration);
            if(kind==1){s.eq[0].design(3,150,controls.target(1),.7,rate*Oversampler::factor,duration);s.eq[1].design(2,750,-controls.target(2)*.12,.55,rate*Oversampler::factor,duration);s.eq[2].design(4,3000,controls.target(3),.7,rate*Oversampler::factor,duration);}
            if(kind==2)s.eq[0].design(2,1000,3,.8,rate*Oversampler::factor,duration);
            if(kind==5)s.eq[0].design(4,2200,controls.target(2)*.06,.7,rate*Oversampler::factor,duration);
        }
    }
    int kind;Controls controls;double rate=48000;Oversampler oversampler;std::array<State,2> states{};
};
}
std::vector<EffectDescriptor> driveDescriptors() {
    std::vector<EffectDescriptor> result;
    auto add=[&](int k,const char* id,const char* name,std::vector<ParameterDescriptor> p){result.push_back({id,name,"Drive",1,p,[k,p](const ParameterBank& b,SlotId s){return std::make_unique<Drive>(k,b,s,p);}});};
    add(0,"asymmetric_drive","Mid-Boost Asymmetric Drive",{{"drive_db","Drive","dB",0,0,36,8},{"low_cut_hz","Tightening","Hz",1,40,300,100},{"tone_hz","Tone","Hz",2,800,10000,4800},{"output_db","Output","dB",3,-30,18,-8}});
    add(1,"oxford_distortion","Oxford Contour Distortion",{{"drive_db","Gain","dB",0,0,42,18},{"bass_db","Bass","dB",1,-12,12,0},{"contour_percent","Contour","%",2,0,100,35},{"treble_db","Treble","dB",3,-12,12,1},{"output_db","Output","dB",4,-36,6,-12}});
    add(2,"hard_distortion","Op-Amp Hard Distortion",{{"drive_db","Drive","dB",0,0,48,16},{"low_cut_hz","Low Cut","Hz",1,30,300,70},{"filter_hz","Post Filter","Hz",2,500,10000,3300},{"output_db","Output","dB",3,-36,6,-12}});
    add(3,"sustain_fuzz","Sustaining Fuzz",{{"drive_db","Drive","dB",0,0,48,20},{"compression_percent","Compression","%",1,0,100,35},{"tone_hz","Tone","Hz",2,500,9000,2700},{"output_db","Output","dB",3,-36,6,-15}});
    add(4,"bias_fuzz","Controlled Bias Fuzz",{{"drive_db","Drive","dB",0,0,48,20},{"gate_db","Gate","dBFS",1,-80,-18,-65},{"compression_percent","Compression","%",2,0,100,35},{"bias_percent","Bias","%",3,-80,80,10},{"stability_percent","Stability","%",4,0,100,100},{"output_db","Output","dB",5,-36,6,-15}});
    add(5,"tight_boost","Tight Boost",{{"drive_db","Drive","dB",0,0,12,1},{"low_cut_hz","Low Cut","Hz",1,40,350,140},{"attack_percent","Attack","%",2,0,100,35},{"output_db","Output","dB",3,-18,24,6}});
    return result;
}
}
