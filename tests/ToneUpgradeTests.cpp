#include "core/Effects.h"
#include "core/RackEngine.h"
#include "core/CabinetAssets.h"
#include "dsp/Amplifiers.h"
#include "HeapAudit.h"
#include <cmath>
#include <complex>
#include <algorithm>
#include <iostream>
#include <numbers>
using namespace guitarrack;
namespace {
int failures=0;void check(bool ok,const char* label){if(!ok){++failures;std::cerr<<"FAIL: "<<label<<'\n';}}
struct Effect {
    EffectRegistry registry=EffectRegistry::builtIns();ParameterBank bank;const EffectDescriptor* d;std::unique_ptr<EffectProcessor> p;double rate;
    Effect(const std::string& type,double r=48000):d(registry.latest(type)),rate(r){for(const auto& c:d->parameters)bank.setValue(0,c.bankIndex,c.normalize(c.defaultValue));p=d->factory(bank,0);check(p->prepare(r,512),"preparation succeeds");p->activate(nullptr);}
    void set(const std::string& key,float v){for(const auto& c:d->parameters)if(c.key==key){check(c.accepts(v),"test control in physical range");bank.setValue(0,c.bankIndex,c.normalize(v));return;}check(false,"test control exists");}
    std::vector<float> render(double f,double amplitude=.1,int seconds=1){std::vector<float> y(int(rate)*seconds);std::array<float,512> l{},r{};for(std::size_t at=0;at<y.size();at+=127){int n=std::min<std::size_t>(127,y.size()-at);for(int j=0;j<n;++j)l[j]=r[j]=float(amplitude*std::sin(2*std::numbers::pi*f*(at+j)/rate));heapAudit::begin();p->process(l.data(),r.data(),n);check(heapAudit::finish(),"process performs no heap operations");std::copy_n(l.begin(),n,y.begin()+at);}return y;}
};
double rms(const std::vector<float>& y){double sum=0;for(std::size_t i=y.size()/2;i<y.size();++i)sum+=double(y[i])*y[i];return std::sqrt(2*sum/y.size());}
double harmonic(const std::vector<float>& y,double f,double rate=48000){double re=0,im=0;for(std::size_t i=y.size()/2;i<y.size();++i){double a=2*std::numbers::pi*f*i/rate;re+=y[i]*std::cos(a);im+=y[i]*std::sin(a);}return 4*std::hypot(re,im)/y.size();}
void families(){
    std::vector<std::vector<float>> voices;
    for(const auto& d:amplifierDescriptors()){
        Effect amp(d.typeId);amp.set("gain_db",18);amp.set("output_db",-12);amp.p->reset();auto y=amp.render(211,.15);
        check(rms(y)>.005&&rms(y)<.6,"amp has useful conservative level before protection");
        amp.set("output_db",-18);amp.p->reset();auto quiet=amp.render(211,.15);
        check(std::abs(rms(quiet)/rms(y)-std::pow(10.,-6./20))<2e-5,"Output independently scales loudness");
        amp.set("output_db",-12);amp.set("master_percent",90);amp.p->reset();auto power=amp.render(211,.15);
        const double h=harmonic(y,633)/harmonic(y,211),h2=harmonic(power,633)/harmonic(power,211);
        check(std::abs(h-h2)>.001,"Master changes harmonic behavior independently from Output");
        const float measuredRms=float(rms(y));for(auto& x:y)x/=measuredRms;voices.push_back(std::move(y));
    }
    double minimumDifference=1;
    for(std::size_t a=0;a<voices.size();++a)for(std::size_t b=a+1;b<voices.size();++b){double e=0;for(std::size_t j=24000;j<48000;++j)e+=std::pow(voices[a][j]-voices[b][j],2);minimumDifference=std::min(minimumDifference,std::sqrt(e/24000));}
    std::cout<<"Matched-RMS amp minimum pair waveform difference: "<<minimumDifference<<'\n';check(minimumDifference>.03,"eight networks demonstrably differ at matched level");
    Effect mark("amp_mark");mark.set("bass_db",-10);mark.p->reset();auto tight=mark.render(82,.25);mark.set("bass_db",10);mark.p->reset();auto full=mark.render(82,.25);
    check(std::abs(harmonic(tight,246)/harmonic(tight,82)-harmonic(full,246)/harmonic(full,82))>.02,"early Mark Bass changes subsequent harmonic generation");
}
void responseAndCleanup(){
    // Characterize actual signal behavior, independently of descriptor counts.
    for(const auto& descriptor:amplifierDescriptors()) {
        Effect amp(descriptor.typeId);amp.set("gain_db",0);amp.p->reset();
        const auto lowGain=amp.render(211,.1);
        amp.set("gain_db",descriptor.parameters[0].maximum);amp.p->reset();
        const auto highGain=amp.render(211,.1);
        const double lowH3=harmonic(lowGain,633)/harmonic(lowGain,211);
        const double highH3=harmonic(highGain,633)/harmonic(highGain,211);
        check(std::abs(highH3-lowH3)>.002,"Gain changes harmonic generation in each amp family");
        amp.set("gain_db",0);
        std::array<double,3> response{};unsigned at=0;
        for(double hz:{82.,750.,4500.}) {
            amp.p->reset();const auto y=amp.render(hz,.005);
            response[at++]=harmonic(y,hz)/.005;
            check(std::isfinite(response[at-1])&&response[at-1]>1e-5&&response[at-1]<16,"small-signal amp response has useful bounded passband");
        }
        // Excite and then remove input: no persistent oscillator or DC tail.
        amp.p->reset();std::array<float,512> l{},r{};double peak=0,late=0;
        for(int block=0;block<94;++block) {
            l.fill(0);r.fill(0);if(block==0)l[0]=r[0]=.25;
            heapAudit::begin();amp.p->process(l.data(),r.data(),512);
            check(heapAudit::finish(),"amp transient and silence allocate nothing");
            for(float x:l){check(std::isfinite(x),"amp transient finite");peak=std::max(peak,std::abs(double(x)));if(block>=47)late=std::max(late,std::abs(double(x)));}
        }
        check(peak>1e-5&&late<1e-5,"amp transient settles to silence without protection clamp");
        std::cout<<"amp-response,"<<descriptor.typeId<<','<<response[0]<<','<<response[1]<<','<<response[2]<<','<<lowH3<<','<<highH3<<','<<peak<<','<<late<<'\n';
    }
    Effect combo("amp_us_combo");combo.set("gain_db",18);combo.p->reset();auto hard=combo.render(211,.15);
    combo.p->reset();auto rolledBack=combo.render(211,.01);
    check(harmonic(rolledBack,633)/harmonic(rolledBack,211)<harmonic(hard,633)/harmonic(hard,211)*.25,"US combo cleans up with reduced guitar input amplitude");
}
void eqGate(){
    Effect graphic("graphic_eq");graphic.set("750_hz_db",6);graphic.p->reset();auto peak=graphic.render(750,.01);check(std::abs(harmonic(peak,750)/.01-std::pow(10.,6./20))<.005,"graphic EQ measured center boost");
    Effect gate("noise_gate");gate.set("threshold_db",-30);gate.set("hold_ms",20);gate.set("release_ms",30);gate.p->reset();
    std::array<float,512> l{},r{};for(int b=0;b<20;++b){l.fill(.1);r.fill(.1);gate.p->process(l.data(),r.data(),512);}check(l.back()>.099,"gate opens and preserves above-threshold input");
    for(int b=0;b<4;++b){l.fill(.001);r=l;gate.p->process(l.data(),r.data(),512);}check(l.back()>.0004,"gate hold and release preserve a decay initially");
    for(int b=0;b<40;++b){l.fill(.001);r=l;gate.p->process(l.data(),r.data(),512);}check(l.back()<.000003,"gate approaches requested attenuation range");
    for(int b=0;b<40;++b){l.fill(.023);r=l;gate.p->process(l.data(),r.data(),512);}check(l.back()<.0001,"signal below opening threshold does not reopen a closed gate");
    for(int b=0;b<20;++b){l.fill(.1);r=l;gate.p->process(l.data(),r.data(),512);}
    for(int b=0;b<40;++b){l.fill(.023);r=l;gate.p->process(l.data(),r.data(),512);}check(l.back()>.022,"same signal inside hysteresis band keeps an already open gate open");
}
void safety(){
    const auto registry=EffectRegistry::builtIns();
    for(const auto& d:registry.descriptors()){
        if(d.typeId=="gain"||d.typeId=="overdrive"||d.typeId=="eq"||d.typeId=="delay"||d.typeId=="distortion"||d.typeId=="reverb"||d.typeId=="cabinet"||d.typeId=="fuzz"||d.typeId=="amp"||d.typeId=="compressor"||d.typeId=="chorus"||d.typeId=="tremolo"||d.typeId=="filter"||d.typeId=="pitch"||d.typeId=="ring_mod")continue;
        for(double rate:{44100.,48000.,96000.}){
            Effect e(d.typeId,rate);std::array<float,512> l{},r{};
            heapAudit::begin();e.p->process(l.data(),r.data(),0);e.p->process(l.data(),r.data(),512);check(heapAudit::finish(),"first callback has no lazy allocation");
            for(float x:l)check(std::abs(x)<1e-6,"default silence remains silent");
            for(const auto& control:d.parameters)e.set(control.key,control.maximum);e.p->reset();
            for(int b=0;b<30;++b){for(unsigned j=0;j<512;++j){l[j]=float(.3*std::sin(j*.37));r[j]=-l[j];}heapAudit::begin();e.p->process(l.data(),r.data(),b%2?512:1);check(heapAudit::finish(),"extremes do not allocate");for(float x:l)check(std::isfinite(x)&&std::abs(x)<64,"DSP bounded without MonitorEngine clamp");}
            auto next=d.factory(e.bank,0);check(next->prepare(rate,512),"replacement prepares");heapAudit::begin();next->activate(e.p.get());check(heapAudit::finish(),"activation copies only bounded fixed state");
        }
    }
    std::vector<std::vector<double>> responses;
    for(const auto& c:builtinCabinets()){
        auto bytes=builtinIRBytes(c.id);check(sha256(bytes)==c.id,"synthetic cab checksum");auto ir=decodeIR(bytes);check(ir.samples.size()==2048&&ir.sampleRate==48000,"synthetic cab original rate/length");
        auto response=[&](double hz){std::complex<double> y{};for(std::size_t j=0;j<ir.samples.size();++j)y+=double(ir.samples[j])*std::polar(1.,-2*std::numbers::pi*hz*j/48000);return std::abs(y);};
        check(std::abs(response(1000)-1)<5e-5,"new synthetic cab documented 1-kHz normalization");
        check(response(20)<.2&&response(15000)<.3,"new synthetic cab rejects deep sub-bass and ultrasonic fizz");
        std::vector<double> shape;for(double hz:{60.,150.,750.,2400.,5500.,9000.})shape.push_back(response(hz));responses.push_back(std::move(shape));
    }
    for(std::size_t a=0;a<responses.size();++a)for(std::size_t b=a+1;b<responses.size();++b){double difference=0;for(unsigned j=0;j<6;++j)difference+=std::pow(responses[a][j]-responses[b][j],2);check(std::sqrt(difference/6)>.07,"five synthetic cabinet families have distinct measured response shapes");}
    RackEngine rack;for(int i=0;i<9;++i)check(rack.add("amp_brit_crunch"),"nine instances structurally requested");check(!rack.prepare(48000),"over-budget serial latency rejected");
}
}
int main(){families();responseAndCleanup();eqGate();safety();if(!failures)std::cout<<"PASS: tone upgrade networks, Output/Master, early EQ, gate and bounded realtime paths\n";return failures?1:0;}
