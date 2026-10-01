#include "core/RackEngine.h"
#include <algorithm>
#include <chrono>
#include <cmath>
#include <iostream>
#include <fstream>
#include <iomanip>
#include <numbers>
#include <thread>

using namespace guitarrack;
// Run the Release target directly. This unpaced wall-clock DSP microbenchmark
// has no driver, GUI, device, input/output protection, or measured round trip.
int matrix(const char* path) {
    std::ofstream csv(path);csv<<"scenario,rate,frames,processor_latency,mean_us,p99_us,max_us,fade_mean_us,fade_p99_us,fade_max_us,peak\n"<<std::setprecision(9);
    for(double rate:{44100.,48000.,96000.})for(int block:{64,128,256,512})for(int scenario=0;scenario<5;++scenario) {
        RackEngine rack;
        if(scenario==3) {if(!rack.setLatencyProfile(4096))return 1;}
        if(scenario==1)for(auto type:{"noise_gate","tight_boost","amp_modern_tight","cabinet","parametric_eq","space_reverb"})rack.add(type);
        if(scenario==2)for(auto type:{"noise_gate","dual_amp","cabinet","parametric_eq","echo","space_reverb"})rack.add(type);
        if(scenario==3)for(auto type:{"amp_brit_chime","cabinet","swell","expression_pitch","echo","space_reverb","audio_hold"})rack.add(type);
        if(scenario==4) {
            ImpulseResponse longIR;longIR.sampleRate=48000;longIR.samples.resize(24000);longIR.samples[0]=.4;
            for(std::size_t i=1;i<longIR.samples.size();++i)longIR.samples[i]=float(.003*std::exp(-double(i)/4000)*std::sin(i*.2));
            rack.setAssetResolver([longIR](const std::string&){return longIR;});
            for(unsigned i=0;i<4;++i)for(auto type:{"asymmetric_drive","amp_brit_hotrod","cabinet"})rack.add(type);
            for(auto type:{"noise_gate","echo","space_reverb","space_reverb"})rack.add(type);
        }
        if(!rack.prepare(rate)){std::cerr<<rack.snapshot().error<<'\n';return 1;}
        constexpr int iterations=1000;std::array<float,512> l{},r{};std::vector<double> times(iterations);std::uint64_t offset=0;double peak=0;
        auto run=[&]{for(int j=0;j<block;++j)l[j]=r[j]=float(.08*std::sin(2*std::numbers::pi*220*(offset+j)/rate));offset+=block;rack.beginBlock();rack.process(l.data(),r.data(),block);};
        for(int j=0;j<200;++j)run();
        double sum=0;
        for(int j=0;j<iterations;++j){const auto start=std::chrono::steady_clock::now();run();times[j]=std::chrono::duration<double,std::micro>(std::chrono::steady_clock::now()-start).count();sum+=times[j];for(int k=0;k<block;++k){if(!std::isfinite(l[k]))return 2;peak=std::max(peak,double(std::abs(l[k])));}}
        std::sort(times.begin(),times.end());double fadeSum=0;std::vector<double> fades;
        if(scenario) {
            auto definition=rack.snapshot().committed;rack.move(definition.order.back(),-1);
            const auto end=std::chrono::steady_clock::now()+std::chrono::seconds(5);
            while(!rack.snapshot().preparedWaiting&&std::chrono::steady_clock::now()<end)std::this_thread::sleep_for(std::chrono::milliseconds(1));
            if(!rack.snapshot().preparedWaiting)return 3;
            if(scenario==2)for(const auto& instance:definition.instances)if(instance.effectType=="dual_amp") {
                // Both branches change while two rack graphs overlap: bounded
                // four-model work per graph, with no timing of GUI control writes.
                if(!rack.setParameter(instance.id,0,4)||!rack.setParameter(instance.id,4,5))return 4;
            }
            const int chunks=(rack.latencySamples()+int(rate*.02)+block-1)/block+2;fades.resize(chunks);
            for(int j=0;j<chunks;++j){auto start=std::chrono::steady_clock::now();run();fades[j]=std::chrono::duration<double,std::micro>(std::chrono::steady_clock::now()-start).count();fadeSum+=fades[j];}std::sort(fades.begin(),fades.end());
        }
        csv<<std::array<const char*,5>{"empty legacy","modern rhythm","dual amp shared cab","spectral ambient creative","dense 16 / four 500ms IRs"}[scenario]<<','<<rate<<','<<block<<','<<rack.latencySamples()<<','<<sum/iterations<<','<<times[iterations*99/100]<<','<<times.back()<<','<<(fades.empty()?0:fadeSum/fades.size())<<','<<(fades.empty()?0:fades[fades.size()*99/100])<<','<<(fades.empty()?0:fades.back())<<','<<peak<<'\n';csv.flush();
    }
    return csv?0:1;
}
int main(int argc,char** argv) {
    if(argc==3&&std::string(argv[1])=="--matrix")return matrix(argv[2]);
    ImpulseResponse longIR;longIR.sampleRate=48000;longIR.samples.resize(24000);
    longIR.samples[0]=.4f;
    for(std::size_t i=1;i<longIR.samples.size();++i) longIR.samples[i]=static_cast<float>(.003*std::exp(-double(i)/4000)*std::sin(i*.2));
    for(int scenario=0;scenario<4;++scenario) {
        RackEngine rack;
        if(scenario>=2) rack.setAssetResolver([&](const std::string&){return longIR;});
        if(scenario==1||scenario==2) for(auto type:{"distortion","cabinet","eq","delay","reverb"}) rack.add(type);
        if(scenario==3) for(int i=0;i<4;++i)for(auto type:{"distortion","cabinet","delay","reverb"}) rack.add(type);
        if(!rack.prepare(48000)) {std::cerr<<rack.snapshot().error<<'\n';return 1;}
        constexpr int block=256,iterations=1600;std::array<float,block> l{},r{};
        std::vector<double> timings(iterations);double peak=0;std::uint64_t offset=0;
        auto run=[&]{for(int i=0;i<block;++i) l[i]=r[i]=static_cast<float>(.08*std::sin(2*std::numbers::pi*220*(offset+i)/48000));offset+=block;rack.beginBlock();rack.process(l.data(),r.data(),block);};
        for(int i=0;i<200;++i)run();
        double total=0;
        for(int i=0;i<iterations;++i) {auto start=std::chrono::steady_clock::now();run();auto us=std::chrono::duration<double,std::micro>(std::chrono::steady_clock::now()-start).count();timings[i]=us;total+=us;for(float x:l){if(!std::isfinite(x)){std::cerr<<"nonfinite benchmark\n";return 1;}peak=std::max(peak,double(std::abs(x)));}}
        std::sort(timings.begin(),timings.end());
        std::cout<<std::array<const char*,4>{"empty","five-effect synthetic starter","five-effect 500ms IR","16 effects / four 500ms IRs"}[scenario]
            <<": mean "<<total/iterations<<" us, p99 "<<timings[iterations*99/100]<<" us, max "<<timings.back()<<" us, mean/block-period "<<total/iterations/(1e6*block/48000)*100<<"%, peak "<<peak<<'\n';
        if(scenario>0) {
            auto d=rack.snapshot().committed;rack.move(d.order.back(),-1);
            auto end=std::chrono::steady_clock::now()+std::chrono::seconds(5);
            while(!rack.snapshot().preparedWaiting&&std::chrono::steady_clock::now()<end)std::this_thread::sleep_for(std::chrono::milliseconds(1));
            if(!rack.snapshot().preparedWaiting){std::cerr<<"benchmark preparation timeout\n";return 1;}
            double maximum=0,totalFade=0;
            for(int i=0;i<5;++i){auto start=std::chrono::steady_clock::now();run();auto us=std::chrono::duration<double,std::micro>(std::chrono::steady_clock::now()-start).count();maximum=std::max(maximum,us);totalFade+=us;}
            std::cout<<"  Two-graph transition mean "<<totalFade/5<<" us, max "<<maximum<<" us\n";
        }
    }
}
