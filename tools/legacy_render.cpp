// Off-line compatibility capture; no MonitorEngine protection conceals DSP errors.
#include "state/Presets.h"
#include "core/RackEngine.h"
#include "core/LinearRamp.h"
#include "dsp/Cabinet.h"
#include "HeapAudit.h"
#include <cmath>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <numbers>
using namespace guitarrack;
int main(int argc,char** argv) {
    if(argc!=3) { std::cerr<<"Usage: legacy_render capture|check fixture.txt\n"; return 2; }
    const bool capture=std::string(argv[1])=="capture";
    std::ofstream out; std::ifstream in;
    if(capture) out.open(argv[2]); else in.open(argv[2]);
    if(capture?!out:!in) return 2;
    out<<std::setprecision(10);
    auto registry=EffectRegistry::builtIns(); unsigned errors=0; double maxError=0;
    auto metric=[&](double value) {
        if(capture) out<<value<<'\n';
        else { double expected=0; if(!(in>>expected)) {++errors;return;}
            const double error=std::abs(expected-value); maxError=std::max(error,maxError);
            // Float output, double reductions: allow compiler/FMA/libm variation,
            // well below -80 dBFS for ordinary renders; catches audible changes.
            if(!std::isfinite(value)||error>2e-6+2e-5*std::abs(expected)) ++errors;
        }
    };
    auto tones=factoryPresets(registry);
    // Limit to the frozen 45 identities when new factories are appended.
    for(double rate:{44100.,48000.,96000.}) for(const auto& tone:tones) {
        const bool legacy=tone.id.starts_with("00000000-")||tone.id.starts_with("50000000-");
        if(!legacy) continue;
        for(unsigned signal=0;signal<5;++signal) {
            ParameterBank bank;
            std::vector<std::unique_ptr<EffectProcessor>> processors;
            std::vector<StereoDelay> bypassDelays;
            std::vector<LinearRamp> bypassRamps;
            unsigned latency=0;
            for(const auto& id:tone.rack.order) {
                const auto& instance=*tone.rack.find(id);
                bank.setValues(instance.slot,tone.controls.values[instance.slot]);
                auto processor=registry.find(instance.effectType,instance.effectVersion)->factory(bank,instance.slot);
                if(auto* c=dynamic_cast<CabinetProcessor*>(processor.get())) c->setImpulse(decodeIR(syntheticIRBytes()));
                if(!processor->prepare(rate,256))return 3;
                processor->activate(nullptr);
                StereoDelay dry;dry.length=processor->latencySamples();latency+=dry.length;bypassDelays.push_back(dry);
                LinearRamp mix;mix.reset(1);bypassRamps.push_back(mix);processors.push_back(std::move(processor));
            }
            StereoDelay padding; padding.length=128-latency;
            StereoDelay rackDry; rackDry.length=128;
            LinearRamp rackMix; rackMix.reset(1);
            std::array<float,256> l{},r{}; const int total=static_cast<int>(rate*1.5);
            for(int at=0;at<total;at+=256) {
                const int n=std::min(256,total-at);
                for(int j=0;j<n;++j) {
                    const double t=(at+j)/rate;
                    double x=0;
                    if(signal==0) x=at+j==0?.25:0;
                    if(signal==1) x=.14*std::exp(-4*t)*(std::sin(2*std::numbers::pi*110*t)+.4*std::sin(2*std::numbers::pi*330*t));
                    if(signal==2||signal==4) for(double f:{82.4069,123.4708,164.8138,207.6523}) x+=.025*std::sin(2*std::numbers::pi*f*t);
                    l[j]=static_cast<float>(x); r[j]=static_cast<float>(x*.83);
                }
                if(signal==4 && at==static_cast<int>(rate*.5)/256*256) {
                    rackMix.aim(0,int(rate*.02));
                    for(auto& mix:bypassRamps)mix.aim(0,int(rate*.02));
                    for(const auto& i:tone.rack.instances) bank.setValue(i.slot,0,registry.find(i.effectType,i.effectVersion)->parameters[0].normalize(registry.find(i.effectType,i.effectVersion)->parameters[0].defaultValue));
                }
                if(signal==4 && at==static_cast<int>(rate*.8)/256*256) {rackMix.aim(1,int(rate*.02));for(auto& mix:bypassRamps)mix.aim(1,int(rate*.02));}
                std::array<float,256> originalL=l,originalR=r,dryL{},dryR{};
                heapAudit::begin();
                for(std::size_t k=0;k<processors.size();++k) {
                    dryL=l;dryR=r;processors[k]->process(l.data(),r.data(),n);
                    for(int j=0;j<n;++j){bypassDelays[k].tick(dryL[j],dryR[j]);float mix=bypassRamps[k].next();l[j]=mix* l[j]+(1-mix)*dryL[j];r[j]=mix*r[j]+(1-mix)*dryR[j];}
                }
                for(int j=0;j<n;++j){padding.tick(l[j],r[j]);rackDry.tick(originalL[j],originalR[j]);float mix=rackMix.next();l[j]=mix*l[j]+(1-mix)*originalL[j];r[j]=mix*r[j]+(1-mix)*originalR[j];}
                if(!heapAudit::finish())return 4;
                for(auto* channel:{&l,&r}) {
                    double sum=0,squares=0,projection=0,peak=0;
                    for(int j=0;j<n;++j) { const double x=(*channel)[j]; sum+=x; squares+=x*x; projection+=x*std::sin(j*.73); peak=std::max(peak,std::abs(x)); }
                    metric(sum/n); metric(std::sqrt(squares/n)); metric(projection/n); metric(peak);
                }
            }

        }
    }
    if(!capture) {double extra;if(in>>extra) ++errors;}
    std::cout<<"Legacy render metrics: "<<errors<<" failures; maximum absolute error "<<maxError<<"\n";
    return errors?1:0;
}
