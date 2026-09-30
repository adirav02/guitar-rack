#include "PresetSession.h"
namespace guitarrack {
PresetSession::PresetSession(RackEngine& r,std::filesystem::path root) : recallFile(root.parent_path()/"last-tone.json"), store(std::move(root)), rack(r) {
    baseline=serializePreset(current(),rack.registry()); refreshLibrary(); parser=std::thread([this]{parserLoop();});
}
PresetSession::~PresetSession() { { std::lock_guard lock(mutex); quitting=true; } wake.notify_one(); parser.join(); }
void PresetSession::refreshLibrary() {
    try {
        library=factoryPresets(rack.registry()); std::vector<std::string> errors;
        auto user=store.list(rack.registry(),errors);
        managedIds.clear();
        for(auto& t:user) { managedIds.insert(t.id); std::erase_if(library,[&](const auto& v){return v.id==t.id;}); library.push_back(std::move(t)); }
        if(!errors.empty()) { error="Invalid user presets were not loaded:\n"; for(const auto& e:errors) error+=e+"\n"; }
    } catch(const std::exception& e) { error=e.what(); }
}
ToneState PresetSession::current() const { return rack.captureTone(selection?selection->id:workingId.empty()?"00000000-0000-4000-8000-000000000001":workingId,selection?selection->name:workingId.empty()?"Clean":"Untitled rack"); }
bool PresetSession::busy() const { auto s=rack.snapshot(); return waitingSerial!=0||s.tonePending||s.transitioning||(s.requestedGeneration!=s.committedGeneration&&s.error.empty()); }
bool PresetSession::dirty() const { if(busy()) return false; return !workingId.empty() || serializePreset(current(),rack.registry())!=baseline; }
bool PresetSession::selectedIsFactory() const { if(!selection) return true;
    for(const auto& factory:factoryPresets(rack.registry())) if(factory.id==selection->id) return true;
    return false; }
void PresetSession::request(Work work) {
    if(graphGeneration) rack.cancelTone(graphGeneration);
    bankRevision=rack.bank.revision(); globalRevision=rack.globalRevision();
    graphGeneration=0; error.clear(); work.serial=++serial; waitingSerial=serial;
    { std::lock_guard lock(mutex); queued=std::move(work); result.reset(); } wake.notify_one();
}
void PresetSession::select(const ToneState& t) {
    if (deletedIds.contains(t.id) && !managedIds.contains(t.id)) {
        error="This preset was deleted. Refresh the selection or explicitly import a copy."; return;
    }
    request({0,{},t});
}
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
        seenTone=s.toneGeneration; selection=s.committedTone; workingId.clear();
        baseline=serializePreset(*selection,rack.registry());
    }
    if(graphGeneration&&s.toneGeneration==graphGeneration) { waitingSerial=graphGeneration=0; }
    else if(graphGeneration&&!s.tonePending&&!s.error.empty()) { error=s.error; waitingSerial=graphGeneration=0; }
}
bool PresetSession::save() {
    if(busy()) { error="Wait for the rack transaction to finish."; return false; }
    if(selectedIsFactory()) { error="Use Save As to create a user preset."; return false; }
    try { auto t=current(); store.save(t,rack.registry()); selection=t; workingId.clear(); baseline=serializePreset(t,rack.registry()); error.clear(); refreshLibrary(); return true; }
    catch(const std::exception& e) { error=e.what(); return false; }
}
bool PresetSession::saveAs(std::string name) {
    if(busy()) { error="Wait for the rack transaction to finish."; return false; }
    try { auto t=current(); t.id=newInstanceId(); t.name=std::move(name); store.save(t,rack.registry()); selection=t; workingId.clear(); baseline=serializePreset(t,rack.registry()); error.clear(); refreshLibrary(); return true; }
    catch(const std::exception& e) { error=e.what(); return false; }
}
bool PresetSession::exportFile(const std::filesystem::path& path) {
    if(busy()) { error="Wait for the rack transaction to finish."; return false; }
    try { atomicWrite(path,serializePreset(current(),rack.registry())); error.clear(); return true; } catch(const std::exception& e) { error=e.what(); return false; }
}
bool PresetSession::canDelete() const {
    return !busy() && selection && !selectedIsFactory() && !selection->name.empty()
        && managedIds.contains(selection->id);
}
void PresetSession::persistRecall() const {
    auto tone=selection.value_or(factoryPresets(rack.registry()).front());
    atomicWrite(recallFile,serializePreset(tone,rack.registry()));
}
bool PresetSession::deleteSelected(const std::string& expectedId, DirtyAction action, bool confirmed) {
    // Dialog choices are staged. Cancelling either prompt never saves/discards
    // controls, touches files, invalidates requests or changes the dirty baseline.
    if (!confirmed || action==DirtyAction::cancel) return false;
    if (!canDelete() || selection->id!=expectedId) {
        error="Cannot delete: select a saved user preset and wait for loads/edits to finish."; return false;
    }
    try {
        auto detachedId=newInstanceId();
        auto deleted=deletedIds; deleted.insert(expectedId);
        auto nextLibrary=library;
        std::erase_if(nextLibrary,[&](const auto& tone){return tone.id==expectedId;});
        const auto committedToneGeneration=rack.snapshot().toneGeneration;
        std::optional<ToneState> edited;
        if (action==DirtyAction::save && dirty()) edited=current();
        store.erase(expectedId,rack.registry(),recallFile,edited?&*edited:nullptr);
        // No graph request, reset, parameter write or audio-lifecycle operation.
        // Parser results never write files; supersede all pre-deletion serials.
        ++serial; waitingSerial=graphGeneration=0;
        { std::lock_guard lock(mutex); queued.reset(); result.reset(); }
        seenTone=committedToneGeneration;
        library=std::move(nextLibrary); deletedIds=std::move(deleted); managedIds.erase(expectedId);
        selection.reset(); workingId=std::move(detachedId); error.clear();
        return true;
    } catch(const std::exception& e) {
        error="Preset was not deleted: "+std::string(e.what())+"\nCheck "
            +(store.directory/(expectedId+".json")).string()+" and folder permissions, then retry.";
        return false;
    }
}
bool PresetSession::resolveDirty(DirtyAction action) { if(action==DirtyAction::cancel) return false; if(action==DirtyAction::save) return save(); return true; }
}
