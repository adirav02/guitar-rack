#include "CreativeEffects.h"
#include "TonePrimitives.h"
namespace guitarrack {
namespace {
using namespace tone;
class Echo final:public EffectProcessor {
public:
    Echo(const ParameterBank& b,SlotId s,std::vector<ParameterDescriptor> p):controls(b,s,std::move(p)){}
    bool prepare(double r,int chunk) override {if(!std::isfinite(r)||r<8000||r>768000||chunk<1)return false;
        rate=r;for(auto& ring:rings)ring.assign(std::size_t(std::ceil(rate*4))+4,0);reset();return true;}
    void reset() noexcept override {for(auto& ring:rings)std::fill(ring.begin(),ring.end(),0);filters={};write=0;phase=0;controls.update(true,rate);currentTime=targetTime=desiredTime();fade=1;ingress.reset(1);}
    void activate(const EffectProcessor* old) noexcept override {if(auto* e=dynamic_cast<const Echo*>(old);e&&e->rate==rate){controls.ramps=e->controls.ramps;currentTime=e->currentTime;targetTime=e->targetTime;fade=e->fade;phase=e->phase;filters=e->filters;}controls.update(!old,rate);ingress.reset(0);ingress.aim(1,int(rate*.02));}
    double tailSeconds() const noexcept override{return 120;}
    void process(float* l,float* r,int n) noexcept override {
        if(n<=0)return;
        controls.update(false,rate);const double desired=desiredTime();
        if(fade>=1&&std::abs(desired-currentTime)>.01){targetTime=desired;fade=0;}
        for(int i=0;i<n;++i){const auto v=controls.next();const double injection=ingress.next();phase+=.7/rate;phase-=std::floor(phase);const double wow=.001*v[7]*.01*rate*std::sin(tau*phase);std::array<double,2> wet{},dry{std::isfinite(l[i])?l[i]:0,std::isfinite(r[i])?r[i]:0};
            for(unsigned c=0;c<2;++c){
                auto heads=[&](double time){const double main=read(c,time+wow*std::clamp(double(v[4])-1.,0.,1.));const double multi=.6*main+.25*read(c,time*.75)+.15*read(c,time*.5);return main+(multi-main)*v[9];};
                const double delayed=(1-fade)*heads(currentTime)+fade*heads(targetTime);
                const double colored=filters[c].low(delayed,v[6],rate);
                const double mode=v[4];wet[c]=mode<1?delayed+(colored-delayed)*mode:colored+(std::tanh(colored*1.2)/1.2-colored)*(mode-1);
            }
            for(unsigned c=0;c<2;++c){const double feedback=wet[c]*(1-v[8])+wet[1-c]*v[8];rings[c][write]=float(clean(dry[c]*injection+feedback*v[5]*.01));}
            const double mix=v[10]*.01,level=db(v[11]);l[i]=float(clean(((1-mix)*dry[0]+mix*wet[0])*level));r[i]=float(clean(((1-mix)*dry[1]+mix*wet[1])*level));
            if(++write==rings[0].size())write=0;
            if(fade<1){fade=std::min(1.,fade+1/(rate*.1));if(fade==1)currentTime=targetTime;}
        }
    }
private:
    double desiredTime() const noexcept {const std::array<double,5> beats{1,.5,.75,.25,2};const unsigned subdivision=std::clamp(unsigned(std::round(controls.target(2))),0u,4u);const double ms=controls.target(3)>.5?60000/controls.target(1)*beats[subdivision]:controls.target(0);return std::clamp(ms*.001*rate,1.,rate*4);}
    double read(unsigned c,double delay) const noexcept {const auto size=rings[c].size();double position=double(write)-std::clamp(delay,1.,double(size-2));if(position<0)position+=size;if(position>=size)position=0;const auto a=std::size_t(position),b=a+1==size?0:a+1;const double f=position-a;return (1-f)*rings[c][a]+f*rings[c][b];}
    Controls controls;double rate=48000,currentTime=20160,targetTime=20160,fade=1,phase=0;std::array<std::vector<float>,2> rings;std::array<Pole,2> filters{};std::size_t write=0;LinearRamp ingress;
};
}
EffectDescriptor echoDescriptor(){
    std::vector<ParameterDescriptor> p{{"time_ms","Time","ms",0,1,4000,420},{"bpm","Tempo","BPM",1,40,240,120},{"subdivision","Subdivision","",2,0,4,0,20,{"Quarter","Eighth","Dotted eighth","Sixteenth","Half"}},{"clock","Clock","",3,0,1,0,20,{"Milliseconds","BPM"}},{"mode","Character","",4,0,2,0,20,{"Digital","Analog","Tape"}},{"feedback_percent","Feedback","%",5,0,70,30},{"color_hz","Color","Hz",6,500,16000,5000},{"wow_percent","Wow","%",7,0,100,0},{"stereo","Stereo","",8,0,1,0,20,{"Stereo","Ping-pong"}},{"heads","Heads","",9,0,1,0,20,{"Single","Three: 1 / 0.75 / 0.5"}},{"mix_percent","Mix","%",10,0,100,25},{"output_db","Output","dB",11,-24,6,0}};
    return {"echo","Tempo / Tape Echo","Delay",1,p,[p](const ParameterBank& b,SlotId s){return std::make_unique<Echo>(b,s,p);}};
}
}
