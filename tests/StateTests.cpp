#include "state/PresetSession.h"
#include "state/Preferences.h"
#include "core/MonitorEngine.h"
#include "HeapAudit.h"
#include <array>
#include <chrono>
#include <cmath>
#include <iostream>
#include <thread>
#include <fstream>
#include <sys/stat.h>
using namespace guitarrack;
namespace {
int failures=0;
void check(bool ok,const char* label) { if(!ok) { ++failures; std::cerr<<"FAIL: "<<label<<'\n'; } }
template<class F> bool wait(F f) { auto end=std::chrono::steady_clock::now()+std::chrono::seconds(4); do { if(f()) return true; std::this_thread::sleep_for(std::chrono::milliseconds(1)); } while(std::chrono::steady_clock::now()<end); return false; }
template<class F> void rejects(F f,const char* label) { bool rejected=false; try { f(); } catch(const std::exception&) { rejected=true; } check(rejected,label); }
juce::var copy(const ToneState& t,const EffectRegistry& r) { return parseJson(serializePreset(t,r)); }
void replace(juce::var& v,const char* key,juce::var value) { v.getDynamicObject()->setProperty(key,value); }
std::string json(const juce::var& v) { return juce::JSON::toString(v).toStdString(); }
struct Audio {
    std::array<float,512> l{},r{},in{}; bool heapFree=true;
    void block(MonitorEngine& engine,float x=.025f,int n=512) { in.fill(x); heapAudit::begin(); engine.process(in.data(),in.data(),l.data(),r.data(),n); heapFree &= heapAudit::finish(); }
};
void fixturesAndValidation(const EffectRegistry& registry) {
    auto factories=factoryPresets(registry);
    for(std::size_t i=0;i<factories.size();++i) {
        auto disk=parsePreset(readBounded(std::filesystem::path(GUITARRACK_SOURCE_DIR)/"assets/presets"/(std::array<const char*,3>{"clean.json","crunch.json","lead-delay.json"}[i])),registry);
        check(serializePreset(disk,registry)==serializePreset(parsePreset(serializePreset(factories[i],registry),registry),registry),"tracked factory fixtures match embedded factory configurations");
    }
    auto tone=factories.back(); tone.name="צליל גיטרה – Lead 🎸";
    auto second=tone.rack.instances.back(); second.id=newInstanceId(); second.slot=14; tone.rack.instances.push_back(second); tone.rack.order.insert(tone.rack.order.begin(),second.id); tone.controls.values[14]=tone.controls.values[2]; tone.controls.values[14][0]=.73f; tone.controls.bypass[14]=true;
    auto text=serializePreset(tone,registry); auto restored=parsePreset(text,registry);
    check(restored.id==tone.id&&restored.name==tone.name&&restored.rack==tone.rack,"Unicode names, independent IDs, exact order and slots round trip");
    check(restored.controls.bypass[14]&&std::abs(restored.controls.values[14][0]-.73f)<1e-6f&&restored.controls.values[2][0]!=restored.controls.values[14][0],"independent parameters and bypass round trip");
    auto invalid=[&](auto edit,const char* label) { auto v=copy(tone,registry); edit(v); rejects([&]{parsePreset(json(v),registry);},label); };
    invalid([](auto& v){replace(v,"schema_version",2);},"future version rejected");
    invalid([](auto& v){replace(v,"schema_version",0);},"no fabricated legacy migration");
    invalid([](auto& v){replace(v,"preset_id","../../escape");},"invalid preset UUID rejected");
    invalid([](auto& v){replace(v,"name","");},"empty name rejected");
    invalid([](auto& v){replace(v,"name",juce::String::repeatedString("x",257));},"long name rejected");
    invalid([](auto& v){ auto& a=*v["instances"].getArray(); replace(a.getReference(1),"instance_id",a[0]["instance_id"]); },"duplicate UUID rejected");
    invalid([](auto& v){ auto& a=*v["instances"].getArray(); replace(a.getReference(1),"slot_id",a[0]["slot_id"]); },"duplicate slot rejected");
    invalid([](auto& v){ auto& a=*v["instances"].getArray(); replace(a.getReference(1),"slot_id",16); },"invalid slot rejected");
    invalid([](auto& v){ auto& a=*v["instances"].getArray(); replace(a.getReference(1),"effect_type","cabinet"); },"unknown required effect rejected");
    invalid([](auto& v){ auto& a=*v["instances"].getArray(); replace(a.getReference(1),"effect_version",2); },"incompatible effect version rejected");
    invalid([](auto& v){ v["order"].getArray()->set(0,juce::String(newInstanceId())); },"invalid order reference rejected");
    invalid([](auto& v){ auto* a=v["order"].getArray(); a->set(1,(*a)[0]); },"duplicate order reference rejected");
    invalid([](auto& v){ auto p=(*v["instances"].getArray())[0]["parameters"]; replace(p,"drive_db",37); },"out of range physical value rejected");
    invalid([](auto& v){ auto p=(*v["instances"].getArray())[0]["parameters"]; replace(p,"drive_db","12"); },"numeric string rejected");
    invalid([](auto& v){ auto p=(*v["instances"].getArray())[0]["parameters"]; p.getDynamicObject()->removeProperty("drive_db"); },"missing parameter rejected");
    invalid([](auto& v){ v["assets"].getArray()->add("../ir.wav"); },"M4 rejects unsupported assets");
    for(const auto& s: {"{\"x\":1,\"x\":2}","{\"x\":1,\"\\u0078\":2}","{\"x\":1,}","{\"x\":01}","{\"x\":NaN}","{\"x\":Infinity}","{\"x\":\"\\ud800\"}","{\"x\":\"\\u0000\"}","[] trailing","{\"x\":18446744073709551616}","{\"x\":-18446744073709551616}"}) rejects([&]{parseJson(s);},"strict malformed JSON rejected");
    rejects([&]{parsePreset(std::string(maxPresetBytes+1,' '),registry);},"oversized input rejected");
    rejects([&]{parseJson(std::string("{\"x\":\"")+char(0xc0)+char(0xaf)+"\"}");},"invalid UTF-8 rejected");
    std::string deep(34,'['); deep+=std::string(34,']'); rejects([&]{parseJson(deep);},"excessive nesting rejected");
    auto v=copy(tone,registry); auto encoded=json(v); auto at=encoded.find("12.0"); if(at!=std::string::npos) encoded.replace(at,4,"1e999"); else { at=encoded.find("12"); encoded.replace(at,2,"1e999"); } rejects([&]{parsePreset(encoded,registry);},"nonfinite exponent rejected");
}
void filesAndActions(const EffectRegistry& registry,const std::filesystem::path& root) {
    PresetStore store(root/"presets"); auto t=factoryPresets(registry).back(); t.id=newInstanceId(); t.name="הגיטרה שלי"; store.save(t,registry); auto reopened=parsePreset(readBounded(store.directory/(t.id+".json")),registry);
    check(reopened.rack==t.rack&&reopened.name==t.name,"save and reopen user preset");
    auto old=readBounded(store.directory/(t.id+".json")); t.name="Updated"; store.save(t,registry); check(readBounded(store.directory/(t.id+".json"))!=old,"atomic replacement saves new contents");
    atomicWrite(root/"export.json",serializePreset(t,registry)); check(parsePreset(readBounded(root/"export.json"),registry).rack==t.rack,"import/export round trip");
    check(::mkfifo((root/"pipe.json").c_str(),0600)==0,"create nonregular file test fixture");
    rejects([&]{readBounded(root/"pipe.json");},"FIFO import rejected without blocking parser");
    rejects([&]{readBounded(root);},"directory import rejected");
    std::filesystem::create_directory(root/"blocked.json"); rejects([&]{atomicWrite(root/"blocked.json","{}");},"replacement failure reported");
    atomicWrite(root/"parent-file","x"); rejects([&]{atomicWrite(root/"parent-file"/"preset.json","{}");},"write failure reported");
    check(std::filesystem::is_directory(root/"blocked.json"),"write failure preserves previous destination");
    for(const auto& f:std::filesystem::directory_iterator(root)) check(!f.path().filename().string().starts_with(".guitarrack-"),"temporary files cleaned after failed writes");
    std::ofstream huge(root/"huge.json"); huge<<std::string(maxPresetBytes+1,' '); huge.close(); rejects([&]{readBounded(root/"huge.json");},"bounded file reader rejects oversized file");
    atomicWrite(store.directory/"bad.json","{}"); std::vector<std::string> errors; auto all=store.list(registry,errors); check(all.size()==1&&errors.size()==1,"invalid library file reported while valid presets remain available");
    Preferences p; p.backend="JACK"; p.inputDevice="missing input"; p.outputDevice="interface"; p.input=1; p.left=0; p.right=1; atomicWrite(root/"settings.json",serializePreferences(p)); check(parsePreferences(readBounded(root/"settings.json"))==p,"device/window preferences saved separately");
    juce::StringArray devices{"another microphone","interface"};
    check(exactDeviceIndex(devices,p.inputDevice)==-1&&exactDeviceIndex(devices,p.outputDevice)==1&&exactDeviceIndex(devices,"")==-1,"missing remembered input never substitutes another microphone");
    auto preferences=parseJson(serializePreferences(p)); replace(preferences,"settings_version",2); rejects([&]{parsePreferences(json(preferences));},"invalid preference version rejected");
    RackEngine rack; MonitorControls c; MonitorEngine engine(c,&rack); PresetSession session(rack,root/"session");
    session.select(t); check(wait([&]{session.poll();return !session.busy();}),"stopped preset commits without monitoring");
    check(session.selected()&&session.selected()->name==t.name&&!session.dirty(),"selected preset and clean baseline committed together");
    auto originalRack=session.current().rack;
    auto moved=originalRack.order.back(); rack.move(moved,-1);
    check(wait([&]{session.poll();return !session.busy();})&&session.dirty()&&session.current().rack.find(moved)->slot==originalRack.find(moved)->slot,"reorder marks dirty and preserves immutable slots");
    rack.move(moved,1); check(wait([&]{session.poll();return !session.busy();})&&!session.dirty(),"restoring original order restores clean configuration");
    rack.add("gain"); check(wait([&]{return !session.busy();})&&session.dirty(),"adding an instance marks dirty");
    auto added=rack.snapshot().committed.order.back(); rack.remove(added); check(wait([&]{return !session.busy();})&&!session.dirty(),"removing added instance restores configuration without serializing unused bank/history");
    auto id=t.rack.order[0]; rack.setParameter(id,0,20); check(session.dirty(),"parameter edit marks dirty");
    check(!session.resolveDirty(DirtyAction::cancel)&&session.dirty()&&session.selected()->id==t.id,"Cancel preserves unsaved state and selection");
    check(session.resolveDirty(DirtyAction::discard),"Discard permits switch"); session.select(t); check(wait([&]{session.poll();return !session.busy();})&&!session.dirty(),"Discard plus recall restores saved controls");
    rack.setBypassed(id,true); check(session.dirty(),"bypass edit marks dirty"); check(session.saveAs("צליל חדש"),"Save As accepts Hebrew"); auto saved=session.current();
    check(saved.id!=t.id&&saved.rack==t.rack&&!session.dirty(),"Save As changes only preset identity, preserving effect UUIDs/slots/order");
    c.setInputDb(-3); c.setOutputDb(-18); rack.bank.setRackBypassed(true); check(session.dirty(),"global edits mark dirty"); check(session.resolveDirty(DirtyAction::save)&&!session.dirty(),"Save action persists and clears dirty");
    check(session.exportFile(root/"session-export.json"),"session export succeeds"); check(parsePreset(readBounded(root/"session-export.json"),registry).id==saved.id,"export preserves saved preset UUID");
    atomicWrite(root/"last-tone.json",serializePreset(*session.selected(),registry));
    {
        RackEngine reopenedRack; MonitorControls reopenedControls; MonitorEngine reopenedEngine(reopenedControls,&reopenedRack);
        PresetSession reopenedSession(reopenedRack,root/"session"); reopenedSession.importFile(root/"last-tone.json");
        check(wait([&]{reopenedSession.poll();return !reopenedSession.busy();})&&reopenedSession.current().rack==session.current().rack&&reopenedSession.selected()->id==session.selected()->id&&reopenedControls.outputDb()==-18,"fresh session recalls saved selection/rack/globals while remaining stopped");
    }
    c.setInputDb(-6); auto prior=session.current(); session.store.directory=root/"parent-file"/"cannot-write"; check(!session.save()&&session.dirty()&&session.current()==prior,"failed save retains current tone and dirty indication");
    session.importFile(root/"bad-input.json"); check(wait([&]{session.poll();return !session.busy();})&&!session.error.empty(),"failed import reported"); check(session.current()==prior&&session.dirty(),"failed import preserves tone, selection and edits");
    session.select(factoryPresets(registry)[1]); session.select(factoryPresets(registry)[0]); check(wait([&]{session.poll();return !session.busy();}),"competing asynchronous requests finish"); check(session.selected()->name=="Clean"&&session.current().rack.instances.empty(),"latest asynchronous load wins");
    session.select(t); c.setInputDb(-7);
    check(wait([&]{session.poll();return !session.busy();})&&session.selected()->name=="Clean"&&c.inputDb()==-7,"control edit during asynchronous parsing supersedes old load");
    session.select(t); check(session.cancelLoad(),"queued async load cancelled"); for(int i=0;i<20;++i) { session.poll(); std::this_thread::sleep_for(std::chrono::milliseconds(1)); } check(session.selected()->name=="Clean","cancelled parser completion cannot change selection");
}
struct Probe { std::atomic<bool> entered{false}, release{false}; bool failure=false; };
class Slow final : public EffectProcessor {
public: explicit Slow(Probe& p):probe(p) {} bool prepare(double,int) override { probe.entered=true; while(!probe.release) std::this_thread::sleep_for(std::chrono::milliseconds(1)); if(probe.failure) throw std::runtime_error("Injected failure"); return true; }
    void reset() noexcept override {} void activate(const EffectProcessor*) noexcept override {} void process(float*,float*,int) noexcept override {}
private: Probe& probe;
};
void transactions() {
    auto registry=EffectRegistry::builtIns(); Probe p; registry.add({"slow","slow","Test",1,{},[&](const ParameterBank&,SlotId){return std::make_unique<Slow>(p);}});
    RackEngine rack(registry); MonitorControls c; c.setOutputDb(-12); MonitorEngine engine(c,&rack); Audio audio;
    auto clean=factoryPresets(registry)[0]; rack.loadTone(clean); check(wait([&]{return !rack.snapshot().tonePending;}),"initial tone commits while stopped"); engine.prepare(48000);
    auto crunch=factoryPresets(registry)[1]; auto blocked=crunch; auto uuid=newInstanceId(); blocked.rack.instances.push_back({uuid,15,"slow",1}); blocked.rack.order.push_back(uuid); blocked.controls.inputDb=6; blocked.controls.outputDb=-24;
    auto before=rack.captureTone(clean.id,clean.name); auto gen=rack.loadTone(blocked); check(gen&&wait([&]{return p.entered.load();}),"preset preparation runs on worker");
    c.setMuted(true); audio.block(engine); check(c.inputDb()==before.controls.inputDb&&c.outputDb()==before.controls.outputDb&&c.isMuted(),"blocked load leaves globals and Mute untouched");
    check(rack.cancelTone(gen),"pre-acceptance graph load cancelled"); p.release=true;
    check(wait([&]{audio.block(engine);auto s=rack.snapshot();return s.committedGeneration==s.requestedGeneration;}),"cancelled graph request settles"); check(rack.snapshot().committed==before.rack&&c.outputDb()==before.controls.outputDb,"cancelled graph retains previous configuration");
    // Same slots, different UUIDs may load while old graphs still reference slots.
    auto lead=factoryPresets(registry)[2]; lead.controls.outputDb=-30; for(auto& i:lead.rack.instances) { auto old=i.id; i.id=newInstanceId(); for(auto& id:lead.rack.order) if(id==old) id=i.id; }
    auto latest=rack.loadTone(lead); check(latest&&wait([&]{audio.block(engine);return rack.snapshot().toneGeneration==latest;}),"exact slot replacement commits live with independent old controls"); check(c.outputDb()==-30&&c.isMuted()&&rack.snapshot().committed==lead.rack,"live transaction commits globals/rack and preserves Mute");
    c.setMuted(false); for(int i=0;i<5;++i) audio.block(engine); check(audio.l.back()!=0,"load did not revoke running monitoring");
    auto changedIdentity=lead; changedIdentity.rack.instances[0].slot=10;
    check(!rack.loadTone(changedIdentity),"preset cannot change a known UUID's immutable slot");
    auto gainTone=lead; gainTone.controls.inputDb=-6; gainTone.controls.outputDb=-24;
    float previous=audio.l.back(); bool continuous=true; auto gainGeneration=rack.loadTone(gainTone);
    check(wait([&]{audio.block(engine);for(float x:audio.l) { continuous &= std::isfinite(x)&&std::abs(x-previous)<.0005f; previous=x; } return rack.snapshot().toneGeneration==gainGeneration;}),"global gain tone transition commits live");
    check(continuous&&c.inputDb()==-6&&c.outputDb()==-24,"preset input/output gains ramp continuously");
    p.failure=true; p.entered=false; auto failing=blocked; failing.controls.outputDb=6; auto prev=rack.captureTone(lead.id,lead.name); auto bad=rack.loadTone(failing);
    check(bad&&wait([&]{audio.block(engine);return !rack.snapshot().tonePending&&!rack.snapshot().error.empty();}),"preparation failure reported"); check(rack.captureTone(lead.id,lead.name)==prev&&rack.snapshot().committed==lead.rack,"failed build rolls back rack, controls and globals together");
    p.failure=false; p.release=false; p.entered=false; rack.loadTone(blocked); check(wait([&]{return p.entered.load();}),"stale preparation blocks off audio"); auto newest=rack.loadTone(clean); p.release=true;
    check(wait([&]{audio.block(engine);return rack.snapshot().toneGeneration==newest;}),"newer tone wins after blocked stale build"); check(rack.snapshot().committed==clean.rack&&c.outputDb()==-12&&rack.snapshot().stale>0,"stale tone never overwrites current controls/globals");
    auto interrupted=rack.loadTone(lead);
    check(wait([&]{return rack.snapshot().preparedWaiting;}),"prepared tone awaits block boundary");
    audio.block(engine,.025f,0);
    check(rack.snapshot().toneGeneration!=interrupted&&c.outputDb()==-12,"zero-sample block cannot partially activate a preset");
    c.setOutputDb(-20); rack.bank.setRackBypassed(true);
    check(wait([&]{audio.block(engine);return !rack.snapshot().tonePending;}),"new control write rejects prepared tone before activation");
    check(rack.snapshot().toneGeneration!=interrupted&&c.outputDb()==-20&&rack.bank.rackBypassed()&&rack.snapshot().committed==clean.rack,"prepared stale preset retains newer globals and bank controls");
    check(audio.heapFree,"preset activation, gains, crossfades, rejection and reclamation are callback heap-free"); engine.reset();
    check(wait([&]{return rack.snapshot().graphs==1;}),"preset graphs reclaimed to one graph");
    // Reprepare at a new rate keeps the request identity across graph generations.
    auto restartGeneration=rack.loadTone(lead); check(engine.prepare(44100),"pending tone survives explicit sample-rate prepare");
    check(wait([&]{return rack.snapshot().toneGeneration==restartGeneration&&!rack.snapshot().tonePending;}),"sample-rate rebuild preserves preset transaction acknowledgment");
    std::atomic<bool> done{false},heapFree{true};
    std::thread consumer([&]{Audio stream;while(!done) stream.block(engine,.02f,127);heapFree=stream.heapFree;});
    std::uint64_t last=0;
    for(int i=0;i<300;++i) last=rack.loadTone(i%2?lead:clean);
    check(wait([&]{return rack.snapshot().toneGeneration==last;}),"competing concurrent tone loads converge to final request");
    done=true; consumer.join(); engine.reset();
    check(heapFree&&wait([&]{return rack.snapshot().graphs==1;})&&rack.snapshot().peakGraphs<=4,"full tone saturation remains heap-free, bounded and reclaims obsolete graphs");
}
void equivalentDSP(const EffectRegistry& registry) {
    auto t=factoryPresets(registry)[2]; auto second=t.rack.instances.back(); second.id=newInstanceId(); second.slot=11; t.rack.instances.push_back(second); t.rack.order.push_back(second.id); t.controls.values[11]=t.controls.values[2]; t.controls.values[11][0]=.075f; t.controls.values[11][2]=.13f;
    auto restored=parsePreset(serializePreset(t,registry),registry);
    RackEngine a,b; MonitorControls ca,cb; MonitorEngine ea(ca,&a),eb(cb,&b); a.loadTone(t); b.loadTone(restored);
    check(wait([&]{return !a.snapshot().tonePending&&!b.snapshot().tonePending;}),"both configurations loaded"); ea.prepare(48000); eb.prepare(48000);
    Audio aa,bb; float difference=0;
    for(int block=0;block<150;++block) {
        for(int i=0;i<512;++i) aa.in[i]=bb.in[i]=.02f*std::sin(static_cast<float>((block*512+i)*.037))+(block==0&&i==0?.1f:0);
        heapAudit::begin(); ea.process(aa.in.data(),aa.in.data(),aa.l.data(),aa.r.data(),512); eb.process(bb.in.data(),bb.in.data(),bb.l.data(),bb.r.data(),512); check(heapAudit::finish(),"restored DSP rendering heap-free");
        for(int i=0;i<512;++i) difference=std::max(difference,std::abs(aa.l[i]-bb.l[i]));
    }
    check(difference<2e-6f,"restored tone gives equivalent DSP from identical reset conditions/input");
}
}
int main() {
    auto registry=EffectRegistry::builtIns(); auto root=std::filesystem::temp_directory_path()/("guitarrack-state-"+newInstanceId()); std::filesystem::create_directory(root);
    try { fixturesAndValidation(registry); filesAndActions(registry,root); transactions(); equivalentDSP(registry); } catch(const std::exception& e) { ++failures; std::cerr<<"Unexpected: "<<e.what()<<'\n'; }
    std::filesystem::remove_all(root); if(failures) return 1; std::cout<<"PASS: strict presets, Unicode, files, dirty actions, rollback, stale/cancelled loads, DSP equivalence, RT audit\n";
}
