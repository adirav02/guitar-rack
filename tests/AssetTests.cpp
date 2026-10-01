#include "core/CabinetAssets.h"
#include "state/Assets.h"
#include "state/PresetSession.h"
#include "core/MonitorEngine.h"
#include "HeapAudit.h"
#include <algorithm>
#include <atomic>
#include <bit>
#include <chrono>
#include <fstream>
#include <iostream>
#include <thread>
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>

using namespace guitarrack;
namespace {
int failures=0;
void check(bool b,const char* s){if(!b){++failures;std::cerr<<"FAIL: "<<s<<'\n';}}
template<class F>void rejects(F f,const char* s){try{f();check(false,s);}catch(const std::exception&){}}
template<class F>bool wait(F f){auto end=std::chrono::steady_clock::now()+std::chrono::seconds(8);do{if(f())return true;std::this_thread::sleep_for(std::chrono::milliseconds(1));}while(std::chrono::steady_clock::now()<end);return false;}
void writeBytes(const std::filesystem::path& p,const std::vector<std::uint8_t>& b){std::ofstream f(p,std::ios::binary);f.write(reinterpret_cast<const char*>(b.data()),b.size());}
std::vector<std::uint8_t> readBytes(const std::filesystem::path& p){std::ifstream f(p,std::ios::binary);return {std::istreambuf_iterator<char>(f),std::istreambuf_iterator<char>()};}
std::vector<std::uint8_t> userIR(float scale=1){auto b=syntheticIRBytes();for(std::size_t p=44;p<b.size();p+=4){unsigned v=0;for(unsigned i=0;i<4;++i)v|=unsigned(b[p+i])<<(8*i);float x=std::bit_cast<float>(v)*scale;v=std::bit_cast<unsigned>(x);for(unsigned i=0;i<4;++i)b[p+i]=v>>(8*i);}return b;}
ToneState cabinetTone(const EffectRegistry& r,const std::string& asset){auto t=factoryPresets(r)[3];t.id=newInstanceId();t.name="User cabinet 🎸";for(auto& i:t.rack.instances)if(i.effectType=="cabinet")i.irAsset=asset;return t;}
void bundles(const std::filesystem::path& root,const EffectRegistry& registry) {
    AssetStore a(root/"managed"/"ir"),b(root/"other"/"ir");auto external=root/"personal-original.wav";auto bytes=userIR(.7f);writeBytes(external,bytes);
    auto id=a.importIR(external);check(id==sha256(bytes)&&a.bytes(id)==bytes,"managed import preserves exact bytes/level/checksum");
    auto tone=cabinetTone(registry,id);auto json=serializePreset(tone,registry);check(json.find(root.string())==std::string::npos&&json.find("personal-original")==std::string::npos,"preset contains only relative checksum asset references");
    auto round=parsePreset(json,registry);check(round.rack==tone.rack&&round.schemaVersion==2,"v2 cabinet identity/configuration round trip");
    auto bundle=root/"portable.grbundle";a.exportBundle(bundle,tone,registry);
    std::filesystem::remove(external);std::filesystem::remove_all(a.directory);auto restored=b.importPreset(bundle,registry);
    check(restored.rack==tone.rack&&b.bytes(id)==bytes,"portable import works after external and source managed IR disappear");
    b.validateAssets(restored);check(decodeIR(b.bytes(id)).samples==decodeIR(bytes).samples,"bundle preserves unnormalized IR samples");
    for(const auto& cabinet:builtinCabinets()) {
        auto original=builtinIRBytes(cabinet.id);auto builtInTone=cabinetTone(registry,cabinet.id);
        auto portable=root/(cabinet.name+".grbundle");a.exportBundle(portable,builtInTone,registry);
        auto imported=b.importPreset(portable,registry);b.validateAssets(imported);
        check(imported.rack==builtInTone.rack&&b.bytes(cabinet.id)==original,"all new embedded cabinet bundles preserve exact portable bytes/identity");
    }
    auto full=readBytes(bundle);
    auto invalid=[&](auto edit,const char* name){auto bad=full;edit(bad);writeBytes(root/"bad.grbundle",bad);rejects([&]{b.importPreset(root/"bad.grbundle",registry);},name);};
    auto u32=[](const auto& v,std::size_t p){unsigned n=0;for(unsigned i=0;i<4;++i)n|=unsigned(v[p+i])<<(8*i);return n;};auto offset=12+u32(full,8)+4; // first entry name length
    invalid([&](auto& v){v[offset+2]='.';v[offset+3]='.';},"bundle traversal rejected");
    invalid([&](auto& v){v[offset+2]='/';},"absolute bundle path rejected");
    invalid([](auto& v){v.back()^=1;},"bundle checksum failure rejected");
    invalid([](auto& v){v.pop_back();},"truncated bundle rejected");
    invalid([](auto& v){v.push_back(0);},"unsupported trailing/link record rejected");
    invalid([&](auto& v){auto p=offset+2+71;for(unsigned i=0;i<4;++i)v[p+i]=255;},"oversized entry rejected before allocation");
    invalid([&](auto& v){auto p=12+u32(v,8);v[p]=17;},"entry count resource bound rejected");
    auto status=parseJson(json);auto* manifest=(*status["assets"].getArray())[0].getDynamicObject();manifest->setProperty("relative_path","../../outside.wav");rejects([&]{parsePreset(juce::JSON::toString(status).toStdString(),registry);},"preset manifest traversal rejected");
    atomicWrite(root/"local.json",json);AssetStore missing(root/"missing"/"ir");rejects([&]{missing.importPreset(root/"local.json",registry);},"JSON cannot silently substitute a missing cabinet");
    try{missing.resolve(id);check(false,"missing IR should fail");}catch(const std::exception& e){check(std::string(e.what()).find("portable bundle")!=std::string::npos,"missing IR error explains bundle/original WAV recovery");}
    auto link=root/"linked.wav";std::filesystem::create_symlink(b.directory/(id+".wav"),link);rejects([&]{b.importIR(link);},"external IR link rejected");
    auto real=root/"real-ir";std::filesystem::create_directory(real);std::filesystem::create_directory_symlink(real,root/"linked-ir");AssetStore unsafe(root/"linked-ir");rejects([&]{unsafe.install(bytes);},"managed directory links rejected");
    auto managed=b.directory/(id+".wav");std::filesystem::remove(managed);std::filesystem::create_symlink(bundle,managed);rejects([&]{b.resolve(id);},"managed asset link rejected");std::filesystem::remove(managed);b.install(bytes);
    auto changed=bytes;changed.back()^=1;writeBytes(managed,changed);rejects([&]{b.resolve(id);},"managed corruption rejected");std::filesystem::remove(managed);b.install(bytes);
    writeBytes(root/"silent.wav",std::vector<std::uint8_t>(44));rejects([&]{b.importIR(root/"silent.wav");},"malformed WAV import rejected");
    check(::mkfifo((root/"fifo.wav").c_str(),0600)==0,"create FIFO");rejects([&]{b.importIR(root/"fifo.wav");},"FIFO decode cannot block parser");
    writeBytes(root/"too-large.grbundle",std::vector<std::uint8_t>(maxBundleBytes+1));rejects([&]{b.importPreset(root/"too-large.grbundle",registry);},"bundle total size bound enforced");
    for(unsigned bits:{16u,24u,32u}) {
        auto pcm=syntheticIRBytes();pcm.resize(44+bits/8);auto put=[&](unsigned p,unsigned v,unsigned n){for(unsigned i=0;i<n;++i)pcm[p+i]=v>>(8*i);};put(4,pcm.size()-8,4);put(20,1,2);put(28,48000*bits/8,4);put(32,bits/8,2);put(34,bits,2);put(40,bits/8,4);put(44,1u<<(bits-2),bits/8);
        if(bits==24){pcm.push_back(0);put(4,pcm.size()-8,4);}auto ir=decodeIR(pcm);check(ir.samples.size()==1&&ir.samples[0]==.5f,"mono PCM16/24/32 decoding");
    }
}
struct Audio {std::array<float,8193> in{},l{},r{};bool heap=true;void block(MonitorEngine& e,float x=.04f,int n=512){in.fill(x);heapAudit::begin();e.process(in.data(),in.data(),l.data(),r.data(),n);heap&=heapAudit::finish();}};
void sessionTransactions(const std::filesystem::path& root,const EffectRegistry& registry) {
    RackEngine rack;MonitorControls controls;MonitorEngine engine(controls,&rack);PresetSession session(rack,root/"session"/"presets");Audio audio;
    session.select(factoryPresets(registry)[3]);check(wait([&]{session.poll();return !session.busy();}),"stopped new factory commits");check(!session.dirty(),"new factory starts clean");
    check(engine.prepare(48000),"new combined rack prepares");for(int i=0;i<8;++i)audio.block(engine);
    auto oldSelection=*session.selected();auto cab=oldSelection.rack.order[1];auto before=session.current();auto file=root/"replace.wav";auto bytes=userIR(.5f);writeBytes(file,bytes);
    session.loadIR(cab,file);check(wait([&]{audio.block(engine);session.poll();return !session.busy();}),"live IR transaction completes");
    auto loaded=session.current();check(loaded.rack.find(cab)->irAsset==sha256(bytes)&&loaded.rack.find(cab)->slot==before.rack.find(cab)->slot&&loaded.rack.order==before.rack.order,"IR replacement preserves instance/slot/order");
    check(session.selected()->id==oldSelection.id&&session.selected()->rack==oldSelection.rack&&session.dirty(),"IR edit preserves selected preset/baseline and becomes dirty");
    auto savedBytes=serializePreset(session.current(),registry);session.loadIR(cab,root/"missing.wav");check(wait([&]{audio.block(engine);session.poll();return !session.busy();}),"failed live IR completes");check(!session.error.empty()&&serializePreset(session.current(),registry)==savedBytes&&session.dirty()&&session.selected()->id==oldSelection.id,"failed IR retains cabinet/rack/selection/dirty");
    auto slowBytes=userIR(.25f);writeBytes(root/"other.wav",slowBytes);session.loadIR(cab,root/"other.wav");check(session.cancelLoad(),"cancel IR before graph acceptance");for(int i=0;i<20;++i){session.poll();audio.block(engine);std::this_thread::sleep_for(std::chrono::milliseconds(1));}check(serializePreset(session.current(),registry)==savedBytes&&session.dirty(),"cancelled parser IR cannot overwrite previous rack");
    session.loadIR(cab,root/"other.wav");rack.setParameter(cab,0,-3);check(wait([&]{audio.block(engine);session.poll();return !session.busy();}),"IR request overtaken by control edit resolves");check(session.current().rack.find(cab)->irAsset==sha256(bytes)&&rack.bank.value(before.rack.find(cab)->slot,0)==.7f,"stale IR cannot overwrite latest controls/asset");
    session.loadIR(cab,root/"other.wav");rack.move(cab,-1);check(wait([&]{audio.block(engine);session.poll();return !session.busy();}),"IR request overtaken by reorder resolves");check(session.current().rack.order[0]==cab&&session.current().rack.find(cab)->irAsset==sha256(bytes),"stale IR cannot overwrite latest processing order");
    std::atomic<bool> entered{false},release{false}; auto slowId=sha256(slowBytes);
    auto assetStore=session.assets;
    rack.setAssetResolver([&](const std::string& asset){
        if(asset==slowId) {entered.store(true);while(!release.load())std::this_thread::sleep_for(std::chrono::milliseconds(1));}
        return assetStore.resolve(asset);
    });
    auto retained=serializePreset(session.current(),registry);
    session.loadIR(cab,root/"other.wav");check(wait([&]{audio.block(engine);session.poll();return entered.load();}),"IR decode succeeds and convolution worker blocks for stale-build fixture");
    check(session.cancelLoad(),"cancel already-building IR before acceptance");release.store(true);
    check(wait([&]{audio.block(engine);session.poll();return !session.busy();}),"cancelled worker settles");
    check(serializePreset(session.current(),registry)==retained&&session.dirty()&&session.selected()->id==oldSelection.id,"stale prepared IR retains rack/selection/dirty state");
    rack.setAssetResolver([assetStore,slowId](const std::string& asset){if(asset==slowId)throw std::runtime_error("Injected convolution preparation failure");return assetStore.resolve(asset);});
    session.loadIR(cab,root/"other.wav");check(wait([&]{audio.block(engine);session.poll();return !session.busy();}),"validated IR preparation failure resolves");
    check(serializePreset(session.current(),registry)==retained&&!session.error.empty()&&session.dirty(),"convolution preparation rollback retains controls and selected baseline");
    rack.setAssetResolver([assetStore](const std::string& asset){return assetStore.resolve(asset);});
    check(session.saveAs("IR preset 🎸"),"save managed IR preset");auto selected=*session.selected();auto id=selected.id;auto managedIR=session.assets.directory/(sha256(bytes)+".wav");
    check(session.exportFile(root/"session.grbundle"),"session exports required WAV bundle");
    check(session.deleteSelected(id,DirtyAction::discard,true),"delete managed IR preset");check(std::filesystem::exists(managedIR)&&session.current().rack==selected.rack&&session.dirty(),"preset deletion retains shared real IR/current sound");
    session.importFile(root/"session.grbundle");check(wait([&]{audio.block(engine);session.poll();return !session.busy();}),"explicit bundle import restores deleted tone");check(session.selected()&&session.selected()->id==id&&!session.dirty(),"bundle import selection/baseline commit together");
    controls.setMuted(true);audio.block(engine);audio.block(engine);audio.block(engine);audio.block(engine,0,8193);check(audio.heap&&std::all_of(audio.l.begin(),audio.l.end(),[](float x){return x==0;}),"combined new catalog Mute/oversized callback heap free");
    check(engine.prepare(44100),"combined asset-bearing rack actual-rate restart");audio.block(engine,0,1);audio.block(engine,0,8193);check(audio.heap,"resampled IR/new effects callback heap audit");
    auto snapshot=rack.snapshot();check(wait([&]{return rack.snapshot().graphs==1;})&&snapshot.peakGraphs<=4,"asset graphs reclaim within bound");
    // Missing assets during a full import fail without a sound/selection mutation.
    controls.setMuted(false);atomicWrite(root/"missing-asset.json",serializePreset(session.current(),registry));std::filesystem::remove(managedIR);auto current=serializePreset(session.current(),registry);auto selectedId=session.selected()->id;
    session.importFile(root/"missing-asset.json");check(wait([&]{audio.block(engine);session.poll();return !session.busy();}),"missing preset resolves failure");check(serializePreset(session.current(),registry)==current&&session.selected()->id==selectedId&&!session.dirty(),"missing asset load retains previous tone/selection/baseline");
    engine.reset();
}
}
int main(){auto root=std::filesystem::temp_directory_path()/("guitarrack-assets-"+newInstanceId());std::filesystem::create_directories(root);try{auto registry=EffectRegistry::builtIns();bundles(root,registry);sessionTransactions(root,registry);}catch(const std::exception& e){std::cerr<<"Unexpected: "<<e.what()<<'\n';++failures;}std::filesystem::remove_all(root);if(failures)return 1;std::cout<<"PASS: managed IR, portable bundles, live load/rollback, deletion and heap audits\n";}
