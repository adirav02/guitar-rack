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
        const auto file=i<5 ? std::filesystem::path(std::array<const char*,5>{"clean.json","crunch.json","lead-delay.json","driven-synthetic-cabinet.json","small-room.json"}[i])
                            : std::filesystem::path("experimental")/(factories[i].id+".json");
        auto disk=parsePreset(readBounded(std::filesystem::path(GUITARRACK_SOURCE_DIR)/"assets/presets"/file),registry);
        check(serializePreset(disk,registry)==serializePreset(parsePreset(serializePreset(factories[i],registry),registry),registry),"tracked factory fixtures match embedded factory configurations");
    }
    auto tone=factories[2]; tone.name="צליל גיטרה – Lead 🎸";
    auto second=tone.rack.instances.back(); second.id=newInstanceId(); second.slot=14; tone.rack.instances.push_back(second); tone.rack.order.insert(tone.rack.order.begin(),second.id); tone.controls.values[14]=tone.controls.values[2]; tone.controls.values[14][0]=.73f; tone.controls.bypass[14]=true;
    auto text=serializePreset(tone,registry); auto restored=parsePreset(text,registry);
    check(restored.id==tone.id&&restored.name==tone.name&&restored.rack==tone.rack,"Unicode names, independent IDs, exact order and slots round trip");
    check(restored.controls.bypass[14]&&std::abs(restored.controls.values[14][0]-.73f)<1e-6f&&restored.controls.values[2][0]!=restored.controls.values[14][0],"independent parameters and bypass round trip");
    auto invalid=[&](auto edit,const char* label) { auto v=copy(tone,registry); edit(v); rejects([&]{parsePreset(json(v),registry);},label); };
    invalid([](auto& v){replace(v,"schema_version",3);},"future version rejected");
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
    PresetStore store(root/"presets"); auto t=factoryPresets(registry)[2]; t.id=newInstanceId(); t.name="הגיטרה שלי"; store.save(t,registry); auto reopened=parsePreset(readBounded(store.directory/(t.id+".json")),registry);
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
    auto preferences=parseJson(serializePreferences(p)); replace(preferences,"settings_version",3); rejects([&]{parsePreferences(json(preferences));},"invalid preference version rejected");
    auto oldPreferences=parseJson(serializePreferences(p));
    for(const auto* key:{"audio_collapsed","browser_x","browser_y","browser_width","browser_height","board_scroll","search","source","artist","category","favorites","expanded","latency_profile"}) oldPreferences.getDynamicObject()->removeProperty(key);
    replace(oldPreferences,"settings_version",1); replace(oldPreferences,"height",1200);
    auto migrated=parsePreferences(json(oldPreferences));
    check(migrated.height==768&&migrated.inputDevice==p.inputDevice&&migrated.rate==p.rate&&migrated.favorites.empty(),"strict v1 migration lowers old tall window and preserves device choices");
    p.width=1366;p.height=768;p.audioCollapsed=true;p.search="צליל";p.favorites={t.id};p.expanded={t.rack.order.front()};p.boardScroll=123;p.latencyProfile=4096;
    check(parsePreferences(serializePreferences(p))==p,"local browser, expansion, UTF-8 and compact geometry round trip");
    auto duplicate=parseJson(serializePreferences(p));duplicate["favorites"].getArray()->add(juce::String(t.id));
    rejects([&]{parsePreferences(json(duplicate));},"duplicate local favorite identities rejected");
    auto tooSmall=parseJson(serializePreferences(p));replace(tooSmall,"height",449);
    rejects([&]{parsePreferences(json(tooSmall));},"inaccessible window size rejected");
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
    session.select(factoryPresets(registry)[3]);check(wait([&]{session.poll();return !session.busy();}),"cabinet selection settles");
    auto cabinetTone=session.current();auto cabinet=std::find_if(cabinetTone.rack.instances.begin(),cabinetTone.rack.instances.end(),[](const auto& i){return i.effectType=="cabinet";});
    check(cabinet!=cabinetTone.rack.instances.end(),"cabinet fixture present");
    auto approvedId=session.selected()->id;auto priorCabinet=cabinetTone;
    session.selectBuiltinIR(cabinet->id,std::string(64,'0'));check(session.current()==priorCabinet&&!session.dirty()&&session.selected()->id==approvedId,"unknown built-in asset retains sound and association");
    session.selectBuiltinIR(cabinet->id,"43274927cae42dd5bd1665c1724fb47ba220a4f48bf3615676a951abe9a3933d");check(wait([&]{session.poll();return !session.busy();}),"built-in cabinet transaction completes");
    check(session.current().rack.order==priorCabinet.rack.order&&session.selected()->id==approvedId&&session.dirty(),"cabinet voicing changes only asset and dirties existing association");
    auto dirtyCabinet=session.current();auto pitched=factoryPresets(registry);auto pitchTone=std::find_if(pitched.begin(),pitched.end(),[](const auto& tone){return std::any_of(tone.rack.instances.begin(),tone.rack.instances.end(),[](const auto& i){return i.effectType=="expression_pitch";});});
    session.select(*pitchTone);check(wait([&]{session.poll();return !session.busy();})&&!session.error.empty()&&session.current()==dirtyCabinet&&session.dirty()&&session.selected()->id==approvedId,"incompatible latency load preserves dirty tone and identity");
    session.select(factoryPresets(registry)[0]);check(wait([&]{session.poll();return !session.busy();}),"restore clean before cancellation check");
    session.select(t); check(session.cancelLoad(),"queued async load cancelled"); for(int i=0;i<20;++i) { session.poll(); std::this_thread::sleep_for(std::chrono::milliseconds(1)); } check(session.selected()->name=="Clean","cancelled parser completion cannot change selection");
}
void deletionActions(const EffectRegistry& registry, const std::filesystem::path& root) {
    auto data=root/"deletion"; RackEngine rack; MonitorControls controls; MonitorEngine engine(controls,&rack);
    PresetSession session(rack,data/"presets");
    check(!session.canDelete(),"unnamed initial rack cannot be deleted");
    for(const auto& factory:factoryPresets(registry)) {
        session.select(factory); check(wait([&]{session.poll();return !session.busy();}),"factory selection settles");
        check(!session.canDelete()&&!session.deleteSelected(factory.id,DirtyAction::discard,true),"factory deletion protected in session");
        session.store.save(factory,registry); const auto protectedFile=session.store.directory/(factory.id+".json");
        const auto protectedBytes=readBounded(protectedFile);
        rejects([&]{session.store.erase(factory.id,registry,session.recallFile);},"factory deletion protected in store");
        check(readBounded(protectedFile)==protectedBytes,"even a factory UUID file inside the user store is protected");
    }
    auto imported=factoryPresets(registry)[2]; imported.id=newInstanceId(); imported.name="Imported only";
    atomicWrite(data/"import.json",serializePreset(imported,registry)); session.importFile(data/"import.json");
    check(wait([&]{session.poll();return !session.busy();})&&!session.canDelete(),"external import alone is not a managed saved preset");
    check(!session.deleteSelected(imported.id,DirtyAction::discard,true)&&std::filesystem::exists(data/"import.json"),"import path is never a deletion target");

    for(auto action:{DirtyAction::save,DirtyAction::discard}) {
        check(session.saveAs("צליל 🎸 ../exports/Lead"),"Unicode and path-like names save with UUID filenames");
        const auto saved=*session.selected(); const auto file=session.store.directory/(saved.id+".json");
        session.persistRecall(); const auto recall=readBounded(session.recallFile), bytes=readBounded(file);
        const auto catalog=session.library;
        rack.setParameter(saved.rack.order[0],0,18); rack.setBypassed(saved.rack.order[1],true);
        controls.setInputDb(-4); controls.setOutputDb(-21); controls.setMuted(true); rack.bank.setRackBypassed(true);
        const auto edited=session.current();
        check(session.dirty()&&session.canDelete(),"saved user with edits is deletable after dirty workflow");
        check(!session.deleteSelected(saved.id,DirtyAction::cancel,true),"dirty Cancel aborts deletion");
        check(!session.deleteSelected(saved.id,action,false),"confirmation Cancel aborts both staged Save and Discard");
        check(session.current()==edited&&session.selected()==saved&&session.dirty()&&session.library==catalog
              &&readBounded(file)==bytes&&readBounded(session.recallFile)==recall,"both cancellation points preserve state and all persisted bytes");
        check(!session.deleteSelected(newInstanceId(),action,true)&&session.current()==edited&&session.dirty(),"stale confirmation cannot delete different selection");
        const auto exported=data/(action==DirtyAction::save?"export-save.json":"export-discard.json");
        check(session.exportFile(exported),"export before deletion"); const auto exportBytes=readBounded(exported);
        atomicWrite(data/"cabinet.wav","asset bytes"); atomicWrite(session.store.directory/"unrelated.txt","unrelated bytes");
        atomicWrite(data/"settings.json",serializePreferences(Preferences{})); const auto preferences=readBounded(data/"settings.json");
        auto duplicate=saved; duplicate.id=newInstanceId(); session.store.save(duplicate,registry);
        const auto duplicateFile=session.store.directory/(duplicate.id+".json"); const auto duplicateBytes=readBounded(duplicateFile);
        const auto generation=rack.snapshot().requestedGeneration;
        check(session.deleteSelected(saved.id,action,true),"confirmed dirty user preset deleted");
        auto working=session.current();
        check(!session.selected()&&session.dirty()&&!session.canDelete()&&working.id!=saved.id&&working.name=="Untitled rack",
              "deletion detaches saved identity and produces unsaved working rack");
        check(working.rack==edited.rack&&working.controls==edited.controls&&controls.isMuted()
              &&rack.snapshot().requestedGeneration==generation,"deletion preserves rack, gains, parameters, bypass, Mute and graph generation");
        check(!std::filesystem::exists(file)&&session.library.size()+1==catalog.size(),"only selected catalog entry and managed file removed");
        check(readBounded(exported)==exportBytes&&readBounded(data/"cabinet.wav")=="asset bytes"
              &&readBounded(session.store.directory/"unrelated.txt")=="unrelated bytes","exports, IR-like assets and unrelated files survive deletion");
        check(readBounded(data/"settings.json")==preferences&&readBounded(duplicateFile)==duplicateBytes,
              "device preferences and another preset with the identical display name survive deletion");
        check(parsePreset(readBounded(session.recallFile),registry).id!=saved.id,"recall updated immediately before close");
        {
            RackEngine fresh; PresetSession restarted(fresh,data/"presets"); restarted.importFile(session.recallFile);
            check(wait([&]{restarted.poll();return !restarted.busy();})&&restarted.selected()->id!=saved.id&&!restarted.canDelete(),"fresh session/restart cannot recall deleted selection");
            check(std::none_of(restarted.library.begin(),restarted.library.end(),[&](const auto& t){return t.id==saved.id;}),"fresh catalog excludes deleted UUID");
        }
        session.persistRecall(); check(parsePreset(readBounded(session.recallFile),registry).id!=saved.id,"normal close never writes deleted association back");
        check(!session.save(),"working rack requires Save As");
        check(session.saveAs("צליל חדש 🎸")&&!session.dirty()&&session.selected()->id!=saved.id
              &&session.current().rack==edited.rack&&session.current().controls==edited.controls,"Save As saves unchanged working sound with new identity");
        // Next iteration needs a dirty change even if the same values were used.
        rack.setParameter(saved.rack.order[0],0,12); controls.setInputDb(0); controls.setOutputDb(-12); rack.bank.setRackBypassed(false);
    }
    auto id=session.selected()->id;
    check(session.save()&&!session.dirty()&&session.deleteSelected(id,DirtyAction::discard,true),"clean preset deletion succeeds without dirty prompt");
}
void deletionFailures(const EffectRegistry& registry, const std::filesystem::path& root) {
    const auto data=root/"delete-failures"; RackEngine rack; MonitorControls controls; MonitorEngine engine(controls,&rack);
    PresetSession session(rack,data/"presets"); session.select(factoryPresets(registry)[1]);
    check(wait([&]{session.poll();return !session.busy();})&&session.saveAs("Failure test"),"create deletion failure fixture");
    const auto saved=*session.selected(); const auto catalog=session.library;
    const auto file=session.store.directory/(saved.id+".json"); const auto original=readBounded(file);
    controls.setInputDb(-7); const auto before=session.current(); session.persistRecall(); const auto recall=readBounded(session.recallFile);
    auto retained=[&] {
        check(session.selected()==saved&&session.library==catalog&&session.current()==before&&session.dirty()
              &&!session.error.empty(),"delete failure retains selection, catalog, rack, dirty state and actionable error");
    };
    std::filesystem::remove(file);
    check(!session.deleteSelected(saved.id,DirtyAction::save,true),"missing managed file is not recreated by dirty Save"); retained();
    check(!std::filesystem::exists(file),"missing entry remains absent after failed deletion");
    atomicWrite(file,original);
    check(::chmod(session.store.directory.c_str(),0500)==0,"restrict user directory for real unlink failure");
    check(!session.deleteSelected(saved.id,DirtyAction::discard,true),"directory permission failure reported"); retained();
    check(readBounded(file)==original&&readBounded(session.recallFile)==recall,"unlink failure rolls recall back and keeps preset file");
    check(!session.deleteSelected(saved.id,DirtyAction::save,true),"unwritable staged Save aborts deletion"); retained();
    check(readBounded(file)==original&&readBounded(session.recallFile)==recall,"Save failure retains original file and recall");
    check(::chmod(session.store.directory.c_str(),0700)==0,"restore fixture permissions");
    // A blocked recall update fails before unlink and rolls staged Save back.
    check(::chmod(data.c_str(),0500)==0,"restrict recall directory");
    check(!session.deleteSelected(saved.id,DirtyAction::save,true),"recall filesystem failure aborts dirty Save deletion"); retained();
    check(readBounded(file)==original&&readBounded(session.recallFile)==recall,"failed recall update restores pre-Save preset and recall");
    check(::chmod(data.c_str(),0700)==0,"restore recall directory permissions");
    atomicWrite(file,"{}"); check(!session.deleteSelected(saved.id,DirtyAction::discard,true),"corrupted managed file cannot be deleted"); retained();
    auto wrong=saved; wrong.id=newInstanceId(); atomicWrite(file,serializePreset(wrong,registry));
    check(!session.deleteSelected(saved.id,DirtyAction::save,true),"UUID mismatch cannot be overwritten by staged Save"); retained();
    const auto exportPath=data/"exported.json"; atomicWrite(exportPath,original); std::filesystem::remove(file); std::filesystem::create_symlink(exportPath,file);
    check(!session.deleteSelected(saved.id,DirtyAction::save,true),"symlink entry rejected before Save/delete"); retained();
    check(std::filesystem::is_symlink(file)&&readBounded(exportPath)==original,"failed symlink deletion leaves exported copy and link intact");
    std::filesystem::remove(file); std::filesystem::create_directory(file);
    check(!session.deleteSelected(saved.id,DirtyAction::discard,true),"directory entry cannot be deleted"); retained();
    check(std::filesystem::is_directory(file),"nonregular managed path preserved"); std::filesystem::remove(file); atomicWrite(file,original);
    const auto storePath=session.store.directory; const auto alias=data/"preset-link"; std::filesystem::create_directory_symlink(storePath,alias); session.store.directory=alias;
    check(!session.deleteSelected(saved.id,DirtyAction::discard,true),"symlink store directory cannot redirect deletion"); retained(); session.store.directory=storePath;
    rejects([&]{session.store.erase("../exported",registry,session.recallFile);},"store rejects traversal IDs");
    check(readBounded(exportPath)==original&&readBounded(file)==original,"all failed deletes preserve managed original and export");
    // Deleting another selected preset must leave an unrelated recall untouched.
    auto clean=factoryPresets(registry).front(); atomicWrite(session.recallFile,serializePreset(clean,registry)); const auto unrelated=readBounded(session.recallFile);
    check(session.deleteSelected(saved.id,DirtyAction::discard,true)&&readBounded(session.recallFile)==unrelated,"unrelated recall is preserved");
}
struct Probe { std::atomic<bool> entered{false}, release{false}; bool failure=false; };
class Slow final : public EffectProcessor {
public: explicit Slow(Probe& p):probe(p) {} bool prepare(double,int) override { probe.entered=true; while(!probe.release) std::this_thread::sleep_for(std::chrono::milliseconds(1)); if(probe.failure) throw std::runtime_error("Injected failure"); return true; }
    void reset() noexcept override {} void activate(const EffectProcessor*) noexcept override {} void process(float*,float*,int) noexcept override {}
private: Probe& probe;
};
void deletionCompetition(const std::filesystem::path& root) {
    auto registry=EffectRegistry::builtIns(); Probe probe;
    registry.add({"slow","slow","Test",1,{},[&](const ParameterBank&,SlotId){return std::make_unique<Slow>(probe);}});
    RackEngine rack(registry); MonitorControls controls; MonitorEngine engine(controls,&rack); Audio audio;
    PresetSession session(rack,root/"delete-races"/"presets"); session.select(factoryPresets(registry)[2]);
    check(wait([&]{session.poll();return !session.busy();})&&session.saveAs("Async victim"),"create competing deletion fixture");
    auto saved=*session.selected(); auto file=session.store.directory/(saved.id+".json");
    session.importFile(file);
    check(!session.canDelete()&&!session.deleteSelected(saved.id,DirtyAction::discard,true),"queued parser load prevents deletion");
    check(session.cancelLoad()&&session.deleteSelected(saved.id,DirtyAction::discard,true),"cancel parser then delete saved selection");
    for(int i=0;i<30;++i) { session.poll(); std::this_thread::sleep_for(std::chrono::milliseconds(1)); }
    check(!session.selected()&&!std::filesystem::exists(file)&&session.dirty(),"late cancelled parser result cannot reselect or recreate deleted preset");
    auto working=session.current(); session.select(saved);
    check(!session.busy()&&!session.error.empty()&&!session.selected()&&session.current()==working,"cached pre-delete selection cannot revive deleted UUID");
    check(session.saveAs("Next saved rack"),"save working rack before blocked graph race"); saved=*session.selected(); file=session.store.directory/(saved.id+".json");
    engine.prepare(48000);
    auto blocked=saved; auto id=newInstanceId(); blocked.rack.instances.push_back({id,15,"slow",1}); blocked.rack.order.push_back(id);
    session.select(blocked);
    check(wait([&]{session.poll();return probe.entered.load();}),"async preset graph blocked on off-audio worker");
    check(!session.canDelete()&&!session.deleteSelected(saved.id,DirtyAction::discard,true)&&session.selected()==saved&&std::filesystem::exists(file),"in-flight graph deletion rejected without modifying selection/file");
    check(session.cancelLoad(),"blocked graph cancelled before acceptance"); probe.release=true;
    check(wait([&]{audio.block(engine);session.poll();return !session.busy();}),"cancelled graph settles before deletion");
    check(session.deleteSelected(saved.id,DirtyAction::discard,true),"delete after graph cancellation");
    for(int i=0;i<30;++i) { audio.block(engine); session.poll(); std::this_thread::sleep_for(std::chrono::milliseconds(1)); }
    check(!session.selected()&&!std::filesystem::exists(file)&&session.current().rack==saved.rack,"stale worker graph cannot restore deleted association or change working rack");
    engine.reset();
    check(session.saveAs("Confirmation target"),"create stale confirmation fixture"); saved=*session.selected();
    auto another=factoryPresets(registry)[0]; session.select(another);
    check(wait([&]{session.poll();return !session.busy();}),"competing selection settles before confirmation");
    check(!session.deleteSelected(saved.id,DirtyAction::save,true)&&session.selected()==another
          &&std::filesystem::exists(session.store.directory/(saved.id+".json")),"selection changed while confirming leaves old preset untouched");
    check(audio.heapFree,"deletion competition preserves callback heap audit");
}
void deletionSound(const EffectRegistry& registry, const std::filesystem::path& root) {
    auto tone=factoryPresets(registry)[2]; tone.id=newInstanceId(); tone.name="Long echoes";
    RackEngine a,b; MonitorControls ca,cb; MonitorEngine ea(ca,&a),eb(cb,&b);
    PresetSession session(a,root/"delete-sound"/"presets"); session.select(tone); b.loadTone(tone);
    check(wait([&]{session.poll();return !session.busy()&&!b.snapshot().tonePending;})&&session.save(),"save delay sound fixture");
    ea.prepare(48000); eb.prepare(48000); Audio aa,bb;
    for(int i=0;i<40;++i) { aa.block(ea,i==0?.2f:0); bb.block(eb,i==0?.2f:0); }
    const auto before=session.current(); const auto graphs=a.snapshot();
    check(session.deleteSelected(tone.id,DirtyAction::discard,true),"delete while DSP has delay history");
    float difference=0, peak=0;
    for(int i=0;i<100;++i) {
        aa.block(ea,0); bb.block(eb,0);
        for(unsigned j=0;j<aa.l.size();++j) { difference=std::max(difference,std::abs(aa.l[j]-bb.l[j])); peak=std::max(peak,std::abs(aa.l[j])); }
        session.poll();
    }
    check(peak>0&&difference==0,"deletion preserves sample-exact ongoing delay history versus uninterrupted reference");
    check(session.current().rack==before.rack&&session.current().controls==before.controls
          &&a.snapshot().requestedGeneration==graphs.requestedGeneration&&a.snapshot().built==graphs.built,
          "deletion builds no DSP graph and writes no controls");
    check(aa.heapFree&&bb.heapFree,"continuing audio after deletion remains heap-free");
    ea.reset(); ea.prepare(44100);
    check(wait([&]{aa.block(ea,0);session.poll();return !session.busy();})&&!session.selected()&&session.dirty(),
          "sample-rate restart cannot republish deleted preset association");
}
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
    try { fixturesAndValidation(registry); filesAndActions(registry,root); deletionActions(registry,root); deletionFailures(registry,root); deletionCompetition(root); deletionSound(registry,root); transactions(); equivalentDSP(registry); } catch(const std::exception& e) { ++failures; std::cerr<<"Unexpected: "<<e.what()<<'\n'; }
    std::filesystem::remove_all(root); if(failures) return 1; std::cout<<"PASS: strict presets, Unicode, files, dirty actions, rollback, stale/cancelled loads, DSP equivalence, RT audit\n";
}

