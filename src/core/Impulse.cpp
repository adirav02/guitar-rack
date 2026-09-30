#include "Impulse.h"
#include <algorithm>
#include <array>
#include <bit>
#include <cmath>
#include <cstring>
#include <numbers>
#include <stdexcept>

namespace guitarrack {
namespace {
std::uint32_t le(std::span<const std::uint8_t> b, std::size_t p, unsigned n) {
    if (p > b.size() || n > b.size()-p) throw std::runtime_error("Truncated WAV; choose a complete mono WAV IR.");
    std::uint32_t v=0; for(unsigned i=0;i<n;++i) v|=std::uint32_t(b[p+i])<<(8*i); return v;
}
bool tag(std::span<const std::uint8_t> b,std::size_t p,const char* s) { return p+4<=b.size() && std::memcmp(b.data()+p,s,4)==0; }
void validate(const ImpulseResponse& ir) {
    if(!std::isfinite(ir.sampleRate)||ir.sampleRate<8000||ir.sampleRate>192000)
        throw std::runtime_error("IR sample rate must be 8000–192000 Hz.");
    if(ir.samples.empty()||ir.samples.size()>maxIRSourceSamples||ir.samples.size()>ir.sampleRate*.5)
        throw std::runtime_error("IR must contain 1–500 ms of mono samples (maximum 96000 samples).");
    double energy=0, sum=0;
    for(float x:ir.samples) {
        if(!std::isfinite(x)||std::abs(x)>1) throw std::runtime_error("IR contains non-finite or out-of-full-scale samples; export a valid WAV at or below full scale.");
        energy+=double(x)*x; sum+=std::abs(double(x));
    }
    if(energy<=1e-16) throw std::runtime_error("IR is empty or silent; choose a non-silent cabinet impulse.");
    if(sum>128) throw std::runtime_error("IR absolute gain exceeds the supported bound (128); reduce its level before loading.");
}
}
ImpulseResponse decodeIR(std::span<const std::uint8_t> b) {
    if(b.size()>maxIRBytes) throw std::runtime_error("IR WAV exceeds 1 MiB.");
    if(b.size()<44||!tag(b,0,"RIFF")||!tag(b,8,"WAVE")||le(b,4,4)!=b.size()-8)
        throw std::runtime_error("Not a complete little-endian RIFF WAV; use mono PCM16/24/32 or float32.");
    unsigned format=0,bits=0,rate=0,align=0; std::size_t data=0,length=0; bool fmt=false;
    for(std::size_t p=12;p<b.size();) {
        if(b.size()-p<8) throw std::runtime_error("Truncated WAV chunk.");
        auto n=std::size_t(le(b,p+4,4)); if(n>b.size()-p-8) throw std::runtime_error("WAV chunk exceeds file size.");
        if(tag(b,p,"fmt ")) {
            if(fmt||(n!=16&&n!=18)||(n==18&&le(b,p+24,2)!=0)) throw std::runtime_error("Unsupported or duplicate WAV format; export standard mono PCM or float32 WAV.");
            fmt=true; format=le(b,p+8,2); auto channels=le(b,p+10,2); rate=le(b,p+12,4); auto bytesPerSecond=le(b,p+16,4);
            align=le(b,p+20,2); bits=le(b,p+22,2);
            if(channels!=1) throw std::runtime_error("Cabinet IR must be mono; stereo/multichannel WAV is unsupported.");
            if(!((format==1&&(bits==16||bits==24||bits==32))||(format==3&&bits==32))||align!=bits/8||bytesPerSecond!=rate*align)
                throw std::runtime_error("Unsupported WAV encoding/header; use PCM16/24/32 or IEEE float32 mono.");
        } else if(tag(b,p,"data")) {
            if(data) throw std::runtime_error("Duplicate WAV data chunk.");
            data=p+8; length=n;
        }
        auto step=8+n+(n&1); if(step>b.size()-p) throw std::runtime_error("Missing WAV chunk padding."); p+=step;
    }
    if(!fmt||!data||!align||length%align) throw std::runtime_error("Missing or malformed WAV format/data.");
    if(rate<8000||rate>192000||length/align>maxIRSourceSamples||length/align>rate/2)
        throw std::runtime_error("IR requires 8000–192000 Hz and at most 500 ms / 96000 mono samples.");
    ImpulseResponse ir; ir.sampleRate=rate; ir.samples.resize(length/align);
    for(std::size_t i=0;i<ir.samples.size();++i) {
        auto v=le(b,data+i*align,align);
        if(format==3) ir.samples[i]=std::bit_cast<float>(v);
        else { if(bits<32 && (v&(1u<<(bits-1)))) v|=(~0u)<<bits; ir.samples[i]=float(std::bit_cast<std::int32_t>(v)/std::ldexp(1.0,bits-1)); }
    }
    validate(ir); return ir;
}
ImpulseResponse resampleIR(const ImpulseResponse& ir,double target) {
    validate(ir);
    if(!std::isfinite(target)||target<8000||target>768000) throw std::runtime_error("Unsupported processing sample rate for IR.");
    if(target==ir.sampleRate) return ir;
    ImpulseResponse out; out.sampleRate=target;
    auto n=static_cast<std::size_t>(std::ceil(ir.samples.size()*target/ir.sampleRate));
    if(n>384000) throw std::runtime_error("Resampled IR exceeds 384000 samples.");
    out.samples.resize(n);
    const double cutoff=std::min(1.0,target/ir.sampleRate), width=32/cutoff, ratio=ir.sampleRate/target;
    // Windowed-sinc interpolation, zero outside the original support. No peak,
    // energy or DC normalization: scale by source/target to preserve FIR gain.
    for(std::size_t i=0;i<n;++i) {
        const double position=i*ratio; double y=0;
        auto first=std::max<long long>(0,static_cast<long long>(std::ceil(position-width)));
        auto last=std::min<long long>(ir.samples.size()-1,static_cast<long long>(std::floor(position+width)));
        for(auto j=first;j<=last;++j) {
            auto d=position-j, x=d*cutoff;
            auto sinc=std::abs(x)<1e-12?1.0:std::sin(std::numbers::pi*x)/(std::numbers::pi*x);
            y+=ir.samples[static_cast<std::size_t>(j)]*cutoff*sinc*(.5+.5*std::cos(std::numbers::pi*d/width));
        }
        out.samples[i]=static_cast<float>(y*ratio);
    }
    return out;
}
bool validAssetId(const std::string& s) noexcept {
    if(s.size()!=64) return false;
    for(char c:s) if(!((c>='0'&&c<='9')||(c>='a'&&c<='f'))) return false;
    return true;
}
std::string sha256(std::span<const std::uint8_t> bytes) {
    constexpr std::array<std::uint32_t,64> k{
        0x428a2f98,0x71374491,0xb5c0fbcf,0xe9b5dba5,0x3956c25b,0x59f111f1,0x923f82a4,0xab1c5ed5,
        0xd807aa98,0x12835b01,0x243185be,0x550c7dc3,0x72be5d74,0x80deb1fe,0x9bdc06a7,0xc19bf174,
        0xe49b69c1,0xefbe4786,0x0fc19dc6,0x240ca1cc,0x2de92c6f,0x4a7484aa,0x5cb0a9dc,0x76f988da,
        0x983e5152,0xa831c66d,0xb00327c8,0xbf597fc7,0xc6e00bf3,0xd5a79147,0x06ca6351,0x14292967,
        0x27b70a85,0x2e1b2138,0x4d2c6dfc,0x53380d13,0x650a7354,0x766a0abb,0x81c2c92e,0x92722c85,
        0xa2bfe8a1,0xa81a664b,0xc24b8b70,0xc76c51a3,0xd192e819,0xd6990624,0xf40e3585,0x106aa070,
        0x19a4c116,0x1e376c08,0x2748774c,0x34b0bcb5,0x391c0cb3,0x4ed8aa4a,0x5b9cca4f,0x682e6ff3,
        0x748f82ee,0x78a5636f,0x84c87814,0x8cc70208,0x90befffa,0xa4506ceb,0xbef9a3f7,0xc67178f2};
    std::array<std::uint32_t,8> h{0x6a09e667,0xbb67ae85,0x3c6ef372,0xa54ff53a,0x510e527f,0x9b05688c,0x1f83d9ab,0x5be0cd19};
    const auto padded=((bytes.size()+9+63)/64)*64;
    for(std::size_t p=0;p<padded;p+=64) {
        std::array<std::uint32_t,64> w{};
        for(unsigned i=0;i<64;++i) {
            auto at=p+i; std::uint8_t v=0;
            if(at<bytes.size()) v=bytes[at]; else if(at==bytes.size()) v=128;
            else if(at>=padded-8) v=static_cast<std::uint8_t>((std::uint64_t(bytes.size())*8)>>((padded-1-at)*8));
            w[i/4]|=std::uint32_t(v)<<(24-8*(i%4));
        }
        for(unsigned i=16;i<64;++i) {
            auto s0=std::rotr(w[i-15],7)^std::rotr(w[i-15],18)^(w[i-15]>>3);
            auto s1=std::rotr(w[i-2],17)^std::rotr(w[i-2],19)^(w[i-2]>>10);
            w[i]=w[i-16]+s0+w[i-7]+s1;
        }
        auto a=h[0],b=h[1],c=h[2],d=h[3],e=h[4],f=h[5],g=h[6],hh=h[7];
        for(unsigned i=0;i<64;++i) {
            auto t1=hh+(std::rotr(e,6)^std::rotr(e,11)^std::rotr(e,25))+((e&f)^((~e)&g))+k[i]+w[i];
            auto t2=(std::rotr(a,2)^std::rotr(a,13)^std::rotr(a,22))+((a&b)^(a&c)^(b&c));
            hh=g; g=f; f=e; e=d+t1; d=c; c=b; b=a; a=t1+t2;
        }
        h[0]+=a; h[1]+=b; h[2]+=c; h[3]+=d; h[4]+=e; h[5]+=f; h[6]+=g; h[7]+=hh;
    }
    std::string result; result.reserve(64); const char* hex="0123456789abcdef";
    for(auto v:h) for(int s=28;s>=0;s-=4) result.push_back(hex[(v>>s)&15]);
    return result;
}
std::vector<std::uint8_t> syntheticIRBytes() {
    // Original analytic damped low-pass/band-pass filter, NOT a measured cabinet.
    std::vector<std::uint8_t> b(44+512*4);
    auto put=[&](std::size_t p,std::uint32_t v,unsigned n) { for(unsigned i=0;i<n;++i) b[p+i]=static_cast<std::uint8_t>(v>>(8*i)); };
    std::memcpy(b.data(),"RIFF",4); put(4,b.size()-8,4); std::memcpy(b.data()+8,"WAVEfmt ",8);
    put(16,16,4); put(20,3,2); put(22,1,2); put(24,48000,4); put(28,48000*4,4); put(32,4,2); put(34,32,2);
    std::memcpy(b.data()+36,"data",4); put(40,512*4,4);
    double lp=0,hp=0; const double p=std::exp(-2*std::numbers::pi*4200/48000), q=std::exp(-2*std::numbers::pi*90/48000);
    for(unsigned i=0;i<512;++i) {
        lp=(1-p)*(i==0?1.0:0.0)+p*lp; hp=(1-q)*lp+q*hp;
        float x=static_cast<float>(lp-hp); put(44+i*4,std::bit_cast<std::uint32_t>(x),4);
    }
    return b;
}
const std::string& syntheticIRId() { static const std::string id=sha256(syntheticIRBytes()); return id; }
}
