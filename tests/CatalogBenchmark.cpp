#include "core/RackEngine.h"
#include <algorithm>
#include <chrono>
#include <cmath>
#include <iostream>
#include <numbers>
#include <thread>

using namespace guitarrack;
// Run the Release target directly. This unpaced wall-clock DSP microbenchmark
// has no driver, GUI, device, input/output protection, or measured round trip.
int main() {
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
