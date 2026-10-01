#include "CreativeEffects.h"
#include "TonePrimitives.h"
namespace guitarrack {
namespace {
using namespace tone;
struct Line {std::vector<float> data;std::size_t write=0;double low=0;double read()const noexcept{return data[write];}void push(double x)noexcept{data[write]=float(clean(x));if(++write==data.size())write=0;}void reset()noexcept{std::fill(data.begin(),data.end(),0);write=0;low=0;}};
// Distinct prepared networks: small 4-line room, diffused 6-line plate, 8-line
// Hadamard hall. Physical RT60 sets length-dependent feedback per line.
class Space final:public EffectProcessor {
public:
    Space(const ParameterBank& b,SlotId s,std::vector<ParameterDescriptor> p):controls(b,s,std::move(p)){}
    bool prepare(double r,int chunk) override {
        if(!std::isfinite(r)||r<8000||r>768000||chunk<1)return false;
        rate=r;
        for(unsigned model=0;model<3;++model)for(unsigned c=0;c<2;++c){auto& net=networks[model][c];const unsigned count=model==0?4:model==1?6:8;
            for(unsigned j=0;j<count;++j)net.lines[j].data.assign(std::size_t(rate*(model==0?.012+.0043*j:model==1?.022+.0059*j:.031+.0077*j))+c*17+1,0);
            for(unsigned j=0;j<4;++j)net.diffusion[j].data.assign(std::size_t(rate*(.0017+.0013*j))+(model*13+c*7)+1,0);
        }
        for(auto& p:predelay)p.assign(std::size_t(rate*.25)+2,0);
        reset();return true;
    }
    void reset() noexcept override {for(auto& models:networks)for(auto& n:models){for(auto& l:n.lines)l.reset();for(auto& l:n.diffusion)l.reset();n.tone={};}for(auto& p:predelay)std::fill(p.begin(),p.end(),0);write=0;controls.update(true,rate);ingress.reset(1);}
    void activate(const EffectProcessor* old) noexcept override {if(auto* s=dynamic_cast<const Space*>(old);s&&s->rate==rate)controls.ramps=s->controls.ramps;controls.update(!old,rate);ingress.reset(0);ingress.aim(1,int(rate*.02));}
    double tailSeconds() const noexcept override{return 40;}
    void process(float* l,float* r,int n) noexcept override {
        if(n<=0)return;
        controls.update(false,rate);
        for(int i=0;i<n;++i){const auto v=controls.next();const double injection=ingress.next();const double delay=std::clamp(v[2]*.001*rate,0.,double(predelay[0].size()-2));const std::array<double,2> dry{std::isfinite(l[i])?l[i]:0,std::isfinite(r[i])?r[i]:0};std::array<double,2> wet{};
            for(unsigned c=0;c<2;++c){predelay[c][write]=float(dry[c]*injection);double position=double(write)-delay;if(position<0)position+=predelay[c].size();if(position>=predelay[c].size())position=0;auto at=std::size_t(position),next=at+1==predelay[c].size()?0:at+1;double f=position-at;double excitation=(1-f)*predelay[c][at]+f*predelay[c][next];
                std::array<double,3> outputs{};
                // Run every prepared mode so mode changes retain tails without constructing.
                for(unsigned model=0;model<3;++model){auto& net=networks[model][c];const unsigned count=model==0?4:model==1?6:8;double x=excitation;
                    if(model!=0)for(auto& ap:net.diffusion){double z=ap.read();double y=z-.65*x;ap.push(x+.65*y);x=y;}
                    std::array<double,8> taps{};double sum=0;
                    for(unsigned j=0;j<count;++j){auto& line=net.lines[j];const double damping=std::exp(-tau*std::min(double(v[3]),rate*.4)/rate);line.low=clean((1-damping)*line.read()+damping*line.low);taps[j]=line.low;sum+=taps[j];}
                    if(model==2){ // Orthogonal 8x8 Walsh-Hadamard feedback; bounded energy.
                        for(unsigned width=1;width<8;width*=2)for(unsigned start=0;start<8;start+=2*width)for(unsigned j=0;j<width;++j){double a=taps[start+j],b=taps[start+j+width];taps[start+j]=a+b;taps[start+j+width]=a-b;}
                        for(auto& y:taps)y/=std::sqrt(8.);
                    }
                    for(unsigned j=0;j<count;++j){const double seconds=net.lines[j].data.size()/rate;const double feedback=std::pow(10.,-3*seconds/v[1]);const double returned=model==2?taps[j]:model==1?(2*sum/count-taps[j]):taps[j];net.lines[j].push(x*.15+feedback*returned);}
                    const double y=sum/count;outputs[model]=net.tone.low(y,v[4],rate);
                }
                double mode=v[0];wet[c]=mode<1?outputs[0]*(1-mode)+outputs[1]*mode:outputs[1]*(2-mode)+outputs[2]*(mode-1);
            }
            if(++write==predelay[0].size())write=0;
            double mix=v[5]*.01,level=db(v[6]);l[i]=float(clean(((1-mix)*dry[0]+mix*wet[0])*level));r[i]=float(clean(((1-mix)*dry[1]+mix*wet[1])*level));
        }
    }
private:
    struct Network {std::array<Line,8> lines;std::array<Line,4> diffusion;Pole tone;};
    Controls controls;double rate=48000;std::array<std::array<Network,2>,3> networks;std::array<std::vector<float>,2> predelay;std::size_t write=0;LinearRamp ingress;
};
}
EffectDescriptor spaceReverbDescriptor(){
    std::vector<ParameterDescriptor> p{{"mode","Space","",0,0,2,0,20,{"Room","Plate","Hall"}},{"decay_seconds","Decay RT60","s",1,.2,12,1.5},{"pre_delay_ms","Pre-delay","ms",2,0,250,12},{"damping_hz","Damping","Hz",3,500,18000,6000},{"tone_hz","Wet Tone","Hz",4,500,18000,9000},{"mix_percent","Mix","%",5,0,100,15},{"output_db","Output","dB",6,-24,6,0}};
    return {"space_reverb","Room / Plate / Hall","Reverb",1,p,[p](const ParameterBank& b,SlotId s){return std::make_unique<Space>(b,s,p);}};
}
}
