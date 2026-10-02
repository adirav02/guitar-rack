// Offline rendering and synthetic headroom/listening preparation; no devices.
#include "state/Presets.h"
#include "core/RackEngine.h"
#include "core/SoundCatalog.h"
#include "HeapAudit.h"
#include <algorithm>
#include <bit>
#include <cmath>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <numbers>
using namespace guitarrack;
namespace {
constexpr double pi=std::numbers::pi;
struct Wave {unsigned rate=48000;std::vector<float> mono;};
std::uint32_t le(const std::vector<unsigned char>& b,std::size_t at,unsigned n){if(at+n>b.size())throw std::runtime_error("Truncated WAV");std::uint32_t v=0;for(unsigned i=0;i<n;++i)v|=std::uint32_t(b[at+i])<<(8*i);return v;}
Wave readWave(const std::filesystem::path& path){
 if(!std::filesystem::is_regular_file(path)||std::filesystem::file_size(path)>256*1024*1024)throw std::runtime_error("DI must be a regular WAV <=256 MiB");
 std::ifstream file(path,std::ios::binary);std::vector<unsigned char> b((std::istreambuf_iterator<char>(file)),{});
 auto tag=[&](std::size_t at,const char* s){return at+4<=b.size()&&std::equal(s,s+4,b.begin()+at);};
 if(!tag(0,"RIFF")||!tag(8,"WAVE")||le(b,4,4)!=b.size()-8)throw std::runtime_error("Use a complete standard mono RIFF WAV");
 unsigned format=0,bits=0,channels=0,rate=0,align=0;std::size_t start=0,length=0;
 for(std::size_t at=12;at+8<=b.size();){std::size_t size=le(b,at+4,4);if(size>b.size()-at-8)throw std::runtime_error("Truncated WAV chunk");if(tag(at,"fmt ")){if(size<16||format)throw std::runtime_error("Invalid WAV fmt");format=le(b,at+8,2);channels=le(b,at+10,2);rate=le(b,at+12,4);align=le(b,at+20,2);bits=le(b,at+22,2);}if(tag(at,"data")){if(start)throw std::runtime_error("Duplicate WAV data");start=at+8;length=size;}at+=8+size+(size&1);}
 if(channels!=1||rate<8000||rate>192000||!align||align!=bits/8||length%align||!start||!((format==1&&(bits==16||bits==24||bits==32))||(format==3&&bits==32)))throw std::runtime_error("DI must be mono PCM16/24/32 or float32, 8-192 kHz");
 if(length/align>120*rate)throw std::runtime_error("DI exceeds 120 seconds");
 Wave w;w.rate=rate;w.mono.resize(length/align);
 for(std::size_t i=0;i<w.mono.size();++i){auto word=le(b,start+i*align,align);float x;if(format==3)x=std::bit_cast<float>(word);else{if(bits<32&&(word&(1u<<(bits-1))))word|=(~0u)<<bits;x=float(std::bit_cast<std::int32_t>(word)/std::ldexp(1.,bits-1));}if(!std::isfinite(x)||std::abs(x)>1)throw std::runtime_error("DI contains nonfinite or out-of-full-scale data");w.mono[i]=x;}return w;
}
void writeWave(const std::filesystem::path& path,unsigned rate,const std::vector<float>& l,const std::vector<float>& r){
 if(std::filesystem::exists(path))throw std::runtime_error("Output exists; choose a new filename");
 std::ofstream f(path,std::ios::binary);
 auto put=[&](std::uint32_t v,unsigned n){for(unsigned i=0;i<n;++i)f.put(char(v>>(i*8)));};const bool stereo=!r.empty();const unsigned channels=stereo?2:1,size=unsigned(l.size())*channels*4;
 f.write("RIFF",4);put(36+size,4);f.write("WAVEfmt ",8);put(16,4);put(3,2);put(channels,2);put(rate,4);put(rate*channels*4,4);put(channels*4,2);put(32,2);f.write("data",4);put(size,4);
 for(std::size_t i=0;i<l.size();++i){put(std::bit_cast<std::uint32_t>(l[i]),4);if(stereo)put(std::bit_cast<std::uint32_t>(r[i]),4);}if(!f)throw std::runtime_error("Cannot write complete render");
}
Wave synthetic(unsigned rate=48000){Wave w;w.rate=rate;w.mono.resize(rate*8);double bendPhase=0;
 for(std::size_t i=0;i<w.mono.size();++i){double t=double(i)/rate,x=0;const double local=std::fmod(t,1.);
  if(t<3){const double strength=t<1?.25:t<2?.6:1.;x=strength*std::exp(-5*local)*(std::sin(2*pi*110*t)+.3*std::sin(2*pi*330*t)+.12*std::sin(2*pi*770*t));}
  else if(t<5){for(double hz:{82.4069,123.4708,164.8138,207.6523})x+=.22*std::sin(2*pi*hz*t);x*=std::exp(-3*std::fmod(t,.5));}
  else if(t<6){double hz=220*std::pow(2.,(t-5)*2/12);bendPhase+=2*pi*hz/rate;x=.7*std::sin(bendPhase);}
  else if(t<7)x=.5*(7-t)*std::sin(2*pi*440*t);
  w.mono[i]=float(.1*x);
 }return w;
}
struct Render {std::vector<float> l,r;double peak=0,rms=0;};
Render render(const ToneState& tone,const Wave& input,float strength=1,bool creative=false){
 RackEngine rack;if(creative&&!rack.setLatencyProfile(4096))throw std::runtime_error("Creative profile unavailable");
 if(!rack.loadTone(tone)||!rack.prepare(input.rate))throw std::runtime_error(rack.snapshot().error);
 std::array<float,512> l{},r{};
 // Deterministic warm-up from silence before the DI begins, including ingress.
 for(unsigned n=0;n<unsigned(rack.latencySamples())+input.rate/20;n+=512){l.fill(0);r.fill(0);rack.beginBlock();rack.process(l.data(),r.data(),512);}
 Render result;const auto length=input.mono.size()+input.rate*8;result.l.resize(length);result.r.resize(length);double squares=0;
 const double trim=std::pow(10.,tone.controls.inputDb/20),output=std::pow(10.,tone.controls.outputDb/20);
 for(std::size_t at=0;at<length;at+=512){int n=std::min<std::size_t>(512,length-at);for(int j=0;j<n;++j)l[j]=r[j]=at+j<input.mono.size()?float(input.mono[at+j]*trim*strength):0;
  heapAudit::begin();rack.beginBlock();rack.process(l.data(),r.data(),n);if(!heapAudit::finish())throw std::runtime_error("Render callback heap operation");
  for(int j=0;j<n;++j){const double a=l[j]*output,b=r[j]*output;if(!std::isfinite(a)||!std::isfinite(b))throw std::runtime_error("Nonfinite DSP render");result.l[at+j]=float(a);result.r[at+j]=float(b);result.peak=std::max({result.peak,std::abs(a),std::abs(b)});if(at+j<input.mono.size())squares+=(a*a+b*b)/2;}
 }result.rms=std::sqrt(squares/input.mono.size());rack.stop();return result;
}
}
int main(int argc,char** argv){try{
 if(argc==3&&std::string(argv[1])=="--synthetic"){auto w=synthetic();writeWave(argv[2],w.rate,w.mono,{});std::cout<<"Original synthetic DI, amplitude bound not pickup calibration: "<<argv[2]<<'\n';return 0;}
 auto registry=EffectRegistry::builtIns();auto catalog=factoryPresets(registry);
 if((argc==3||argc==4)&&std::string(argv[1])=="--catalog-audit"){
  std::ofstream csv(argv[2]);csv<<"id,name,rate,input_scale,input_peak,input_bound,processor_latency,peak,rms,suggested_output_db\n"<<std::setprecision(8);auto input=synthetic();double inputPeak=0;for(float x:input.mono)inputPeak=std::max(inputPeak,std::abs(double(x)));
  unsigned audited=0;
  for(const auto& tone:catalog){
   if(argc==4){const auto* metadata=soundMetadata(tone.id);if(!metadata||std::find(metadata->tags.begin(),metadata->tags.end(),argv[3])==metadata->tags.end())continue;}
   ++audited;bool creative=std::any_of(tone.rack.instances.begin(),tone.rack.instances.end(),[](const auto& i){return i.effectType=="expression_pitch";});
   for(float strength:{.3f,1.f,3.f}){auto result=render(tone,input,strength,creative);double change=20*std::log10(.015/std::max(result.rms,1e-12));double limit=20*std::log10(.6/std::max(result.peak,1e-12));double suggested=std::clamp(tone.controls.outputDb+std::min(change,limit),-60.,6.);csv<<tone.id<<','<<std::quoted(tone.name)<<','<<input.rate<<','<<strength<<','<<inputPeak*strength<<','<<.142*strength<<','<<(creative?4096:128)<<','<<result.peak<<','<<result.rms<<','<<suggested<<'\n';}
  }if(!audited)throw std::runtime_error("No factory sounds match the audit tag");return csv?0:2;
 }
 if(argc<4){std::cerr<<"Usage: guitarrack_render <factory UUID or full name> mono-DI.wav new-output.wav [--creative]\n       guitarrack_render --synthetic new-DI.wav\n       guitarrack_render --catalog-audit output.csv [exact metadata tag]\n";return 2;}
 auto at=std::find_if(catalog.begin(),catalog.end(),[&](const auto& t){return t.id==argv[1]||t.name==argv[1];});if(at==catalog.end())throw std::runtime_error("Unknown factory sound UUID/name");
 const bool creative=argc==5&&std::string(argv[4])=="--creative";auto input=readWave(argv[2]);auto result=render(*at,input,1,creative);writeWave(argv[3],input.rate,result.l,result.r);
 std::cout<<"Rendered "<<at->name<<", "<<input.rate<<" Hz; DSP profile "<<(creative?4096:128)<<" samples, peak "<<result.peak<<", phrase RMS "<<result.rms<<"; 8-second tail. No MonitorEngine clamp or device used.\n";return 0;
 }catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
