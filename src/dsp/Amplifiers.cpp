#include "Amplifiers.h"
#include "TonePrimitives.h"
#include <memory>
namespace guitarrack {
namespace {
using namespace tone;
struct AmpSettings {
    double gain=6,bass=0,mid=0,treble=0,master=35,presence=40,output=-12,sag=15,lowCut=70,resonance=20,lead=18,bright=0,cut=35;
};
AmpSettings settings(const std::array<float,16>& v) noexcept {return {v[0],v[1],v[2],v[3],v[4],v[5],v[6],v[7],v[8],v[9],v[10],v[11],v[12]};}
// Derived controls are shared by both channels and all eight internal samples.
// This preserves the equations while avoiding repeated pow/sqrt in the model.
struct AmpFrame {
    AmpSettings settings;
    double gain=1,lead=1,power=1,inverseRootPower=1,cutHz=9000;
};
AmpFrame frame(const AmpSettings& s,int family) noexcept {
    const double power=.55+s.master*.045;
    return {s,db(s.gain),family==5?db(s.lead):1,power,1/std::sqrt(power),family==2?9000*std::pow(.18,s.cut*.01):9000};
}
// Eight original stage networks. These borrow musical roles, not measured circuits.
struct AmpNetwork {
    struct Channel {std::array<Pole,12> p{};std::array<Biquad,5> tone{};double envelope=0,dc=0,previous=0;};
    std::array<Channel,2> channels{};double rate=192000;int family=0;
    double attack=0,release=0,dcPole=0;
    void prepare(double internalRate) noexcept {
        rate=internalRate;attack=std::exp(-1/(rate*.004));release=std::exp(-1/(rate*.10));dcPole=std::exp(-tau*15/rate);
    }
    void configure(const AmpSettings& s,int duration) noexcept {
        for(auto& c:channels) {
            const double bass=family==2?s.bass-s.treble*.22:s.bass;
            const double treble=family==2?s.treble-s.bass*.18:s.treble;
            c.tone[0].design(3,family==5?180:family==1?120:180,bass,.7,rate,duration);
            c.tone[1].design(2,family==3||family==4?850:family==1?450:750,s.mid,.65,rate,duration);
            c.tone[2].design(4,family==2?2100:2800,treble,.7,rate,duration);
            c.tone[3].design(4,3300,family==2?0:(s.presence-40)*.10+s.bright*.04,.7,rate,duration);
            c.tone[4].design(3,95,s.resonance*.07,.7,rate,duration);
        }
    }
    double tick(unsigned channel,double x,const AmpFrame& f) noexcept {
        const auto& s=f.settings;auto& c=channels[channel];auto& p=c.p;
        const double pole=std::abs(x)>c.envelope?attack:release;
        c.envelope=clean(pole*c.envelope+(1-pole)*std::abs(x));
        const double sag=1/(1+s.sag*.03*c.envelope),g=f.gain;
        auto stack=[&](double y){for(unsigned i=0;i<3;++i)y=c.tone[i].tick(y);return y;};
        auto triode=[](double y,double bias){return std::tanh(y+bias)-std::tanh(bias);};
        double y=0;
        switch(family) {
        case 0: // High-headroom rail, no serial preamp clipping; optional bright.
            x=p[0].high(x,28,rate);y=stack(x)*g;
            y=2.4*std::tanh(y/2.4);break;
        case 1: // Tone before the soft first breakup, volume-dependent second stage.
            x=stack(p[0].high(x,42,rate));
            y=triode(x*g*.7,.10);y=p[1].low(y,11000,rate);
            y=triode(y*(1+.012*s.gain),-.04);break;
        case 2: // Interacting bright stack, cathode-like asymmetric pair, late Cut.
            x=p[0].high(x,60,rate);y=triode(x*g*.8,.18);
            y=stack(y);y=triode(p[1].high(y,95,rate)*1.8,-.12);break;
        case 3: // Crunch: two gain stages, mid focus after the first, late stack.
            x=p[0].high(x,80,rate);y=triode(x*g*.55,.07);
            y=p[1].high(y,130,rate);y=std::tanh(y*2.4);y=stack(y);break;
        case 4: // Hot rod: extra filtered asymmetric stage, controlled interstage bass.
            x=p[0].high(x,std::max(80.,s.lowCut),rate);y=triode(x*g*.45,.16);
            y=triode(p[1].high(p[2].low(y,9000,rate),180,rate)*3.3,-.18);
            y=std::tanh(p[3].high(y,135,rate)*2.1);y=stack(y);break;
        case 5: // Mark role: early Bass/Treble changes what clips; separate Lead gain.
            x=stack(p[0].high(x,65,rate));y=triode(x*g*.55,.10);
            y=p[1].high(p[2].low(y,12000,rate),110,rate);
            y=triode(y*f.lead*.45,-.08);y=std::tanh(p[3].high(y,85,rate)*1.4);break;
        case 6: // Modern tight: repeated bass cuts and three opposite-bias stages.
            x=p[0].high(x,std::max(65.,s.lowCut),rate);y=triode(x*g*.5,.04);
            y=triode(p[1].high(p[2].low(y,10500,rate),180,rate)*3.8,-.09);
            y=std::tanh(p[3].high(y,150,rate)*3.0);y=stack(y);break;
        default: // Broad rectifier role: lower interstage cut; envelope-sensitive bass.
            x=p[0].high(x,45,rate);y=triode(x*g*.5,.22);
            {const double low=p[1].low(y,160,rate);y=y-low+low/(1+3*c.envelope);}
            y=triode(p[2].low(y,8200,rate)*3.2,-.25);y=std::tanh(p[3].high(y,65,rate)*2.2);y=stack(y);break;
        }
        // Master changes compression/harmonics and recovery; Output is after it.
        y=std::tanh(y*f.power*sag)*f.inverseRootPower;
        y=c.tone[3].tick(y);y=c.tone[4].tick(y);
        if(family==2)y=p[4].low(y,f.cutHz,rate);
        c.dc=clean(dcPole*(c.dc+y-c.previous));c.previous=y;
        return c.dc*.65;
    }
};
class Amp final:public EffectProcessor {
public:
    Amp(int f,const ParameterBank& b,SlotId slot,std::vector<ParameterDescriptor> p):controls(b,slot,std::move(p)){network.family=f;}
    bool prepare(double r,int chunk) override {if(!std::isfinite(r)||r<8000||r>768000||chunk<1)return false;
        rate=r;network.prepare(rate*Oversampler::factor);oversampler.prepare();reset();return true;}
    void reset() noexcept override {network.channels={};oversampler.channels={};controls.update(true,rate);configure(true);}
    void activate(const EffectProcessor* old) noexcept override {
        if(const auto* a=dynamic_cast<const Amp*>(old);a&&a->rate==rate&&a->network.family==network.family) {network.channels=a->network.channels;oversampler.channels=a->oversampler.channels;controls.ramps=a->controls.ramps;}
        controls.update(!old,rate);configure(!old);
    }
    int latencySamples() const noexcept override{return 16;}
    double tailSeconds() const noexcept override{return .5;}
    void process(float* l,float* r,int n) noexcept override {
        if(n<=0)return;
        controls.update(false,rate);configure(false);
        for(int i=0;i<n;++i) {const auto s=settings(controls.next());const auto f=frame(s,network.family);const double output=db(s.output);for(unsigned c=0;c<2;++c){float& x=c?r[i]:l[i];const double input=std::isfinite(x)?std::clamp(double(x),-1e3,1e3):0;x=static_cast<float>(oversampler.tick(c,input,[&](double y){return network.tick(c,y,f);})*output);}}
    }
private:
    void configure(bool immediate) noexcept {std::array<float,16> v{};for(unsigned j=0;j<16;++j)v[j]=controls.target(j);network.configure(settings(v),immediate?0:int(rate*.02*Oversampler::factor));}
    Controls controls;AmpNetwork network;Oversampler oversampler;double rate=48000;
};
std::vector<ParameterDescriptor> ampParameters(int f) {
    std::vector<ParameterDescriptor> p{{"gain_db",f==5?"Input Gain":"Gain","dB",0,0,f<=1?24.f:48.f,f<=1?3.f:18.f},
        {"bass_db","Bass","dB",1,-12,12,f==5?-4.f:0.f}};
    if(f!=2)p.push_back({"mid_db","Mid","dB",2,-12,12,0});
    p.push_back({"treble_db","Treble","dB",3,-12,12,0});p.push_back({"master_percent","Master","%",4,0,100,35});
    if(f!=2)p.push_back({"presence_percent","Presence","%",5,0,100,40});
    p.push_back({"output_db","Output","dB",6,-36,6,-12});
    if(f!=0)p.push_back({"sag_percent","Sag","%",7,0,100,f==6?5.f:f==7?45.f:20.f});
    if(f==4||f==6)p.push_back({"low_cut_hz","Tightness","Hz",8,50,250,120});
    if(f==6||f==7)p.push_back({"resonance_percent","Resonance","%",9,0,100,f==7?40.f:20.f});
    if(f==5)p.push_back({"lead_gain_db","Lead Gain","dB",10,0,36,18});
    if(f<=1)p.push_back({"bright_percent","Bright","%",11,0,100,0});
    if(f==2)p.push_back({"cut_percent","Cut","%",12,0,100,35});
    return p;
}
class DualAmp final:public EffectProcessor {
public:
    DualAmp(const ParameterBank& b,SlotId s,std::vector<ParameterDescriptor> p):controls(b,s,std::move(p)){}
    bool prepare(double r,int chunk) override {
        if(!std::isfinite(r)||r<8000||r>768000||chunk<1)return false;
        rate=r;oversampler.prepare();for(auto& branch:networks)for(unsigned f=0;f<8;++f){branch[f].family=int(f);branch[f].prepare(r*Oversampler::factor);}
        reset();return true;
    }
    void reset() noexcept override {
        for(auto& branch:networks)for(auto& net:branch)net.channels={};
        oversampler.channels={};controls.update(true,rate);
        for(unsigned b=0;b<2;++b){current[b]=previous[b]=int(controls.target(b?4:0));fade[b]=1;}
        configure(true);
    }
    void activate(const EffectProcessor* old) noexcept override {
        if(auto* d=dynamic_cast<const DualAmp*>(old);d&&d->rate==rate){controls.ramps=d->controls.ramps;current=d->current;previous=d->previous;fade=d->fade;oversampler.channels=d->oversampler.channels;
            for(unsigned b=0;b<2;++b){networks[b][current[b]].channels=d->networks[b][current[b]].channels;if(previous[b]!=current[b])networks[b][previous[b]].channels=d->networks[b][previous[b]].channels;}}
        controls.update(!old,rate);configure(!old);
    }
    int latencySamples() const noexcept override{return 16;}
    double tailSeconds() const noexcept override{return .5;}
    void process(float* l,float* r,int count) noexcept override {
        if(count<=0)return;
        controls.update(false,rate);configure(false);
        for(unsigned b=0;b<2;++b){const int desired=int(std::round(controls.target(b?4:0)));if(fade[b]>=1&&desired!=current[b]){previous[b]=current[b];current[b]=desired;fade[b]=0;}}
        for(int i=0;i<count;++i){const auto v=controls.next();
            std::array<AmpFrame,2> activeFrames,priorFrames;
            for(unsigned b=0;b<2;++b){activeFrames[b]=frame(branchSettings(v,b,current[b]),current[b]);if(fade[b]<1)priorFrames[b]=frame(branchSettings(v,b,previous[b]),previous[b]);}
            const std::array<double,2> levels{db(v[3]),db(v[7])};const double output=db(v[13]);
            for(unsigned c=0;c<2;++c){float& x=c?r[i]:l[i];const double input=std::isfinite(x)?x:0;
                x=float(oversampler.tick(c,input,[&](double y){std::array<double,2> branch{};
                    for(unsigned b=0;b<2;++b){branch[b]=networks[b][current[b]].tick(c,y,activeFrames[b]);
                        if(fade[b]<1){branch[b]=branch[b]*fade[b]+networks[b][previous[b]].tick(c,y,priorFrames[b])*(1-fade[b]);}
                        branch[b]*=levels[b];}
                    return branch[0]*(1-v[8]*.01)+branch[1]*v[8]*.01;
                })*output);}
            for(auto& f:fade)f=std::min(1.,f+1/(rate*.02));
        }
    }
private:
    AmpSettings branchSettings(const std::array<float,16>& v,unsigned b,int family)const noexcept {
        AmpSettings s;s.gain=v[b?5:1];s.master=v[b?6:2];s.bass=v[9];s.mid=v[10];s.treble=v[11];s.presence=v[12];s.sag=family==0?0:family==6?5:family==7?45:20;s.lowCut=120;s.resonance=family==7?40:20;s.lead=18;return s;
    }
    void configure(bool immediate) noexcept {
        std::array<float,16> v{};for(unsigned j=0;j<16;++j)v[j]=controls.target(j);
        for(unsigned b=0;b<2;++b)for(unsigned f=0;f<8;++f)networks[b][f].configure(branchSettings(v,b,int(f)),immediate?0:int(rate*.02*Oversampler::factor));
    }
    Controls controls;double rate=48000;Oversampler oversampler;std::array<std::array<AmpNetwork,8>,2> networks{};std::array<int,2> current{},previous{};std::array<double,2> fade{1,1};
};

}
std::vector<EffectDescriptor> amplifierDescriptors() {
    const std::array<const char*,8> ids{"amp_solid_clean","amp_us_combo","amp_brit_chime","amp_brit_crunch","amp_brit_hotrod","amp_mark","amp_modern_tight","amp_recto"};
    const std::array<const char*,8> names{"Solid-State Clean","US Clean Combo","British Chime","British Crunch","British Hot-Rod","Mark High Gain","Modern Tight","Recto Heavy"};
    std::vector<EffectDescriptor> result;
    for(int f=0;f<8;++f){auto p=ampParameters(f);result.push_back({ids[f],names[f],"Amp",1,p,[f,p](const ParameterBank& b,SlotId s){return std::make_unique<Amp>(f,b,s,p);}});}
    return result;
}
EffectDescriptor dualAmpDescriptor() {
    const std::vector<std::string> models{"Solid-State Clean","US Clean Combo","British Chime","British Crunch","British Hot-Rod","Mark High Gain","Modern Tight","Recto Heavy"};
    std::vector<ParameterDescriptor> p{{"model_a","Amp A","",0,0,7,6,20,models},{"gain_a_db","Gain A","dB",1,0,48,18},{"master_a_percent","Master A","%",2,0,100,35},{"level_a_db","Level A","dB",3,-24,6,0},
        {"model_b","Amp B","",4,0,7,2,20,models},{"gain_b_db","Gain B","dB",5,0,48,8},{"master_b_percent","Master B","%",6,0,100,35},{"level_b_db","Level B","dB",7,-24,6,0},
        {"blend_percent","Blend B","%",8,0,100,30},{"bass_db","Shared Bass","dB",9,-12,12,0},{"mid_db","Shared Mid","dB",10,-12,12,0},{"treble_db","Shared Treble","dB",11,-12,12,0},{"presence_percent","Presence","%",12,0,100,40},{"output_db","Output","dB",13,-36,6,-12}};
    return {"dual_amp","Dual Amp Blend","Amp",1,p,[p](const ParameterBank& b,SlotId s){return std::make_unique<DualAmp>(b,s,p);}};
}

}
