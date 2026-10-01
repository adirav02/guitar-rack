#include "core/Effects.h"
#include "core/RackEngine.h"
#include "HeapAudit.h"
#include <algorithm>
#include <cmath>
#include <iostream>
#include <numbers>
using namespace guitarrack;
namespace {
int failures=0;void check(bool ok,const char* label){if(!ok){++failures;std::cerr<<"FAIL: "<<label<<'\n';}}
struct Effect {
 EffectRegistry registry=EffectRegistry::builtIns();ParameterBank bank;const EffectDescriptor* d;std::unique_ptr<EffectProcessor> p;double rate;
 Effect(const char* type,double r=48000):d(registry.latest(type)),rate(r){for(const auto& c:d->parameters)bank.setValue(0,c.bankIndex,c.normalize(c.defaultValue));p=d->factory(bank,0);check(p->prepare(rate,512),"creative preparation");p->activate(nullptr);}
 void set(const char* key,float value){for(const auto& c:d->parameters)if(c.key==key){check(c.accepts(value),"creative test value valid");bank.setValue(0,c.bankIndex,c.normalize(value));return;}check(false,"creative test key exists");}
 std::vector<float> render(int length,double frequency=0,bool impulse=false,bool rightImpulse=false){std::vector<float> y(length);std::array<float,512> l{},r{};for(int at=0;at<length;at+=127){int n=std::min(127,length-at);for(int j=0;j<n;++j){l[j]=impulse?(at+j==0?.1f:0.f):float(.1*std::sin(2*std::numbers::pi*frequency*(at+j)/rate));r[j]=rightImpulse?l[j]:0;}heapAudit::begin();p->process(l.data(),r.data(),n);check(heapAudit::finish(),"creative callback heap audit");std::copy_n(l.begin(),n,y.begin()+at);}return y;}
};
double energy(const std::vector<float>& y,std::size_t first,std::size_t last){double sum=0;for(auto i=first;i<std::min(last,y.size());++i)sum+=double(y[i])*y[i];return sum;}
double amplitude(const std::vector<float>& y,double hz,double rate=48000){double re=0,im=0;const auto first=y.size()/2;for(std::size_t i=first;i<y.size();++i){const double a=2*std::numbers::pi*hz*i/rate;re+=y[i]*std::cos(a);im+=y[i]*std::sin(a);}return 2*std::hypot(re,im)/(y.size()-first);}
void timing(){
 Effect echo("echo");echo.set("clock",1);echo.set("bpm",120);echo.set("subdivision",2);echo.set("mix_percent",100);echo.set("feedback_percent",50);echo.p->reset();auto y=echo.render(48000,0,true);
 check(std::abs(y[18000]-.1)<1e-6&&std::abs(y[36000]-.05)<1e-6,"dotted-eighth first/repeat impulse timing at 120 BPM");
 echo.set("stereo",1);echo.p->reset();std::array<float,512> l{},r{};double firstLeft=0,firstRight=0,secondLeft=0,secondRight=0;
 for(int at=0;at<40000;at+=127){int n=std::min(127,40000-at);for(int i=0;i<n;++i){l[i]=at+i==0?.1f:0;r[i]=0;}echo.p->process(l.data(),r.data(),n);for(int i=0;i<n;++i){if(at+i==18000){firstLeft=l[i];firstRight=r[i];}if(at+i==36000){secondLeft=l[i];secondRight=r[i];}}}
 check(std::abs(firstLeft-.1)<1e-6&&firstRight==0&&secondLeft==0&&std::abs(secondRight-.05)<1e-6,"ping-pong repeats cross channels after the first echo");
 Effect wah("wah");wah.set("position_percent",0);wah.p->reset();auto heel=wah.render(48000,350);wah.set("position_percent",100);wah.p->reset();auto toe=wah.render(48000,350);check(amplitude(heel,350)>4*amplitude(toe,350),"wah moves a resonant band-pass, not a low-pass cutoff");
 Effect flanger("flanger");flanger.set("rate_hz",.05);flanger.set("depth_percent",0);flanger.set("manual_ms",2);flanger.set("mix_percent",100);flanger.set("feedback_percent",0);flanger.p->reset();auto flange=flanger.render(1000,0,true);check(std::abs(flange[96]-.1)<1e-6&&energy(flange,0,96)==0,"flanger short wet delay timing");
}
void spaces(){
 Effect space("space_reverb");space.set("mix_percent",100);space.set("pre_delay_ms",0);space.set("decay_seconds",.5);space.p->reset();auto shortTail=space.render(4*48000,0,true);
 space.set("decay_seconds",3);space.p->reset();auto longTail=space.render(4*48000,0,true);
 check(energy(longTail,48000,3*48000)>100*energy(shortTail,48000,3*48000),"physical decay control changes late tail energy");
 auto room=longTail;space.set("mode",1);space.p->reset();auto plate=space.render(4*48000,0,true);space.set("mode",2);space.p->reset();auto hall=space.render(4*48000,0,true);
 check(room!=plate&&plate!=hall&&energy(plate,0,48000)>0&&energy(hall,0,48000)>0,"room/plate/hall are distinct excited networks");
 space.set("mode",0);space.set("pre_delay_ms",100);space.p->reset();auto pre=space.render(48000,0,true);check(energy(pre,0,4800)==0&&energy(pre,4800,12000)>0,"reverb pre-delay adds intentional onset time");
 space.set("pre_delay_ms",18);space.p->reset();auto fractionalWrap=space.render(24000,0,true);check(energy(fractionalWrap,0,864)==0&&energy(fractionalWrap,864,10000)>0,"18-ms read-head wrap remains valid at integer boundary with floating rounding");
 Effect hold("audio_hold");hold.set("mix_percent",100);hold.set("output_db",0);hold.p->reset();auto source=hold.render(48000,220);hold.set("hold",1);auto captured=hold.render(48000,0);check(energy(captured,24000,48000)>1,"audio hold repeats captured signal during new silence");auto expired=hold.render(121*48000,0);check(energy(expired,120*48000,121*48000)<1e-12,"captured tail expires within declared 120 seconds and does not retrigger while Hold stays on");
 hold.set("hold",0);auto released=hold.render(12000,0);check(energy(released,5000,12000)<1e-12,"hold release returns to silence without hidden sustain");
}
void pitch(){
 for(double rate:{44100.,48000.,96000.}){
  Effect pitch("expression_pitch",rate);pitch.set("mix_percent",0);pitch.p->reset();auto dry=pitch.render(5000,0,true);check(pitch.p->latencySamples()==2048&&dry[2048]==.1f&&energy(dry,0,2048)==0,"pitch zero-mix dry is exactly latency aligned");
 }
 // Cents measured by a fine coherent projection search, not a loose octave bin.
 for(double note:{82.4069,110.,220.,440.,880.,1760.})for(int shift:{-24,-12,12,24}){
  Effect pitch("expression_pitch");pitch.set("heel_semitones",float(shift));pitch.set("toe_semitones",float(shift));pitch.p->reset();auto y=pitch.render(96000,note);
  const double expected=note*std::pow(2.,shift/12.);double best=0,bestCents=0;
  for(int cents=-30;cents<=30;++cents){const double a=amplitude(y,expected*std::pow(2.,cents/1200.));if(a>best){best=a;bestCents=cents;}}
  std::cout<<"pitch,"<<note<<','<<shift<<','<<expected<<','<<bestCents<<','<<best<<'\n';
  check(std::abs(bestCents)<=5&&best>.005,"pitch held-note cents and useful fundamental amplitude");
 }
 Effect endpoint("expression_pitch");endpoint.set("expression_percent",100);endpoint.p->reset();auto toe=endpoint.render(96000,220);check(amplitude(toe,440)>.09&&amplitude(toe,220)<.001,"software expression reaches the musical octave endpoint");
 for(double rate:{44100.,96000.})for(int shift:{-24,24}){Effect p("expression_pitch",rate);p.set("heel_semitones",float(shift));p.set("toe_semitones",float(shift));p.p->reset();auto y=p.render(int(rate*2),110);const double expected=110*std::pow(2.,shift/12.);std::cout<<"pitch-rate,"<<rate<<','<<shift<<','<<amplitude(y,expected,rate)<<'\n';check(amplitude(y,expected,rate)>.005,"two-octave endpoints remain audible at other prepared rates");}
 // Characterize changing notes, chord separation and onset independently. These
 // are artifact evidence, not a claimed polyphonic quality mode.
 Effect moving("expression_pitch");moving.set("heel_semitones",12);moving.set("toe_semitones",12);moving.p->reset();
 std::vector<float> bend(96000),chord(96000);std::array<float,127> l{},r{};double phase=0;
 for(int at=0;at<96000;at+=127){int n=std::min(127,96000-at);for(int j=0;j<n;++j){double t=double(at+j)/48000;phase+=2*std::numbers::pi*220*std::pow(2.,t/12)/48000;l[j]=r[j]=float(.1*std::sin(phase));}heapAudit::begin();moving.p->process(l.data(),r.data(),n);check(heapAudit::finish(),"bend processing is allocation free");std::copy_n(l.begin(),n,bend.begin()+at);}
 double bendPeak=0;for(float x:bend){check(std::isfinite(x),"pitch bend finite");bendPeak=std::max(bendPeak,std::abs(double(x)));}check(bendPeak<.25,"pitch bend is bounded without monitor protection");
 moving.p->reset();for(int at=0;at<96000;at+=127){int n=std::min(127,96000-at);for(int j=0;j<n;++j){double t=double(at+j)/48000;l[j]=r[j]=float(.035*(std::sin(2*std::numbers::pi*110*t)+std::sin(2*std::numbers::pi*138.5913*t)+std::sin(2*std::numbers::pi*164.8138*t)));}heapAudit::begin();moving.p->process(l.data(),r.data(),n);check(heapAudit::finish(),"chord characterization is allocation free");std::copy_n(l.begin(),n,chord.begin()+at);}
 std::cout<<"Pitch bend peak: "<<bendPeak<<"; chord octave fundamental amplitudes: "<<amplitude(chord,220)<<','<<amplitude(chord,277.1826)<<','<<amplitude(chord,329.6276)<<" (no chord-quality mode claimed)\n";
 check(std::all_of(chord.begin(),chord.end(),[](float x){return std::isfinite(x)&&std::abs(x)<.3f;}),"chord render finite and bounded; listening acceptance remains pending");
 Effect shifted("expression_pitch");shifted.set("heel_semitones",12);shifted.set("toe_semitones",12);shifted.p->reset();auto impulse=shifted.render(10000,0,true);auto peak=std::max_element(impulse.begin(),impulse.end(),[](float a,float b){return std::abs(a)<std::abs(b);});auto onset=std::find_if(impulse.begin(),impulse.end(),[](float x){return std::abs(x)>1e-5;});std::cout<<"Pitch octave impulse onset/peak: "<<onset-impulse.begin()<<'/'<<peak-impulse.begin()<<" samples; declared window/alignment 2048\n";
 RackEngine rack;check(rack.setLatencyProfile(creativeLatencySamples),"Creative selected while stopped");check(rack.add("expression_pitch")&&rack.prepare(48000),"spectral rack prepares in explicit profile");check(!rack.setLatencyProfile(128),"profile cannot change while processing");rack.stop();check(!rack.setLatencyProfile(128)&&rack.latencySamples()==4096,"incompatible profile retains existing sound and declared latency");
}
void blend(){
 Effect a("dual_amp"),b("dual_amp");a.set("model_b",6);a.set("gain_b_db",18);a.set("level_a_db",0);a.set("level_b_db",0);a.set("blend_percent",0);a.p->reset();auto dry=a.render(48000,211);
 a.set("blend_percent",100);a.p->reset();auto full=a.render(48000,211);check(dry==full,"identical parallel branches have equal polarity and timing at endpoints");
 a.set("blend_percent",50);a.p->reset();auto half=a.render(48000,211);check(half==dry,"correlated equal branches do not double level at 50/50 blend");
}
}
int main(){timing();spaces();pitch();blend();if(!failures)std::cout<<"PASS: creative timing, networks, capture, pitch cents/alignment and parallel polarity\n";return failures?1:0;}
