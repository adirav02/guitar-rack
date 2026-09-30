#include "PresetSession.h"
namespace guitarrack {
PresetSession::PresetSession(RackEngine& r,std::filesystem::path root) : store(std::move(root)), rack(r) {
    baseline=serializePreset(current(),rack.registry()); refreshLibrary(); parser=std::thread([this]{parserLoop();});
}
PresetSession::~PresetSession() { { std::lock_guard lock(mutex); quitting=true; } wake.notify_one(); parser.join(); }
void PresetSession::refreshLibrary() {
    try {
        library=factoryPresets(rack.registry()); std::vector<std::string> errors;
        auto user=store.list(rack.registry(),errors);
        for(auto& t:user) { std::erase_if(library,[&](const auto& v){return v.id==t.id;}); library.push_back(std::move(t)); }
        if(!errors.empty()) { error="Invalid user presets were not loaded:\n"; for(const auto& e:errors) error+=e+"\n"; }
    } catch(const std::exception& e) { error=e.what(); }
}
ToneState PresetSession::current() const { return rack.captureTone(selection?selection->id:"00000000-0000-4000-8000-000000000001",selection?selection->name:"Clean"); }
bool PresetSession::busy() const { auto s=rack.snapshot(); return waitingSerial!=0||s.tonePending||s.transitioning||(s.requestedGeneration!=s.committedGeneration&&s.error.empty()); }
bool PresetSession::dirty() const { if(busy()) return false; return serializePreset(current(),rack.registry())!=baseline; }
bool PresetSession::selectedIsFactory() const { if(!selection) return true;
    for(const auto& factory:factoryPresets(rack.registry())) if(factory.id==selection->id) return true;
    return false; }
void PresetSession::request(Work work) {
    if(graphGeneration) rack.cancelTone(graphGeneration);
    bankRevision=rack.bank.revision(); globalRevision=rack.globalRevision();
    graphGeneration=0; error.clear(); work.serial=++serial; waitingSerial=serial;
    { std::lock_guard lock(mutex); queued=std::move(work); result.reset(); } wake.notify_one();
}
void PresetSession::select(const ToneState& t) { request({0,{},t}); }
void PresetSession::importFile(std::filesystem::path path) { request({0,std::move(path),{}}); }
bool PresetSession::cancelLoad() {
    if(graphGeneration&&!rack.cancelTone(graphGeneration)) return false;
    ++serial; waitingSerial=graphGeneration=0; { std::lock_guard lock(mutex); queued.reset(); result.reset(); } return true;
}
void PresetSession::parserLoop() {
    std::unique_lock lock(mutex);
    while(!quitting) {
        wake.wait(lock,[&]{return quitting||queued.has_value();}); if(quitting) break;
        auto work=std::move(*queued); queued.reset(); lock.unlock(); Result value{work.serial,{}, {}};
        try { value.tone=work.tone?parsePreset(serializePreset(*work.tone,rack.registry()),rack.registry()):parsePreset(readBounded(work.file),rack.registry()); }
        catch(const std::exception& e) { value.error=e.what(); }
        lock.lock(); result=std::move(value);
    }
}
void PresetSession::poll() {
    std::optional<Result> ready; { std::lock_guard lock(mutex); if(result) { ready=std::move(result); result.reset(); } }
    if(ready&&ready->serial==waitingSerial) {
        if(bankRevision!=rack.bank.revision()||globalRevision!=rack.globalRevision()) { error="Preset was superseded by a newer control edit."; waitingSerial=0; return; }
        if(!ready->error.empty()) { error=ready->error; waitingSerial=0; }
        else { graphGeneration=rack.loadTone(*ready->tone); if(!graphGeneration) { error=rack.snapshot().error; waitingSerial=0; } }
    }
    auto s=rack.snapshot();
    if(s.toneGeneration!=seenTone&&s.committedTone) {
        seenTone=s.toneGeneration; selection=s.committedTone;
        baseline=serializePreset(*selection,rack.registry());
    }
    if(graphGeneration&&s.toneGeneration==graphGeneration) { waitingSerial=graphGeneration=0; }
    else if(graphGeneration&&!s.tonePending&&!s.error.empty()) { error=s.error; waitingSerial=graphGeneration=0; }
}
bool PresetSession::save() {
    if(busy()) { error="Wait for the rack transaction to finish."; return false; }
    if(selectedIsFactory()) { error="Use Save As to create a user preset."; return false; }
    try { auto t=current(); store.save(t,rack.registry()); selection=t; baseline=serializePreset(t,rack.registry()); error.clear(); refreshLibrary(); return true; }
    catch(const std::exception& e) { error=e.what(); return false; }
}
bool PresetSession::saveAs(std::string name) {
    if(busy()) { error="Wait for the rack transaction to finish."; return false; }
    try { auto t=current(); t.id=newInstanceId(); t.name=std::move(name); store.save(t,rack.registry()); selection=t; baseline=serializePreset(t,rack.registry()); error.clear(); refreshLibrary(); return true; }
    catch(const std::exception& e) { error=e.what(); return false; }
}
bool PresetSession::exportFile(const std::filesystem::path& path) {
    if(busy()) { error="Wait for the rack transaction to finish."; return false; }
    try { atomicWrite(path,serializePreset(current(),rack.registry())); error.clear(); return true; } catch(const std::exception& e) { error=e.what(); return false; }
}
bool PresetSession::resolveDirty(DirtyAction action) { if(action==DirtyAction::cancel) return false; if(action==DirtyAction::save) return save(); return true; }
}
