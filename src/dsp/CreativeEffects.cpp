#include "CreativeEffects.h"
#include "TonePrimitives.h"
namespace guitarrack {
namespace {
using namespace tone;
enum class Kind {wah,envelope,phaser,flanger,swell,hold};
class Creative final:public EffectProcessor {
public:
    Creative(Kind k,const ParameterBank& b,SlotId s,std::vector<ParameterDescriptor> p):kind(k),controls(b,s,std::move(p)){}
    bool prepare(double r,int chunk) override {
        if(!std::isfinite(r)||r<8000||r>768000||chunk<1)return false;
        rate=r;
        if(kind==Kind::flanger||kind==Kind::hold)for(auto& ring:rings)ring.assign(std::size_t(std::ceil(rate*(kind==Kind::hold?.5:.025)))+4,0);
        reset();return true;
    }
    void reset() noexcept override {for(auto& r:rings)std::fill(r.begin(),r.end(),0);states={};phase=0;envelope=0;write=0;holdPosition=holdStart=0;held=false;captureArmed=true;holdSamples=0;gain=0;controls.update(true,rate);ingress.reset(1);captureMix.reset(0);}
    void activate(const EffectProcessor* old) noexcept override {
        if(auto* e=dynamic_cast<const Creative*>(old);e&&e->rate==rate&&e->kind==kind){states=e->states;phase=e->phase;envelope=e->envelope;gain=e->gain;controls.ramps=e->controls.ramps;}
        controls.update(!old,rate);ingress.reset(0);ingress.aim(1,int(rate*.02)); // Fresh long rings; never inherit captured audio.
    }
    double tailSeconds() const noexcept override {return kind==Kind::hold?120:kind==Kind::flanger?2:1;}
    void process(float* l,float* r,int count) noexcept override {
        if(count<=0)return;
        controls.update(false,rate);
        for(int i=0;i<count;++i){const auto v=controls.next();const double inputFade=ingress.next();const std::array<double,2> dry{std::isfinite(l[i])?l[i]:0,std::isfinite(r[i])?r[i]:0};std::array<double,2> wet=dry;double mix=1,level=1;
            const double peak=std::max(std::abs(dry[0]),std::abs(dry[1]));
            if(kind==Kind::wah||kind==Kind::envelope){
                double position=v[0]*.01;
                if(kind==Kind::envelope){const double pole=std::exp(-1/(rate*(peak>envelope?v[4]:v[5])*.001));envelope=clean(pole*envelope+(1-pole)*peak);position=std::clamp(v[0]*.01+(1-v[0]*.01)*envelope*db(v[6]),0.,1.);if(controls.target(7)>.5)position=1-position;}
                const double hz=std::clamp(v[1]*std::pow(v[2]/v[1],position),40.,rate*.35);
                const double g=std::tan(std::numbers::pi*hz/rate),k=1/v[3],a=1/(1+g*(g+k));
                for(unsigned c=0;c<2;++c){auto& s=states[c];const double d=dry[c]-s.ic2,b=a*s.ic1+g*a*d,low=s.ic2+g*a*s.ic1+g*g*a*d;s.ic1=clean(2*b-s.ic1);s.ic2=clean(2*low-s.ic2);wet[c]=b*k;}
                level=db(v[kind==Kind::wah?4:8]);mix=v[kind==Kind::wah?5:9]*.01;
            }else if(kind==Kind::phaser){
                phase+=v[0]/rate;phase-=std::floor(phase);
                for(unsigned c=0;c<2;++c){auto& s=states[c];const double hz=600*std::pow(4.,v[1]*.01*std::sin(tau*(phase+c*v[4]/360)));
                    const double t=std::tan(std::numbers::pi*std::min(hz,rate*.35)/rate),a=(1-t)/(1+t);double y=dry[c]+v[2]*.01*s.feedback,four=0;
                    for(unsigned j=0;j<6;++j){const double out=-a*y+s.ap[j];s.ap[j]=clean(y+a*out);y=out;if(j==3)four=y;}
                    y=four+(y-four)*v[5];
                    s.feedback=clean(y);wet[c]=y;
                }mix=v[3]*.01;level=db(v[6]);
            }else if(kind==Kind::flanger){
                phase+=v[0]/rate;phase-=std::floor(phase);
                for(unsigned c=0;c<2;++c){const double ms=std::clamp(double(v[2])+.003*v[1]*10*std::sin(tau*(phase+c*v[5]/360)),.15,18.);
                    wet[c]=read(c,ms*.001*rate);rings[c][write]=float(clean(dry[c]*inputFade+wet[c]*v[3]*.01));}
                mix=v[4]*.01;level=db(v[6]);if(++write==rings[0].size())write=0;
            }else if(kind==Kind::swell){
                const double pole=std::exp(-1/(rate*(peak>envelope?.0008:.03)));envelope=clean(pole*envelope+(1-pole)*peak);
                const bool sounding=envelope>db(v[2]);const double target=sounding?1:0;const double gainPole=std::exp(-1/(rate*(sounding?v[0]:v[1])*.001));gain=clean(gainPole*gain+(1-gainPole)*target);
                for(unsigned c=0;c<2;++c)wet[c]=dry[c]*gain;
                level=db(v[3]);
            }else { // Freeze/hold loops a captured 500-ms history, with 25-ms overlap seam.
                const bool capture=controls.target(0)>.5;
                if(!capture) captureArmed=true;
                if(capture&&captureArmed&&!held){captureArmed=false;held=true;holdSamples=0;holdStart=write;holdPosition=0;captureMix.aim(1,int(rate*.04));}
                // Bound the declared tail: 119.96 seconds held plus a 40-ms release.
                if(held&&++holdSamples>=std::size_t(rate*119.96)){held=false;captureMix.aim(0,int(rate*.04));}
                if(!capture&&held){held=false;captureMix.aim(0,int(rate*.04));}
                const double amount=captureMix.next();const std::size_t size=rings[0].size(),seam=std::max<std::size_t>(1,std::size_t(rate*.025));
                for(unsigned c=0;c<2;++c){double frozen=rings[c][(holdStart+holdPosition)%size];if(holdPosition>=size-seam){double weight=double(holdPosition-(size-seam))/seam;frozen=(1-weight)*frozen+weight*rings[c][(holdStart+holdPosition-(size-seam))%size];}
                    wet[c]=(1-amount)*dry[c]+amount*frozen;
                    if(!held&&amount==0)rings[c][write]=float(dry[c]*inputFade);
                }
                if(!held&&amount==0){if(++write==size)write=0;holdPosition=write;}
                else {if(++holdPosition==size)holdPosition=seam;}
                mix=v[1]*.01;level=db(v[2]);
            }
            l[i]=float(clean(((1-mix)*dry[0]+mix*wet[0])*level));r[i]=float(clean(((1-mix)*dry[1]+mix*wet[1])*level));
        }
    }
private:
    float read(unsigned c,double delay) const noexcept {const auto size=rings[c].size();double at=double(write)-std::clamp(delay,1.,double(size-2));if(at<0)at+=size;if(at>=size)at=0;const auto a=std::size_t(at),b=a+1==size?0:a+1;double f=at-a;return float((1-f)*rings[c][a]+f*rings[c][b]);}
    struct State {double ic1=0,ic2=0,feedback=0;std::array<double,6> ap{};};
    Kind kind;Controls controls;double rate=48000,phase=0,envelope=0,gain=0;std::array<State,2> states{};std::array<std::vector<float>,2> rings;std::size_t write=0,holdPosition=0,holdStart=0;std::size_t holdSamples=0;bool held=false,captureArmed=true;LinearRamp ingress,captureMix;
};
}
std::vector<EffectDescriptor> creativeDescriptors(){
    std::vector<EffectDescriptor> result;
    auto add=[&](Kind k,const char* id,const char* name,std::vector<ParameterDescriptor> p){result.push_back({id,name,k==Kind::wah||k==Kind::envelope?"Filter":"Modulation",1,p,[k,p](const ParameterBank& b,SlotId s){return std::make_unique<Creative>(k,b,s,p);}});};
    add(Kind::wah,"wah","Manual Wah",{{"position_percent","Position","%",0,0,100,50},{"heel_hz","Heel","Hz",1,150,1000,350},{"toe_hz","Toe","Hz",2,1100,4000,2400},{"q","Q","Q",3,.5,8,2},{"output_db","Output","dB",4,-24,6,0},{"mix_percent","Mix","%",5,0,100,100}});
    add(Kind::envelope,"envelope_wah","Envelope Wah",{{"position_percent","Base Position","%",0,0,100,0},{"heel_hz","Heel","Hz",1,150,1000,350},{"toe_hz","Toe","Hz",2,1100,4000,2400},{"q","Q","Q",3,.5,8,2},{"attack_ms","Attack","ms",4,1,100,8},{"release_ms","Release","ms",5,10,1000,160},{"sensitivity_db","Sensitivity","dB",6,0,36,18},{"direction","Direction","",7,0,1,0,20,{"Up","Down"}},{"output_db","Output","dB",8,-24,6,0},{"mix_percent","Mix","%",9,0,100,100}});
    add(Kind::phaser,"phaser","Phaser",{{"rate_hz","Rate","Hz",0,.05,10,.6},{"depth_percent","Depth","%",1,0,100,60},{"feedback_percent","Feedback","%",2,-80,80,15},{"mix_percent","Mix","%",3,0,100,50},{"stereo_degrees","Stereo Phase","deg",4,0,180,0},{"stages","Stages","",5,0,1,0,20,{"4","6"}},{"output_db","Output","dB",6,-24,6,0}});
    add(Kind::flanger,"flanger","Flanger",{{"rate_hz","Rate","Hz",0,.05,10,.25},{"depth_percent","Depth","%",1,0,100,40},{"manual_ms","Manual","ms",2,.5,10,3},{"feedback_percent","Feedback","%",3,-85,85,30},{"mix_percent","Mix","%",4,0,100,35},{"stereo_degrees","Stereo Phase","deg",5,0,180,90},{"output_db","Output","dB",6,-24,6,0}});
    add(Kind::swell,"swell","Auto Swell / Volume",{{"attack_ms","Swell","ms",0,5,2000,350},{"release_ms","Release","ms",1,10,1000,120},{"threshold_db","Threshold","dBFS",2,-90,-12,-60},{"output_db","Output","dB",3,-60,6,0}});
    add(Kind::hold,"audio_hold","Audio Freeze / Hold",{{"hold","Capture Hold","",0,0,1,0,20,{"Off","Hold"}},{"mix_percent","Mix","%",1,0,100,60},{"output_db","Output","dB",2,-24,6,-6}});
    return result;
}
}
