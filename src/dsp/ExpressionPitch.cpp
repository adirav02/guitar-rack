#include "CreativeEffects.h"
#include "TonePrimitives.h"
#include "core/StereoDelay.h"
#include <complex>
namespace guitarrack {
namespace {
using namespace tone;
// Original fixed-window spectral sinusoidal pitch shifter. Monophonic starting point; no chord
// quality mode, neural model, Whammy equivalence or low-latency claim.
class ExpressionPitch final:public EffectProcessor {
public:
    ExpressionPitch(const ParameterBank& b,SlotId s,std::vector<ParameterDescriptor> p):controls(b,s,std::move(p)){}
    bool prepare(double r,int chunk) override {
        if(!std::isfinite(r)||r<8000||r>768000||chunk<1)return false;
        rate=r;
        for(unsigned i=0;i<N;++i){window[i]=.5-.5*std::cos(tau*i/N);unsigned x=i,y=0;for(unsigned bit=0;bit<11;++bit){y=(y<<1)|(x&1);x>>=1;}reverse[i]=y;}
        for(unsigned i=0;i<N/2;++i)twiddle[i]=std::polar(1.,-tau*i/N);
        reset();return true;
    }
    void reset() noexcept override {channels={};write=outWrite=hop=0;controls.update(true,rate);dryDelay.length=N;dryDelay.reset();ingress.reset(1);}
    void activate(const EffectProcessor* old) noexcept override {if(auto* p=dynamic_cast<const ExpressionPitch*>(old);p&&p->rate==rate)controls.ramps=p->controls.ramps;controls.update(!old,rate);ingress.reset(0);ingress.aim(1,int(rate*.02));}
    int latencySamples() const noexcept override{return N;}
    double tailSeconds() const noexcept override{return .6;}
    void process(float* l,float* r,int count) noexcept override {
        if(count<=0)return;
        controls.update(false,rate);
        for(int i=0;i<count;++i){const auto v=controls.next();const double semitones=v[0]+(v[1]-v[0])*v[2]*.01+v[3]*.01;const double ratio=std::pow(2.,semitones/12);const double inputFade=ingress.next();float dl=std::isfinite(l[i])?l[i]:0,dr=std::isfinite(r[i])?r[i]:0;
            const std::array<float,2> input{dl,dr};std::array<double,2> shifted{};
            for(unsigned c=0;c<2;++c){channels[c].input[write]=input[c]*inputFade;shifted[c]=channels[c].output[outWrite];channels[c].output[outWrite]=0;}
            if(++write==N)write=0;
            if(++outWrite==2*N)outWrite=0;
            if(++hop==H){hop=0;for(unsigned c=0;c<2;++c)frame(channels[c],ratio);}
            dryDelay.tick(dl,dr);
            // Exact neutral endpoint and aligned dry Mix. Analysis delay remains
            // audible/reported for nonzero pitch; transient smearing is additional.
            if(std::abs(semitones)<1e-7)shifted={dl,dr};
            const double mix=v[4]*.01,level=db(v[5]);l[i]=float(clean(((1-mix)*dl+mix*shifted[0])*level));r[i]=float(clean(((1-mix)*dr+mix*shifted[1])*level));
        }
    }
private:
    static constexpr unsigned N=2048,H=256,B=N/2+1;
    struct Channel {std::array<double,N> input{};std::array<double,2*N> output{};std::array<double,B> previous{},phase{};std::array<double,64> oscillator{},frequencies{};unsigned peaks=0;bool first=true;};
    void fft(std::array<std::complex<double>,N>& values,bool inverse) noexcept {
        for(unsigned i=0;i<N;++i)if(i<reverse[i])std::swap(values[i],values[reverse[i]]);
        for(unsigned size=2;size<=N;size*=2)for(unsigned start=0;start<N;start+=size)for(unsigned j=0;j<size/2;++j){const auto w=inverse?std::conj(twiddle[j*N/size]):twiddle[j*N/size];const auto a=values[start+j],b=w*values[start+j+size/2];values[start+j]=a+b;values[start+j+size/2]=a-b;}
        if(inverse)for(auto& v:values)v/=N;
    }
    void frame(Channel& c,double ratio) noexcept {
        for(unsigned i=0;i<N;++i)work[i]={c.input[(write+i)%N]*window[i],0};
        fft(work,false);
        for(unsigned bin=0;bin<B;++bin){
            analysisPhase[bin]=std::arg(work[bin]);magnitude[bin]=std::abs(work[bin]);
            const double expected=tau*bin*H/N;
            double deviation=analysisPhase[bin]-c.previous[bin]-expected;
            deviation-=tau*std::round(deviation/tau);c.previous[bin]=analysisPhase[bin];
            frequency[bin]=(bin+deviation*N/(tau*H))*rate/N;
            if(c.first)c.phase[bin]=analysisPhase[bin];
            else c.phase[bin]+=tau*frequency[bin]*ratio*H/rate;
            c.phase[bin]-=tau*std::floor(c.phase[bin]/tau);
        }
        work.fill({0,0});
        // Reconstruct a narrow Hann lobe around each measured spectral peak.
        // Stretching the entire source lobe creates octave-dependent cancellation
        // at +24. This bounded sinusoidal model preserves held-note amplitudes.
        double maximum=0;for(double m:magnitude)maximum=std::max(maximum,m);
        std::array<double,64> nextPhase{},nextFrequency{};std::array<bool,64> used{};unsigned peaks=0;
        auto hann=[&](double x) noexcept {
            if(std::abs(x)<1e-9)return N*.5;
            if(std::abs(std::abs(x)-1)<1e-9)return N*.25;
            return N*.5*std::sin(std::numbers::pi*x)/(std::numbers::pi*x*(1-x*x));
        };
        for(unsigned bin=1;bin<N/2-1 && peaks<64;++bin) {
            if(magnitude[bin]<maximum*.015 || magnitude[bin]<1e-12 || magnitude[bin]<magnitude[bin-1] || magnitude[bin]<magnitude[bin+1])continue;
            const double sourceHz=frequency[bin],targetHz=sourceHz*ratio;
            if(sourceHz<=0||targetHz>=rate*.48)continue;
            unsigned matched=c.peaks;double distance=1e9;
            for(unsigned j=0;j<c.peaks;++j)if(!used[j]){double d=std::abs(std::log2(sourceHz/c.frequencies[j]));if(d<distance&&d<.2){distance=d;matched=j;}}
            const double sourcePosition=sourceHz*N/rate;
            double carrier=analysisPhase[bin]+std::numbers::pi*(bin-sourcePosition);
            if(matched<c.peaks){used[matched]=true;carrier=c.oscillator[matched]+tau*targetHz*H/rate;}
            carrier-=tau*std::floor(carrier/tau);
            nextPhase[peaks]=carrier;nextFrequency[peaks]=sourceHz;++peaks;
            const double amplitude=magnitude[bin]/std::max(1.,std::abs(hann(bin-sourcePosition)));
            const double target=targetHz*N/rate;const int center=int(std::round(target));
            for(int offset=-4;offset<=4;++offset){const int at=center+offset;if(at<=0||at>=int(N/2))continue;const double x=at-target,w=hann(x);work[unsigned(at)]+=amplitude*w*std::polar(1.,carrier-std::numbers::pi*x);}
        }
        c.oscillator=nextPhase;c.frequencies=nextFrequency;c.peaks=peaks;
        work[0]={work[0].real(),0};work[N/2]={work[N/2].real(),0};
        for(unsigned bin=1;bin<N/2;++bin)work[N-bin]=std::conj(work[bin]);
        fft(work,true);
        for(unsigned i=0;i<N;++i)c.output[(outWrite+i)%(2*N)]+=work[i].real()*window[i]/3.;
        c.first=false;
    }
    Controls controls;double rate=48000;std::array<Channel,2> channels{};std::array<double,N> window{};std::array<unsigned,N> reverse{};std::array<std::complex<double>,N/2> twiddle{};std::array<std::complex<double>,N> work{};std::array<double,B> magnitude{},frequency{},analysisPhase{};unsigned write=0,outWrite=0,hop=0;StereoDelay dryDelay;LinearRamp ingress;
};
}
EffectDescriptor expressionPitchDescriptor(){
    std::vector<ParameterDescriptor> p{{"heel_semitones","Heel Shift","st",0,-24,24,0},{"toe_semitones","Toe Shift","st",1,-24,24,12},{"expression_percent","Expression","%",2,0,100,0},{"detune_cents","Detune","cent",3,-100,100,0},{"mix_percent","Mix","%",4,0,100,100},{"output_db","Output","dB",5,-24,6,0}};
    return {"expression_pitch","Expression Pitch (Spectral)","Pitch",1,p,[p](const ParameterBank& b,SlotId s){return std::make_unique<ExpressionPitch>(b,s,p);}};
}
}
