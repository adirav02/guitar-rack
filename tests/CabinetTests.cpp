#include "dsp/Cabinet.h"
#include "core/RackEngine.h"
#include "core/MonitorEngine.h"
#include "HeapAudit.h"
#include <algorithm>
#include <bit>
#include <chrono>
#include <cmath>
#include <iostream>
#include <fstream>
#include <limits>
#include <random>
#include <thread>

using namespace guitarrack;
namespace {
int failures=0;
void check(bool b,const char* s){if(!b){++failures;std::cerr<<"FAIL: "<<s<<'\n';}}
template<class F> void rejects(F f,const char* s){try{f();check(false,s);}catch(const std::exception&){}}
template<class F> bool wait(F f){auto end=std::chrono::steady_clock::now()+std::chrono::seconds(5);do{if(f())return true;std::this_thread::sleep_for(std::chrono::milliseconds(1));}while(std::chrono::steady_clock::now()<end);return false;}
void wavValidation() {
    const std::string abc="abc";
    check(sha256(std::span(reinterpret_cast<const std::uint8_t*>(abc.data()),abc.size()))=="ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad","SHA-256 published abc vector");
    check(sha256({})=="e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855","SHA-256 empty vector");
    auto bytes=syntheticIRBytes();auto ir=decodeIR(bytes);
    std::ifstream file(std::string(GUITARRACK_SOURCE_DIR)+"/assets/ir/synthetic-filter.wav",std::ios::binary);
    std::vector<std::uint8_t> tracked{std::istreambuf_iterator<char>(file),std::istreambuf_iterator<char>()};
    check(tracked==bytes,"tracked synthetic WAV matches embedded analytic asset exactly");
    check(ir.samples.size()==512&&ir.sampleRate==48000&&sha256(bytes)==syntheticIRId(),"embedded synthetic WAV identity/format");
    auto mutate=[&](auto f,const char* name){auto b=bytes;f(b);rejects([&]{decodeIR(b);},name);};
    mutate([](auto& b){b[0]='X';},"non WAV rejected");
    mutate([](auto& b){b.resize(40);},"truncated WAV rejected");
    mutate([](auto& b){b[22]=2;},"stereo IR rejected");
    mutate([](auto& b){b[20]=6;},"unsupported codec rejected");
    mutate([](auto& b){b[32]=2;},"bad block alignment rejected");
    mutate([](auto& b){for(std::size_t i=44;i<b.size();++i)b[i]=0;},"silent IR rejected");
    mutate([](auto& b){b[44]=0;b[45]=0;b[46]=128;b[47]=127;},"NaN/Inf IR rejected");
    mutate([](auto& b){b[44]=0;b[45]=0;b[46]=0;b[47]=64;},"out-of-full-scale float rejected");
    mutate([](auto& b){b.resize(maxIRBytes+1);},"file size limit enforced");
    mutate([](auto& b){b[24]=0;b[25]=0;b[26]=0;b[27]=0;},"invalid sample rate rejected");
    auto floatBytes=[&](unsigned n,unsigned rate,float value){if(n>maxIRBytes/4)throw std::runtime_error("test WAV too large");auto b=bytes;b.resize(44+4*n);auto put=[&](unsigned p,unsigned v){for(unsigned i=0;i<4;++i)b[p+i]=v>>(8*i);};put(4,b.size()-8);put(24,rate);put(28,rate*4);put(40,n*4);for(unsigned i=0;i<n;++i)put(44+i*4,std::bit_cast<unsigned>(value));return b;};
    rejects([&]{decodeIR(floatBytes(24001,48000,.001f));},"duration beyond 500 ms rejected");
    rejects([&]{decodeIR(floatBytes(96001,192000,.001f));},"decoded sample cap rejected");
    rejects([&]{decodeIR(floatBytes(1000,48000,1));},"excessive IR gain bound rejected");
    check(decodeIR(floatBytes(96000,192000,.0001f)).samples.size()==96000,"maximum valid source accepted");
}
void referenceConvolution() {
    std::mt19937 rng(7);std::uniform_real_distribution<float> random(-1,1);
    ImpulseResponse ir;ir.sampleRate=48000;ir.samples.resize(1173);
    for(auto& x:ir.samples)x=random(rng)*.012f;ir.samples[0]=.45f;ir.samples[255]=.1f;ir.samples[256]=-.15f;ir.samples[768]=.08f;
    constexpr int n=4096;std::vector<float> input(n),inputR(n),l(n),r(n),singleL(n),singleR(n);
    for(int i=0;i<1600;++i){input[i]=random(rng)*.1f;inputR[i]=random(rng)*.15f;}
    ParameterBank bank;bank.setValue(0,0,24.f/30);CabinetProcessor cabinet(bank,0);cabinet.setImpulse(ir);check(cabinet.prepare(48000,512),"prepare reference convolution");
    l=input;r=inputR;bool heap=true;
    for(int p=0;p<n;){int count=std::min((p%3==0?1:p%3==1?127:512),n-p);heapAudit::begin();cabinet.process(l.data()+p,r.data()+p,count);heap&=heapAudit::finish();p+=count;}
    double error=0;
    for(int i=0;i<n;++i){double a=0,b=0;for(int j=0;j<=i&&j<int(ir.samples.size());++j){a+=input[i-j]*ir.samples[j];b+=inputR[i-j]*ir.samples[j];}error=std::max({error,std::abs(a-l[i]),std::abs(b-r[i])});}
    check(error<2e-6,"partitioned stereo convolution matches direct reference across block boundaries");
    cabinet.reset();singleL=input;singleR=inputR;
    heapAudit::begin();cabinet.process(nullptr,nullptr,0);for(int i=0;i<n;++i)cabinet.process(singleL.data()+i,singleR.data()+i,1);heap&=heapAudit::finish();
    check(singleL==l&&singleR==r,"one sample/variable partitions identical; zero block no change");
    cabinet.reset();std::fill(l.begin(),l.end(),0);std::fill(r.begin(),r.end(),0);l[0]=1;cabinet.process(l.data(),r.data(),n);
    check(std::abs(l[0]-ir.samples[0])<1e-6&&cabinet.latencySamples()==0,"zero buffering latency and first tap timing");
    check(std::all_of(r.begin(),r.end(),[](float x){return x==0;}),"independent stereo convolution has no crosstalk");
    bank.setValue(0,0,18.f/30);cabinet.reset();l.assign(n,0);r.assign(n,0);l[0]=1;cabinet.process(l.data(),r.data(),n);
    check(std::abs(l[256]-ir.samples[256]*std::pow(10.f,-6.f/20))<1e-6,"IR Level preserves predictable unnormalized scale");
    cabinet.reset();l.assign(n,0);r.assign(n,0);cabinet.process(l.data(),r.data(),n);check(std::all_of(l.begin(),l.end(),[](float x){return x==0;}),"reset clears convolution histories");
    l[0]=std::numeric_limits<float>::infinity();r[0]=std::numeric_limits<float>::max();cabinet.process(l.data(),r.data(),n);check(std::all_of(l.begin(),l.end(),[](float x){return std::isfinite(x);})&&std::all_of(r.begin(),r.end(),[](float x){return std::isfinite(x);}),"abnormal samples stay finite");
    check(heap,"first use FFT/convolution no callback allocation/deallocation");
    std::cout<<"Convolution reference maximum error "<<error<<'\n';
}
void rates() {
    ImpulseResponse ir;ir.sampleRate=48000;ir.samples.assign(4800,0);ir.samples[2400]=.5f;
    for(double rate:{8000.,44100.,48000.,96000.,192000.,768000.}){
        auto out=resampleIR(ir,rate);auto peak=std::max_element(out.samples.begin(),out.samples.end());double sum=0;for(float x:out.samples)sum+=x;
        check(std::abs(double(peak-out.samples.begin())-rate*.05)<=1,"IR resampling keeps physical impulse time");
        check(std::abs(sum-.5)<.003,"IR resampling preserves interior impulse FIR gain");
        ParameterBank bank;bank.setValue(0,0,.8f);CabinetProcessor c(bank,0);c.setImpulse(ir);check(c.prepare(rate,512),"cabinet prepares actual sample rates including extremes");
        std::vector<float> l(out.samples.size()+512),r(l.size());l[0]=1;c.process(l.data(),r.data(),static_cast<int>(l.size()));double error=0;for(std::size_t i=0;i<out.samples.size();++i)error=std::max(error,std::abs(double(l[i]-out.samples[i])));check(error<1e-5,"resampled convolution impulse matches prepared kernel");
        check(c.prepare(rate,512),"reprepare retains selected impulse");
    }
}
void latencyAndEdits() {
    RackEngine rack;check(rack.prepare(48000),"prepare latency-aware rack");
    std::array<float,512> l{},r{};bool heap=true;
    auto block=[&](float x=0){l.fill(x);r.fill(x);heapAudit::begin();rack.beginBlock();rack.process(l.data(),r.data(),512);heap&=heapAudit::finish();};
    auto commit=[&]{return wait([&]{block(.05f);auto s=rack.snapshot();return s.requestedGeneration==s.committedGeneration;});};
    block();l[0]=1;heapAudit::begin();rack.beginBlock();rack.process(l.data(),r.data(),512);heap&=heapAudit::finish();check(l[128]==1&&l[0]==0&&rack.latencySamples()==128,"empty rack truthful fixed latency");
    check(rack.add("distortion")&&commit(),"Distortion live add");auto d=rack.snapshot().committed.order[0];rack.setParameter(d,3,0);for(int i=0;i<5;++i)block();
    l.fill(0);r.fill(0);l[0]=1;r[3]=.5f;heapAudit::begin();rack.beginBlock();rack.process(l.data(),r.data(),512);heap&=heapAudit::finish();check(l[128]==1&&r[131]==.5f,"Distortion Mix0 rack compensation matches empty graph");
    rack.setBypassed(d,true);for(int i=0;i<4;++i)block(.05f);rack.setParameter(d,3,100);for(int i=0;i<4;++i)block(.05f);
    float previous=.05f;bool continuity=true;check(rack.add("cabinet")&&commit(),"Cabinet registry/live preparation");auto cab=rack.snapshot().committed.order.back();rack.setBypassed(cab,true);for(int i=0;i<4;++i)block(.05f);
    check(rack.add("reverb")&&commit(),"Reverb registry/live preparation");auto rev=rack.snapshot().committed.order.back();rack.setParameter(rev,2,0);for(int i=0;i<4;++i)block(.05f);
    for(int edit=0;edit<30;++edit){rack.move(d,edit%2?-1:1);for(int i=0;i<5;++i){block(.05f);for(float x:l){continuity&=std::isfinite(x)&&std::abs(x-previous)<.02f;previous=x;}}}
    check(commit()&&continuity,"latency-aware bypass/reorder transitions stay bounded");
    rack.bank.setRackBypassed(true);for(int i=0;i<4;++i)block();l.fill(0);r.fill(0);l[0]=.25f;heapAudit::begin();rack.beginBlock();rack.process(l.data(),r.data(),512);heap&=heapAudit::finish();check(l[128]==.25f&&l[0]==0,"rack bypass retains exact processing latency");
    check(heap,"new integrated graph swaps/retirement/bypasses heap free");
    check(rack.prepare(96000),"new processors graph rate restart");block();auto s=rack.snapshot();check(wait([&]{return rack.snapshot().graphs==1;})&&s.peakGraphs<=4,"new catalog resources reclaimed off audio within four-graph bound");
}
void fullBankAndOrder() {
    RackEngine full;
    for(unsigned i=0;i<16;++i)check(full.add("distortion"),"16-slot maximum oversampling latency rack accepted");
    auto d=full.snapshot().requested;for(const auto& id:d.order)full.setParameter(id,3,0);
    check(full.prepare(48000),"prepare full latency budget");std::array<float,512> l{},r{};
    for(int b=0;b<5;++b){l.fill(0);r.fill(0);full.beginBlock();full.process(l.data(),r.data(),512);}
    l[0]=.5f;heapAudit::begin();full.beginBlock();full.process(l.data(),r.data(),512);bool heap=heapAudit::finish();
    check(l[128]==.5f&&l[0]==0&&heap,"16 Distortion stages total128 latency without double compensation");
    RackEngine a,b;
    for(auto* rack:{&a,&b}) {
        rack->add("distortion");rack->add("cabinet");auto definition=rack->snapshot().requested;
        rack->setParameter(definition.order[0],0,30);rack->setParameter(definition.order[0],2,0);rack->setParameter(definition.order[0],3,100);
        if(rack==&b)rack->move(definition.order[1],-1);
        check(rack->prepare(48000),"prepare noncommuting Distortion/Cabinet orders");
    }
    std::array<float,512> al{},ar{},bl{},br{};double difference=0,energy=0;
    for(int block=0;block<40;++block) {
        for(int i=0;i<512;++i)al[i]=ar[i]=bl[i]=br[i]=static_cast<float>(.1*std::sin((block*512+i)*.21));
        heapAudit::begin();a.beginBlock();b.beginBlock();a.process(al.data(),ar.data(),512);b.process(bl.data(),br.data(),512);heap&=heapAudit::finish();
        if(block>10)for(int i=0;i<512;++i){difference+=std::pow(al[i]-bl[i],2);energy+=al[i]*al[i];}
    }
    check(difference>energy*.001&&heap,"Cabinet/Distortion processing order has noncommuting measured signal output without identity/control changes");
    RackEngine bounded;bounded.setAssetResolver([](const std::string&){ImpulseResponse ir;ir.sampleRate=192000;ir.samples.assign(96000,.0001f);return ir;});
    for(int i=0;i<3;++i)bounded.add("cabinet");
    check(!bounded.prepare(768000)&&bounded.snapshot().error.find("768000 resampled")!=std::string::npos,"aggregate IR preparation budget rejects over-capacity before convolution allocation");
}
}
int main(){wavValidation();referenceConvolution();rates();latencyAndEdits();fullBankAndOrder();if(failures)return 1;std::cout<<"PASS: Cabinet WAV bounds, convolution, resampling, latency and live graph alignment\n";}
