#include "Assets.h"
#include <algorithm>
#include <cerrno>
#include <cstring>
#include <set>
#include <stdexcept>
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>

namespace guitarrack {
namespace {
[[noreturn]] void fail(const std::string& e) { throw std::runtime_error(e); }
struct FD { int n; ~FD(){if(n>=0)::close(n);} };
std::vector<std::uint8_t> read(const std::filesystem::path& p,std::size_t limit) {
    FD f{::open(p.c_str(),O_RDONLY|O_CLOEXEC|O_NONBLOCK|O_NOFOLLOW)}; struct stat st{};
    if(f.n<0||::fstat(f.n,&st)||!S_ISREG(st.st_mode)) fail("Cannot read regular file (links are unsupported): "+p.string());
    if(st.st_size<0||static_cast<std::uintmax_t>(st.st_size)>limit) fail("File exceeds supported size: "+p.string());
    std::vector<std::uint8_t> b(static_cast<std::size_t>(st.st_size)); std::size_t used=0;
    while(used<b.size()) { auto n=::read(f.n,b.data()+used,b.size()-used); if(n<0&&errno==EINTR) continue; if(n<=0) fail("File is truncated or unreadable: "+p.string()); used+=static_cast<std::size_t>(n); }
    std::uint8_t extra; if(::read(f.n,&extra,1)!=0) fail("File changed while reading: "+p.string()); return b;
}
void safeDirectory(const std::filesystem::path& p) {
    std::filesystem::path accumulated;
    for(const auto& part:std::filesystem::absolute(p).lexically_normal()) {
        accumulated/=part; auto st=std::filesystem::symlink_status(accumulated);
        if(std::filesystem::is_symlink(st)) fail("Managed IR directory cannot contain links: "+accumulated.string());
    }
    std::filesystem::create_directories(p);
}
void write(const std::filesystem::path& p,std::span<const std::uint8_t> b) {
    if(b.size()>maxBundleBytes) fail("Asset/bundle exceeds 17 MiB.");
    auto dir=p.parent_path(); if(dir.empty()) dir=".";
    safeDirectory(dir);
    FD folder{::open(dir.c_str(),O_RDONLY|O_DIRECTORY|O_CLOEXEC|O_NOFOLLOW)};
    if(folder.n<0) fail("Cannot open IR/bundle directory: "+dir.string());
    auto name=p.filename().string(),temp=".guitarrack-asset-"+newInstanceId();
    FD f{::openat(folder.n,temp.c_str(),O_WRONLY|O_CREAT|O_EXCL|O_CLOEXEC,0600)};
    if(f.n<0) fail("Cannot create IR/bundle temporary file: "+dir.string());
    try {
        std::size_t used=0;
        while(used<b.size()) { auto n=::write(f.n,b.data()+used,b.size()-used); if(n<0&&errno==EINTR) continue; if(n<=0) fail("IR/bundle write failed: "+std::string(std::strerror(errno))); used+=static_cast<std::size_t>(n); }
        if(::fsync(f.n)) fail("IR/bundle sync failed.");
        if(::renameat(folder.n,temp.c_str(),folder.n,name.c_str())) fail("IR/bundle atomic replacement failed.");
        if(::fsync(folder.n)) fail("Directory sync failed; IR/bundle durability uncertain.");
    } catch(...) { ::unlinkat(folder.n,temp.c_str(),0); throw; }
}
std::set<std::string> ids(const ToneState& t) { std::set<std::string> ids; for(const auto& i:t.rack.instances) if(!i.irAsset.empty()) ids.insert(i.irAsset); return ids; }
void append(std::vector<std::uint8_t>& b,std::uint32_t v,unsigned n) { for(unsigned i=0;i<n;++i) b.push_back(static_cast<std::uint8_t>(v>>(8*i))); }
void append(std::vector<std::uint8_t>& b,const std::string& s) { b.insert(b.end(),s.begin(),s.end()); }
class Reader {
public:
    explicit Reader(std::span<const std::uint8_t> b):b(b){}
    std::span<const std::uint8_t> take(std::size_t n) { if(n>b.size()-p) fail("Truncated portable bundle."); auto out=b.subspan(p,n); p+=n; return out; }
    std::uint32_t number(unsigned n) { auto s=take(n); std::uint32_t v=0; for(unsigned i=0;i<n;++i) v|=std::uint32_t(s[i])<<(i*8); return v; }
    std::string text(std::size_t n) { auto s=take(n); return std::string(reinterpret_cast<const char*>(s.data()),s.size()); }
    bool done()const{return p==b.size();}
private: std::span<const std::uint8_t> b; std::size_t p=0;
};
}
std::vector<std::uint8_t> AssetStore::bytes(const std::string& id) const {
    if(!validAssetId(id)) fail("Invalid IR checksum identity.");
    if(id==syntheticIRId()) return syntheticIRBytes();
    std::vector<std::uint8_t> b;
    try { safeDirectory(directory); b=read(directory/(id+".wav"),maxIRBytes); }
    catch(const std::exception& e) { fail("IR asset unavailable ("+id.substr(0,12)+"). Import its portable bundle or load the original mono WAV again. "+e.what()); }
    if(sha256(b)!=id) fail("IR checksum mismatch; reimport the WAV or portable bundle: "+id);
    return b;
}
std::string AssetStore::install(std::span<const std::uint8_t> b) const {
    decodeIR(b); auto id=sha256(b); if(id==syntheticIRId()) return id;
    safeDirectory(directory); auto p=directory/(id+".wav"); auto st=std::filesystem::symlink_status(p);
    if(std::filesystem::exists(st)) { auto old=read(p,maxIRBytes); if(sha256(old)!=id) fail("Managed IR checksum mismatch; restore the asset before retrying."); }
    else write(p,b);
    return id;
}
std::string AssetStore::importIR(const std::filesystem::path& p) const { return install(read(p,maxIRBytes)); }
void AssetStore::validateAssets(const ToneState& t) const { for(const auto& id:ids(t)) resolve(id); }
void AssetStore::exportBundle(const std::filesystem::path& p,const ToneState& t,const EffectRegistry& r) const {
    auto text=serializePreset(t,r); std::vector<std::uint8_t> b; append(b,"GRBNDL2\n"); append(b,text.size(),4); append(b,text);
    auto assets=ids(t); append(b,assets.size(),4);
    for(const auto& id:assets) { auto bytes=this->bytes(id); decodeIR(bytes); auto name="ir/"+id+".wav"; append(b,name.size(),2); append(b,name); append(b,bytes.size(),4); b.insert(b.end(),bytes.begin(),bytes.end()); }
    write(p,b);
}
ToneState AssetStore::importPreset(const std::filesystem::path& p,const EffectRegistry& r) const {
    auto b=read(p,maxBundleBytes);
    if(b.size()<8||std::memcmp(b.data(),"GRBNDL2\n",8)!=0) {
        if(b.size()>maxPresetBytes) fail("Preset JSON exceeds 128 KiB (portable bundles use .grbundle).");
        auto t=parsePreset(std::string(reinterpret_cast<const char*>(b.data()),b.size()),r); validateAssets(t); return t;
    }
    Reader reader(b); reader.take(8); auto n=reader.number(4); if(n>maxPresetBytes) fail("Bundle preset JSON exceeds 128 KiB.");
    auto t=parsePreset(reader.text(n),r); auto required=ids(t), seen=std::set<std::string>{}; auto count=reader.number(4);
    if(count>rackSlots||count!=required.size()) fail("Bundle must contain exactly its required IR assets (maximum 16).");
    std::vector<std::vector<std::uint8_t>> validated;
    // Validate the ENTIRE bundle before managed writes; no arbitrary extraction,
    // paths, link records, compression bombs or partially decoded running state.
    for(unsigned i=0;i<count;++i) {
        auto length=reader.number(2); if(length!=71) fail("Unsafe bundle asset path; expected ir/<sha256>.wav.");
        auto name=reader.text(length), id=name.substr(3,64);
        if(!validAssetId(id)||name!="ir/"+id+".wav"||!required.contains(id)||!seen.insert(id).second)
            fail("Unsafe, duplicate or unreferenced bundle asset path.");
        auto size=reader.number(4); if(size>maxIRBytes) fail("Bundle WAV exceeds 1 MiB."); auto data=reader.take(size);
        if(sha256(data)!=id) fail("Bundle IR checksum mismatch.");
        decodeIR(data); validated.emplace_back(data.begin(),data.end());
    }
    if(!reader.done()) fail("Trailing or unsupported bundle records (links are unsupported).");
    for(const auto& asset:validated) install(asset);
    validateAssets(t); return t;
}
}
