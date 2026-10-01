#include "core/SoundLibrary.h"
#include "core/RackEngine.h"
#include "core/MonitorEngine.h"
#include "HeapAudit.h"
#include <algorithm>
#include <atomic>
#include <cmath>
#include <iostream>
#include <numbers>
#include <set>
#include <thread>

using namespace guitarrack;
namespace {
int failures=0;
void check(bool ok,const char* label) {if(!ok){++failures;std::cerr<<"FAIL: "<<label<<'\n';}}
struct Effect {
    EffectRegistry registry=EffectRegistry::builtIns();
    ParameterBank bank;
    const EffectDescriptor* descriptor;
    std::unique_ptr<EffectProcessor> processor;
    std::array<float,512> l{},r{};
    bool heapFree=true;
    Effect(const char* type,unsigned version=0,double rate=48000)
        : descriptor(version?registry.find(type,version):registry.latest(type)) {
        for(const auto& p:descriptor->parameters) bank.setValue(0,p.bankIndex,p.normalize(p.defaultValue));
        processor=descriptor->factory(bank,0); check(processor->prepare(rate,512),"effect preparation");
        processor->activate(nullptr);
    }
    void set(const char* key,float value) {
        for(const auto& p:descriptor->parameters) if(p.key==key) {check(p.accepts(value),"physical control valid");bank.setValue(0,p.bankIndex,p.normalize(value));return;}
        check(false,"known control");
    }
    void block(int n) {heapAudit::begin();processor->process(l.data(),r.data(),n);heapFree &= heapAudit::finish();}
    std::vector<float> sine(double frequency,int samples=48000,float amplitude=.1f,bool right=false) {
        std::vector<float> output;output.reserve(samples);
        for(int start=0;start<samples;start+=127) {
            const int n=std::min(127,samples-start);
            for(int i=0;i<n;++i) {l[i]=static_cast<float>(amplitude*std::sin(2*std::numbers::pi*frequency*(start+i)/48000));r[i]=right?l[i]:0;}
            block(n);
            for(int i=0;i<n;++i) {check(std::isfinite(l[i])&&std::isfinite(r[i]),"finite stereo output");output.push_back(l[i]);}
        }
        return output;
    }
};
double amplitude(const std::vector<float>& signal,double frequency) {
    double re=0,im=0;
    for(std::size_t i=signal.size()/2;i<signal.size();++i) {
        const double phase=2*std::numbers::pi*frequency*i/48000;
        re+=signal[i]*std::cos(phase);im+=signal[i]*std::sin(phase);
    }
    return 4*std::hypot(re,im)/signal.size();
}
void coherentControls() {
    ParameterBank bank;
    std::atomic<bool> done{false};std::atomic<unsigned> reads{0};
    std::thread writer([&]{std::array<float,16> row{};for(unsigned j=0;j<20000;++j){row.fill(j%2?.25f:.75f);bank.setValues(0,row);}done=true;});
    bool coherent=true;std::array<float,16> row{};bool bypass=false;
    while(!done) if(bank.readSlot(0,row,bypass)) {for(float x:row)coherent &= x==row[0];++reads;}
    writer.join();check(coherent&&reads>0,"concurrent pedal voicings never expose a mixed control row");
    check(!bank.readSlot(rackSlots,row,bypass),"invalid snapshot slot rejected");
    auto invalid=row;invalid[3]=2;
    check(!bank.setValues(0,invalid),"out of range batch rejected before mutation");
    std::array<float,16> after{};bank.readSlot(0,after,bypass);check(after==row || after[0]==.25f,"writer leaves a complete row");
}
void signals() {
    Effect compressor("compressor");compressor.set("threshold_db",-20);compressor.set("ratio",4);compressor.set("makeup_db",0);compressor.set("mix_percent",100);compressor.processor->reset();
    for(int b=0;b<200;++b){compressor.l.fill(.4f);compressor.r.fill(0);compressor.block(512);}
    const float expected=.4f*std::pow(10.f,-(20*std::log10(.4f)+20)*.75f/20);
    check(std::abs(compressor.l.back()-expected)<.0001f&&compressor.r.back()==0,"compressor measured steady threshold/ratio and silent right");
    Effect tremolo("tremolo");tremolo.set("rate_hz",4);tremolo.set("depth_percent",100);tremolo.set("mix_percent",100);tremolo.processor->reset();
    float lo=1,hi=0;
    for(int b=0;b<100;++b){tremolo.l.fill(.1f);tremolo.r.fill(.1f);tremolo.block(512);for(float x:tremolo.l){lo=std::min(lo,x);hi=std::max(hi,x);}}
    check(lo<.0001f&&hi>.099f,"tremolo completes deep gain cycles at musical rate");
    Effect filter("filter");filter.set("cutoff_hz",500);filter.set("resonance_percent",0);filter.set("mix_percent",100);filter.processor->reset();
    auto filtered=filter.sine(4000);check(amplitude(filtered,4000)<.003,"filter attenuates above its physical cutoff");
    Effect pitch("pitch");pitch.set("semitones",12);pitch.set("mix_percent",100);pitch.processor->reset();
    auto shifted=pitch.sine(220,96000);
    double peak=0,frequency=0;
    for(double f=420;f<=460;f+=.5) if(const auto value=amplitude(shifted,f);value>peak){peak=value;frequency=f;}
    std::cout<<"Pitch peak: "<<frequency<<" Hz, amplitude "<<peak<<", input residual "<<amplitude(shifted,220)<<"\n";
    check(peak>.025&&std::abs(frequency-440)<8&&peak>4*amplitude(shifted,220),"granular octave moves the dominant fundamental near 440 Hz");
    Effect ring("ring_mod");ring.set("rate_hz",100);ring.set("mix_percent",100);ring.processor->reset();
    auto bells=ring.sine(220);
    check(amplitude(bells,120)>.04&&amplitude(bells,320)>.04&&amplitude(bells,220)<.001,"ring modulation creates sum and difference sidebands");
    Effect chorus("chorus");chorus.set("width_percent",100);chorus.set("depth_percent",80);chorus.set("mix_percent",50);chorus.processor->reset();
    chorus.sine(500,48000,.1f,true);
    double stereo=0;for(unsigned j=0;j<127;++j)stereo+=std::abs(chorus.l[j]-chorus.r[j]);
    check(stereo>.01,"chorus creates stereo motion from duplicated mono");
    for(auto* e:{&compressor,&tremolo,&filter,&pitch,&ring,&chorus})check(e->heapFree,"experimental processing has no heap operations");
}
void compatibilityAndLifecycle() {
    Effect old("delay",1),fresh("delay",2);
    auto a=old.sine(733),b=fresh.sine(733);
    check(a==b,"default Digital v2 preserves the version 1 signal exactly");
    for(const char* type:{"fuzz","amp","compressor","chorus","tremolo","filter","pitch","ring_mod","delay"}) {
        for(double rate:{8000.,44100.,48000.,96000.}) {
            Effect e(type,0,rate);
            e.block(0);
            for(const auto& p:e.descriptor->parameters) {
                if(!p.choices.empty())continue;
                if(p.key=="mix_percent")e.set(p.key.c_str(),0);
            }
            e.processor->reset();
            e.l.fill(.123f);e.r.fill(-.2f);e.block(37);
            check(e.l[0]==.123f&&e.r[0]==-.2f,"zero mix is independent exact dry for new processors");
            for(const auto& p:e.descriptor->parameters)e.set(p.key.c_str(),p.maximum);
            for(int j=0;j<12;++j){e.l.fill(.05f);e.r.fill(-.02f);e.block(j%2?511:1);}
            for(float x:e.l)check(std::isfinite(x)&&std::abs(x)<1e6f,"extreme controls remain bounded");
            auto next=e.descriptor->factory(e.bank,0);check(next->prepare(rate,512),"replacement prepares");
            heapAudit::begin();next->activate(e.processor.get());check(heapAudit::finish(),"activation inherits only fixed state");
            check(e.heapFree,"variable blocks and live control changes are heap free");
        }
    }
    RackEngine rack;check(rack.add("delay"),"new delay can be added");
    auto snapshot=rack.snapshot();const auto id=snapshot.requested.instances.front().id;
    check(snapshot.requested.instances.front().effectVersion==2,"new delay picks latest version");
    const auto* descriptor=rack.registry().find("delay",2);std::vector<float> values;
    for(const auto& p:descriptor->parameters)values.push_back(p.defaultValue);
    values[3]=.5f;check(!rack.setParameters(id,values),"fractional mode rejected");
    values[3]=2;check(rack.setParameters(id,values),"complete pedal voicing accepted");
    check(rack.snapshot().requestedGeneration==snapshot.requestedGeneration,"pedal voicing does not rebuild graph or replace identity");
}
void catalog() {
    const auto registry=EffectRegistry::builtIns();const auto pedals=effectPresets(registry);const auto racks=experimentalFactoryPresets(registry);
    check(pedals.size()==77&&racks.size()==40,"complete experimental library embedded");
    std::set<std::string> identities,names;
    for(const auto& p:pedals){check(validateEffectPreset(p,registry).empty(),"pedal recipe valid");check(identities.insert(p.id).second&&names.insert(p.name).second,"pedal identities and names unique");}
    for(const auto& t:racks) {
        check(validateTone(t,registry).empty(),"factory rack validates");check(identities.insert(t.id).second,"factory rack identity unique");
        RackEngine rack;MonitorControls controls;MonitorEngine monitor(controls,&rack);
        check(rack.loadTone(t)!=0&&monitor.prepare(48000),"full experimental rack prepares");
        std::array<float,512> in{},l{},r{};
        bool finite=true,heapFree=true;
        for(int b=0;b<32;++b) {
            for(unsigned i=0;i<512;++i)in[i]=b<16?static_cast<float>(.03*std::sin(2*std::numbers::pi*220*(b*512+i)/48000)):0;
            heapAudit::begin();monitor.process(in.data(),in.data(),l.data(),r.data(),512);heapFree &= heapAudit::finish();
            for(unsigned i=0;i<512;++i)finite &= std::isfinite(l[i])&&std::isfinite(r[i])&&std::abs(l[i])<=.98f&&std::abs(r[i])<=.98f;
        }
        check(finite&&heapFree,"every factory rack renders safely with callback allocation audit");
    }
}
}
int main(){coherentControls();signals();compatibilityAndLifecycle();catalog();if(!failures)std::cout<<"PASS: experimental signals, coherent voicings, compatibility, lifecycle, 77 pedals and 40 rendered racks\n";return failures?1:0;}
