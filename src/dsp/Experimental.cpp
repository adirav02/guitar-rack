#include "Experimental.h"
#include "EQ.h"
#include "core/LinearRamp.h"
#include <algorithm>
#include <cmath>
#include <numbers>

namespace guitarrack {
namespace {
enum class Kind { fuzz, amp, compressor, chorus, tremolo, filter, pitch, ring };
constexpr double tau = 2 * std::numbers::pi;
float safe(float x) noexcept {
    if (!std::isfinite(x) || std::abs(x) < 1e-20f) return 0;
    return std::clamp(x, -1e6f, 1e6f);
}
// All resources are prepared off audio. Temporal effects own independent stereo
// rings; activation inherits only small state, never copies a ring.
class Experimental final : public EffectProcessor {
public:
    Experimental(Kind k, const ParameterBank& b, SlotId s, std::vector<ParameterDescriptor> p)
        : kind(k), bank(b), slot(s), parameters(std::move(p)), equalizer(eqBank, 0) {}
    bool prepare(double sampleRate, int chunk) override {
        if (!std::isfinite(sampleRate) || sampleRate < 8000 || sampleRate > 768000 || chunk <= 0) return false;
        rate = sampleRate; duration = static_cast<int>(std::ceil(rate * .02));
        if (kind == Kind::chorus || kind == Kind::pitch)
            for (auto& ring : rings) ring.assign(static_cast<std::size_t>(std::ceil(rate * .12)) + 4, 0);
        if (kind == Kind::amp) {
            for (unsigned i = 0; i < 3; ++i) eqBank.setValue(0, i, bank.value(slot, i+1));
            if (!equalizer.prepare(rate, chunk)) return false;
        }
        reset(); return true;
    }
    void reset() noexcept override {
        for (auto& ring : rings) std::fill(ring.begin(), ring.end(), 0);
        states = {}; phase = 0; write = 0; envelope = 0;
        controls(true); ingress.reset(1);
        if (kind == Kind::amp) equalizer.reset();
    }
    void activate(const EffectProcessor* previous) noexcept override {
        if (auto* old = dynamic_cast<const Experimental*>(previous); old && old->kind == kind && old->rate == rate) {
            ramps = old->ramps; states = old->states; phase = old->phase; envelope = old->envelope;
            if (kind == Kind::amp) equalizer.activate(&old->equalizer);
        }
        controls(previous == nullptr);
        ingress.reset(0); ingress.aim(1, duration);
    }
    double tailSeconds() const noexcept override {
        return kind == Kind::chorus || kind == Kind::pitch ? .15 : (kind == Kind::compressor ? 2 : .5);
    }
    void process(float* left, float* right, int count) noexcept override {
        if (count <= 0) return;
        controls(false);
        if (kind == Kind::amp)
            for (unsigned j = 0; j < 3; ++j) eqBank.setValue(0, j, bank.value(slot, j+1));
        for (int i = 0; i < count; ++i) {
            std::array<float, 10> v{};
            for (std::size_t j = 0; j < parameters.size(); ++j) v[j] = ramps[j].next();
            const float inputFade = ingress.next();
            const std::array<float, 2> dry{safe(left[i]), safe(right[i])};
            std::array<float, 2> wet = dry;
            float mix = 1;
            if (kind == Kind::compressor) {
                const double peak = std::max(std::abs(dry[0]), std::abs(dry[1]));
                const double seconds = (peak > envelope ? v[2] : v[3]) * .001;
                const double pole = std::exp(-1 / (rate * seconds));
                envelope = pole * envelope + (1-pole) * peak;
                const double db = 20 * std::log10(std::max(envelope, 1e-12));
                const double reduction = std::max(0.0, db - v[0]) * (1 - 1 / v[1]);
                const float gain = static_cast<float>(std::pow(10, (v[4]-reduction)/20));
                for (unsigned c = 0; c < 2; ++c) wet[c] = dry[c] * gain;
                mix = v[5] * .01f;
            } else if (kind == Kind::amp) {
                const double pole = std::exp(-1 / (rate * .08));
                envelope = pole * envelope + (1-pole) * std::max(std::abs(dry[0]), std::abs(dry[1]));
                const float drive = std::pow(10.f, v[0]/20) / (1 + 5*v[5]*.01f*static_cast<float>(envelope));
                const double tone = std::exp(-tau * std::min(rate*.4, 1800.0 + 5000*v[4]*.01) / rate);
                for (unsigned c = 0; c < 2; ++c) {
                    const double x = std::tanh(dry[c]*drive);
                    states[c].low = safe(static_cast<float>((1-tone)*x + tone*states[c].low));
                    wet[c] = static_cast<float>(states[c].low);
                }
                equalizer.process(&wet[0], &wet[1], 1);
                for (auto& x : wet) x *= std::pow(10.f, v[6]/20);
                mix = v[7]*.01f;
            } else if (kind == Kind::fuzz) {
                const double tone = std::exp(-tau * std::min(rate*.4, static_cast<double>(v[1]))/rate);
                const double dc = std::exp(-tau*20/rate), gatePole = std::exp(-1/(rate*.005));
                for (unsigned c = 0; c < 2; ++c) {
                    auto& s = states[c];
                    const double gain = std::pow(10.f, v[0]/20);
                    const double x = dry[c]*gain + v[3]*.01;
                    const double soft = std::tanh(x), folded = std::sin(std::clamp(x, -1e4, 1e4));
                    const double character = v[2]*.01;
                    const double shaped = (1-character)*soft + character*folded;
                    const double threshold = v[4]*.0005;
                    const double gateTarget = std::abs(dry[c]) >= threshold ? 1 : 0;
                    s.gate = gatePole*s.gate + (1-gatePole)*gateTarget;
                    s.dc = safe(static_cast<float>(dc*(s.dc + shaped-s.previous))); s.previous = shaped;
                    s.low = safe(static_cast<float>((1-tone)*s.dc + tone*s.low));
                    wet[c] = static_cast<float>(s.low*s.gate) * std::pow(10.f,v[5]/20);
                }
                mix = v[6]*.01f;
            } else if (kind == Kind::chorus || kind == Kind::pitch) {
                const double speed = kind == Kind::chorus ? v[0]/rate
                    : (1-std::pow(2.0, (v[0]+v[1]*.01)/12))/(rate*.11);
                phase += speed; phase -= std::floor(phase);
                for (unsigned c = 0; c < 2; ++c) {
                    rings[c][write] = dry[c]*inputFade;
                    const double stereoPhase = phase + c*v[2]*.005;
                    if (kind == Kind::chorus) {
                        const double time = (.012 + .008*v[1]*.01*std::sin(tau*stereoPhase))*rate;
                        wet[c] = read(c, time);
                    } else {
                        double a = stereoPhase - std::floor(stereoPhase);
                        double b = a + .5; b -= std::floor(b);
                        const double weight = std::pow(std::sin(std::numbers::pi*a), 2);
                        wet[c] = static_cast<float>(weight*read(c, (.008+.11*a)*rate)
                            + (1-weight)*read(c, (.008+.11*b)*rate));
                        if (v[0] == 0 && v[1] == 0) wet[c] = dry[c];
                    }
                }
                if (++write == rings[0].size()) write = 0;
                mix = v[3]*.01f;
            } else if (kind == Kind::tremolo || kind == Kind::ring) {
                phase += v[0]/rate; phase -= std::floor(phase);
                for (unsigned c = 0; c < 2; ++c) {
                    const double wave = std::sin(tau*(phase+c*(kind == Kind::tremolo ? v[3] : v[2])*.005));
                    if (kind == Kind::tremolo) {
                        const double shape = v[2]*.01;
                        const double lfo = .5+.5*((1-shape)*wave + shape*std::tanh(8*wave));
                        wet[c] = static_cast<float>(dry[c]*(1-v[1]*.01+v[1]*.01*lfo));
                    } else wet[c] = static_cast<float>(dry[c]*wave);
                }
                mix = (kind == Kind::tremolo ? v[4] : v[1])*.01f;
            } else if (kind == Kind::filter) {
                phase += v[3]/rate; phase -= std::floor(phase);
                const double peak = std::max(std::abs(dry[0]),std::abs(dry[1]));
                const double pole = std::exp(-1/(rate*(peak>envelope?.008:.12)));
                envelope = pole*envelope+(1-pole)*peak;
                const double octave = 4*v[2]*.01*std::clamp(envelope*8,0.0,1.0)+2*v[4]*.01*std::sin(tau*phase);
                const double cutoff = std::clamp(v[0]*std::pow(2,octave),40.0,rate*.25);
                const double g = std::tan(std::numbers::pi*cutoff/rate), k = 1/(.5+9.5*v[1]*.01);
                const double a = 1/(1+g*(g+k));
                for(unsigned c=0;c<2;++c) {
                    auto& s=states[c];
                    const double v3 = dry[c]-s.ic2, v1=a*s.ic1+g*a*v3, v2=s.ic2+g*a*s.ic1+g*g*a*v3;
                    s.ic1=safe(static_cast<float>(2*v1-s.ic1)); s.ic2=safe(static_cast<float>(2*v2-s.ic2));
                    wet[c]=safe(static_cast<float>(v2));
                }
                mix=v[5]*.01f;
            }
            left[i]=safe((1-mix)*dry[0]+mix*wet[0]);
            right[i]=safe((1-mix)*dry[1]+mix*wet[1]);
        }
    }
private:
    struct State { double low=0, dc=0, previous=0, gate=0, ic1=0, ic2=0; };
    void controls(bool immediate) noexcept {
        for (std::size_t i=0;i<parameters.size();++i) {
            const auto& p=parameters[i]; float x=p.denormalize(bank.value(slot,p.bankIndex));
            if (immediate) ramps[i].reset(x); else ramps[i].aim(x,duration);
        }
    }
    float read(unsigned c, double delay) const noexcept {
        const auto size=rings[c].size();
        double position=static_cast<double>(write)-std::clamp(delay,1.0,static_cast<double>(size-2));
        if(position<0) position+=static_cast<double>(size);
        auto a=static_cast<std::size_t>(position), b=a+1==size?0:a+1;
        const float fraction=static_cast<float>(position-static_cast<double>(a));
        return (1-fraction)*rings[c][a]+fraction*rings[c][b];
    }
    Kind kind;
    const ParameterBank& bank;
    SlotId slot;
    std::vector<ParameterDescriptor> parameters;
    std::array<LinearRamp,10> ramps{};
    std::array<State,2> states{};
    std::array<std::vector<float>,2> rings;
    ParameterBank eqBank;
    EQProcessor equalizer;
    LinearRamp ingress;
    double rate=48000, phase=0, envelope=0;
    int duration=960;
    std::size_t write=0;
};
EffectDescriptor make(Kind kind, const char* id, const char* name, const char* category, std::vector<ParameterDescriptor> p) {
    EffectDescriptor result{id,name,category,1,p,{}};
    result.factory=[kind,p=std::move(p)](const ParameterBank& bank,SlotId slot) {
        return std::make_unique<Experimental>(kind,bank,slot,p);
    };
    return result;
}
}
EffectDescriptor fuzzDescriptor() {
    return make(Kind::fuzz,"fuzz","Fuzz","Drive",{
        {"drive_db","Drive","dB",0,0,48,18},{"tone_hz","Tone","Hz",1,400,8000,2400},
        {"character_percent","Fold","%",2,0,100,0},{"bias_percent","Bias","%",3,-50,50,0},
        {"gate_percent","Gate","%",4,0,100,5},{"level_db","Level","dB",5,-30,6,-12},
        {"mix_percent","Mix","%",6,0,100,60}});
}
EffectDescriptor ampDescriptor() {
    return make(Kind::amp,"amp","Basic Amp","Amp",{
        {"drive_db","Drive","dB",0,0,36,3},{"bass_db","Bass","dB",1,-12,12,0},
        {"mid_db","Mid","dB",2,-12,12,0},{"treble_db","Treble","dB",3,-12,12,0},
        {"presence_percent","Presence","%",4,0,100,45},{"sag_percent","Sag","%",5,0,100,20},
        {"level_db","Level","dB",6,-30,6,-6},{"mix_percent","Mix","%",7,0,100,100}});
}
EffectDescriptor compressorDescriptor() {
    return make(Kind::compressor,"compressor","Compressor","Dynamics",{
        {"threshold_db","Threshold","dB",0,-48,0,-24},{"ratio","Ratio",":1",1,1,12,3},
        {"attack_ms","Attack","ms",2,.5,100,10},{"release_ms","Release","ms",3,10,1000,150},
        {"makeup_db","Makeup","dB",4,0,18,0},{"mix_percent","Mix","%",5,0,100,75}});
}
EffectDescriptor chorusDescriptor() {
    return make(Kind::chorus,"chorus","Chorus","Modulation",{
        {"rate_hz","Rate","Hz",0,.05,8,.7},{"depth_percent","Depth","%",1,0,100,35},
        {"width_percent","Width","%",2,0,100,75},{"mix_percent","Mix","%",3,0,100,25}});
}
EffectDescriptor tremoloDescriptor() {
    return make(Kind::tremolo,"tremolo","Tremolo","Modulation",{
        {"rate_hz","Rate","Hz",0,.05,20,4},{"depth_percent","Depth","%",1,0,100,50},
        {"shape_percent","Chop","%",2,0,100,0},{"width_percent","Stereo","%",3,0,100,0},
        {"mix_percent","Mix","%",4,0,100,100}});
}
EffectDescriptor filterDescriptor() {
    return make(Kind::filter,"filter","Resonant Filter","Filter",{
        {"cutoff_hz","Cutoff","Hz",0,100,6000,1800},{"resonance_percent","Resonance","%",1,0,90,20},
        {"envelope_percent","Auto Wah","%",2,0,100,0},{"rate_hz","Rate","Hz",3,.05,12,1},
        {"depth_percent","Sweep","%",4,0,100,0},{"mix_percent","Mix","%",5,0,100,100}});
}
EffectDescriptor pitchDescriptor() {
    return make(Kind::pitch,"pitch","Pitch Shifter","Pitch",{
        {"semitones","Shift","st",0,-12,12,0},{"cents","Detune","cent",1,-50,50,0},
        {"width_percent","Width","%",2,0,100,0},{"mix_percent","Mix","%",3,0,100,25}});
}
EffectDescriptor ringDescriptor() {
    return make(Kind::ring,"ring_mod","Ring Modulator","Modulation",{
        {"rate_hz","Carrier","Hz",0,20,2000,120},{"mix_percent","Mix","%",1,0,100,35},
        {"width_percent","Stereo","%",2,0,100,0}});
}
}
