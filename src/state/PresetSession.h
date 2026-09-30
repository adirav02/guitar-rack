#pragma once
#include "Presets.h"
#include "core/RackEngine.h"
#include <condition_variable>
#include <optional>
#include <thread>

namespace guitarrack {
enum class DirtyAction { save, discard, cancel };
// One control/message-thread producer; one file parser worker. UI-neutral for later host use.
class PresetSession {
public:
    PresetSession(RackEngine&, std::filesystem::path userDirectory);
    ~PresetSession();
    void select(const ToneState&);
    void importFile(std::filesystem::path);
    bool cancelLoad();
    void poll();
    bool busy() const;
    bool loading() const { return waitingSerial != 0 || rack.snapshot().tonePending; }
    bool dirty() const;
    bool save();
    bool saveAs(std::string name);
    bool exportFile(const std::filesystem::path&);
    bool resolveDirty(DirtyAction); // false preserves current state/action
    ToneState current() const;
    const std::optional<ToneState>& selected() const { return selection; }
    void refreshLibrary();
    std::vector<ToneState> library;
    std::string error;
    bool selectedIsFactory() const;
    PresetStore store;
private:
    RackEngine& rack;
    std::optional<ToneState> selection;
    std::string baseline;
    struct Work { std::uint64_t serial; std::filesystem::path file; std::optional<ToneState> tone; };
    struct Result { std::uint64_t serial; std::optional<ToneState> tone; std::string error; };
    std::mutex mutex;
    std::condition_variable wake;
    std::thread parser;
    bool quitting=false;
    std::optional<Work> queued;
    std::optional<Result> result;
    std::uint64_t serial=0, waitingSerial=0, graphGeneration=0, seenTone=0;
    std::uint64_t bankRevision=0, globalRevision=0;
    void request(Work);
    void parserLoop();
};
}
