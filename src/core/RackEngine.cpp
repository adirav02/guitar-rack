#include "RackEngine.h"
#include "MonitorEngine.h"
#include "LinearRamp.h"
#include <algorithm>
#include <chrono>
#include <cmath>
#include <stdexcept>

namespace guitarrack {
namespace {
float blend(float dry, float wet, float mix) noexcept {
    // Exact endpoints preserve dry bypass even after invalid DSP and avoid
    // cancellation when replacing chains with widely different gain.
    if (mix <= 0) return dry;
    if (mix >= 1) return wet;
    return (1 - mix) * dry + mix * wet;
}
}
class PreparedRack {
public:
    struct Node {
        RackInstance identity;
        std::unique_ptr<EffectProcessor> processor;
        LinearRamp wet;
    };
    PreparedRack(ParameterBank& authority, RackDefinition d, std::uint64_t g)
        : authority(authority), definition(std::move(d)), generation(g) {
        for (const auto& i : definition.instances) mask |= 1u << i.slot;
        authority.reserve(mask);
    }
    ~PreparedRack() { authority.unreserve(mask); }
    void build(const EffectRegistry& registry, double rate) {
        if (tone) {
            inputGain=std::pow(10.0f,tone->controls.inputDb/20);
            outputGain=std::pow(10.0f,tone->controls.outputDb/20);
        }
        rampSamples = static_cast<int>(std::ceil(rate * 0.020));
        nodes.reserve(definition.order.size());
        for (const auto& id : definition.order) {
            const auto& i = *definition.find(id);
            auto processor = registry.find(i.effectType, i.effectVersion)->factory(bank, i.slot);
            if (!processor || !processor->prepare(rate, rackChunkSize)) throw std::runtime_error("Effect preparation failed.");
            // M3 permits bounded temporal tails, but all dry/transition paths still
            // require zero latency. History may fade out on structural replacement.
            const double tail = processor->tailSeconds();
            if (processor->latencySamples() != 0 || !std::isfinite(tail) || tail < 0 || tail > 120)
                throw std::runtime_error("Rack requires zero latency and a finite tail of at most 120 seconds.");
            tailBound += tail;
            nodes.push_back({i, std::move(processor), {}});
        }
    }
    void sync() noexcept {
        for (const auto& i : definition.instances) {
            for (unsigned p = 0; p < controlsPerSlot; ++p) bank.setValue(i.slot, p, authority.value(i.slot, p));
            bank.setBypassed(i.slot, authority.bypassed(i.slot));
        }
    }
    void activate(const PreparedRack* previous) noexcept {
        sync();
        for (auto& node : nodes) {
            const Node* match = nullptr;
            if (previous) for (const auto& old : previous->nodes)
                if (old.identity == node.identity) { match = &old; break; }
            node.processor->activate(match ? match->processor.get() : nullptr);
            if (match) node.wet = match->wet;
            else node.wet.reset(bank.bypassed(node.identity.slot) ? 0.0f : 1.0f);
        }
    }
    void reset() noexcept {
        sync();
        for (auto& n : nodes) { n.processor->reset(); n.wet.reset(bank.bypassed(n.identity.slot) ? 0.0f : 1.0f); }
    }
    void process(float* l, float* r, int count) noexcept {
        for (auto& node : nodes) {
            std::copy_n(l, count, dryL.data()); std::copy_n(r, count, dryR.data());
            node.processor->process(l, r, count);
            node.wet.aim(bank.bypassed(node.identity.slot) ? 0.0f : 1.0f, rampSamples);
            for (int i = 0; i < count; ++i) {
                const float mix = node.wet.next();
                l[i] = blend(dryL[i], l[i], mix);
                r[i] = blend(dryR[i], r[i], mix);
            }
        }
    }
    ParameterBank& authority;
    ParameterBank bank;
    std::optional<ToneState> tone;
    bool frozen = false;
    float inputGain=1, outputGain=0.2511886432f;
    std::uint64_t bankRevision=0, globalRevision=0, toneRequestGeneration=0;
    const RackDefinition definition;
    const std::uint64_t generation;
    double tailBound = 0; // Sum of serial stage bounds, built only by worker.
private:
    unsigned mask = 0;
    int rampSamples = 960;
    std::vector<Node> nodes;
    std::array<float, rackChunkSize> dryL{}, dryR{};
};
RackEngine::RackEngine(EffectRegistry registry) : effects(std::move(registry)) { worker = std::thread([this] { workerLoop(); }); }
RackEngine::~RackEngine() {
    stop();
    { std::lock_guard lock(mutex); quitting = true; }
    wake.notify_one(); worker.join(); // worker destroys every remaining owned graph before exit
}
RackSnapshot RackEngine::snapshot() const {
    std::lock_guard lock(mutex); auto result = state;
    result.preparedWaiting = pending.load(std::memory_order_acquire) != nullptr;
    result.transitioning = activated.load() != acknowledged.load();
    return result;
}
std::uint64_t RackEngine::globalRevision() const noexcept { return globals ? globals->revision() : 0; }
std::uint64_t RackEngine::loadTone(const ToneState& tone) {
    std::lock_guard lock(mutex);
    if (auto error = validateTone(tone, effects); !error.empty()) { state.error = error; return 0; }
    // The same persistent UUID cannot acquire a different slot/type/version.
    for (const auto& i : tone.rack.instances) {
        auto same=[&](const RackDefinition& d) { const auto* old=d.find(i.id); return !old || *old==i; };
        if (!same(state.requested) || !same(state.committed)
            || (requestedTone && !same(requestedTone->rack)) || (buildingDefinition && !same(*buildingDefinition))) {
            state.error="Preset changes an immutable instance identity."; return 0;
        }
        for (const auto& graph : owned) if (graph) {
            if (const auto* old = graph->definition.find(i.id); old && *old != i) {
                state.error = "Preset changes an immutable instance identity."; return 0;
            }
        }
    }
    // Publish the new generation first, then drain any activation that already
    // accepted an older request. Keep the control mutex: the builder cannot
    // snapshot this transaction halfway through. Audio never acquires this mutex.
    requested.store(++state.requestedGeneration);
    while (activationReaders.load() != 0) std::this_thread::yield();
    toneBankRevision=bank.revision(); toneGlobalRevision=globalRevision();
    toneRequestGeneration=state.requestedGeneration;
    requestedTone = tone; state.tonePending = true; state.error.clear();
    wake.notify_one(); return state.requestedGeneration;
}
bool RackEngine::cancelTone(std::uint64_t generation) {
    std::unique_lock lock(mutex);
    if (!requestedTone || toneRequestGeneration != generation) return false;
    // Supersede first; an already accepted transition is allowed to complete.
    requested.store(++state.requestedGeneration);
    while (activationReaders.load() != 0) std::this_thread::yield();
    bool accepted=false;
    for (const auto& g : owned) if (g && g->toneRequestGeneration==generation && g->generation==activated.load()) {
        state.requested=g->definition; accepted=true;
    }
    if (accepted) {
        requestedTone.reset(); state.tonePending = false; wake.notify_one(); return false;
    }
    requestedTone.reset(); state.tonePending = false; wake.notify_one(); return true;
}
ToneState RackEngine::captureTone(std::string id, std::string name) const {
    std::lock_guard lock(mutex);
    if (state.tonePending || activated.load()!=acknowledged.load()) throw std::runtime_error("Cannot capture tone during preset activation.");
    ToneState tone; tone.id = std::move(id); tone.name = std::move(name); tone.rack = state.requested;
    // Control producer calls this only after tone acknowledgment. No audio writer
    // touches the bank during normal edits; full loads are explicitly pending.
    for (unsigned s = 0; s < rackSlots; ++s) {
        for (unsigned p = 0; p < controlsPerSlot; ++p) tone.controls.values[s][p] = bank.value(s,p);
        tone.controls.bypass[s] = bank.bypassed(s);
    }
    tone.controls.rackBypass = bank.rackBypassed();
    if (globals) { tone.controls.inputDb = globals->inputDb(); tone.controls.outputDb = globals->outputDb(); }
    return tone;
}
bool RackEngine::submitLocked(const RackDefinition& d) {
    if (requestedTone) { state.error = "Wait for preset loading to finish."; return false; }
    if (auto error = validateRack(d, effects); !error.empty()) { state.error = error; return false; }
    // Existing identities are immutable, including graphs still in transit/retirement.
    for (const auto& i : d.instances) {
        const auto check = [&](const RackDefinition& old) {
            const auto* match = old.find(i.id);
            return !match || *match == i;
        };
        if (!check(state.requested) || !check(state.committed)) { state.error = "Instance identity cannot be changed."; return false; }
        const auto* old = state.requested.find(i.id);
        if (!old) {
            // A restored identity is allowed only in its exact already-reserved slot.
            bool same = false;
            for (const auto& graph : owned) if (graph) {
                if (!check(graph->definition)) { state.error = "Instance identity cannot be changed."; return false; }
                if (const auto* found = graph->definition.find(i.id)) same |= *found == i;
            }
            if (bank.references(i.slot) != 0 && !same) { state.error = "Slot is still in use by a retiring graph; retry shortly."; return false; }
        }
    }
    if (state.requestedGeneration > attempted) ++state.coalesced;
    state.requested = d; state.error.clear();
    requested.store(++state.requestedGeneration);
    wake.notify_one(); return true;
}
bool RackEngine::submit(const RackDefinition& d) { std::lock_guard lock(mutex); return submitLocked(d); }
bool RackEngine::add(const std::string& type) {
    std::lock_guard lock(mutex);
    if (requestedTone) return false;
    const auto* descriptor = effects.find(type, 1);
    if (!descriptor) { state.error = "Unknown effect."; return false; }
    unsigned used = 0;
    for (const auto& i : state.requested.instances) used |= 1u << i.slot;
    SlotId slot = 0;
    for (; slot < rackSlots; ++slot) if (!(used & (1u << slot)) && bank.references(slot) == 0) break;
    if (slot == rackSlots) { state.error = "All 16 slots are occupied or retiring; retry after the transition."; return false; }
    RackDefinition next = state.requested;
    const auto id = newInstanceId(); next.instances.push_back({id, slot, type, descriptor->version}); next.order.push_back(id);
    for (unsigned p = 0; p < controlsPerSlot; ++p) bank.setValue(slot, p, 0);
    for (const auto& p : descriptor->parameters) bank.setValue(slot, p.bankIndex, p.normalize(p.defaultValue));
    bank.setBypassed(slot, false);
    return submitLocked(next);
}
bool RackEngine::remove(const InstanceId& id) {
    std::lock_guard lock(mutex); auto next = state.requested;
    if (!next.find(id)) return false;
    std::erase_if(next.instances, [&](const auto& i) { return i.id == id; }); std::erase(next.order, id);
    return submitLocked(next);
}
bool RackEngine::move(const InstanceId& id, int direction) {
    std::lock_guard lock(mutex); auto next = state.requested;
    auto at = std::find(next.order.begin(), next.order.end(), id);
    if (at == next.order.end() || (direction != -1 && direction != 1)) return false;
    const auto index = at - next.order.begin(), destination = index + direction;
    if (destination < 0 || destination >= static_cast<int>(next.order.size())) return false;
    std::swap(next.order[index], next.order[destination]); return submitLocked(next);
}
bool RackEngine::setParameter(const InstanceId& id, unsigned index, float physical) {
    std::lock_guard lock(mutex);
    if (requestedTone) return false;
    const auto* i = state.requested.find(id); if (!i) return false;
    const auto* effect = effects.find(i->effectType, i->effectVersion);
    for (const auto& p : effect->parameters) if (p.bankIndex == index) {
        if (!std::isfinite(physical) || physical < p.minimum || physical > p.maximum) return false;
        return bank.setValue(i->slot, index, p.normalize(physical));
    }
    return false;
}
bool RackEngine::setBypassed(const InstanceId& id, bool bypass) {
    std::lock_guard lock(mutex); if (requestedTone) return false; const auto* i = state.requested.find(id); if (!i) return false;
    bank.setBypassed(i->slot, bypass); return true;
}
bool RackEngine::prepare(double sampleRate) {
    if (!std::isfinite(sampleRate) || sampleRate < 8000 || sampleRate > 768000) return false;
    stop();
    std::unique_lock lock(mutex); rate = sampleRate; fadeLength = static_cast<int>(std::ceil(rate * 0.020));
    requested.store(++state.requestedGeneration); state.error.clear(); wake.notify_one();
    wake.wait(lock, [&] { return attempted == state.requestedGeneration && (acknowledged.load() == attempted || !state.error.empty()); });
    if (!state.error.empty()) return false;
    live = true; return true;
}
void RackEngine::stop() {
    std::unique_lock lock(mutex); live = false; resetRequested = true; wake.notify_one();
    wake.wait(lock, [&] { return !resetRequested; });
}
void RackEngine::retire(PreparedRack* graph) noexcept {
    if (!graph) return;
    const auto w = writeIndex.load(std::memory_order_relaxed);
    retirement[w % retirement.size()] = graph;
    writeIndex.store(w + 1, std::memory_order_release);
}
void RackEngine::activatePending() noexcept {
    if (fading) return;
    auto* next = pending.exchange(nullptr, std::memory_order_acq_rel);
    if (!next) return;
    if (next->generation != requested.load()) { retire(next); return; }
    if (next->tone && (next->bankRevision != bank.revision() || next->globalRevision != globalRevision())) {
        rejectedTone.store(next->generation, std::memory_order_release); retire(next); return;
    }
    if (next->tone) {
        // The old graph keeps its own controls during the crossfade, even if slots
        // now describe different effects. No DSP is constructed/destroyed here.
        if (active) { active->sync(); active->frozen = true; }
        const auto& c = next->tone->controls;
        for (unsigned s = 0; s < rackSlots; ++s) {
            for (unsigned p = 0; p < controlsPerSlot; ++p) bank.setValue(s, p, c.values[s][p]);
            bank.setBypassed(s, c.bypass[s]);
        }
        bank.setRackBypassed(c.rackBypass);
        if (globals) globals->restorePreparedGains(c.inputDb,c.outputDb,next->inputGain,next->outputGain);
    }
    next->activate(active); fading = active; active = next; fadePosition = 0;
    reportedTail.store(std::max(active->tailBound, fading ? fading->tailBound : 0), std::memory_order_relaxed);
    activated.store(active->generation, std::memory_order_release);
    if (!fading) acknowledged.store(active->generation, std::memory_order_release);
}
void RackEngine::beginBlock() noexcept {
    activationReaders.fetch_add(1);
    activatePending();
    activationReaders.fetch_sub(1);
}
void RackEngine::process(float* l, float* r, int count) noexcept {
    // Only bounded chunks enter here. No loops depend on queue occupancy.
    std::copy_n(l, count, dryL.data()); std::copy_n(r, count, dryR.data());
    if (fading) { std::copy_n(l, count, oldL.data()); std::copy_n(r, count, oldR.data()); if (!fading->frozen) fading->sync(); fading->process(oldL.data(), oldR.data(), count); }
    if (active) { active->sync(); active->process(l, r, count); }
    const float target = bank.rackBypassed() ? 0.0f : 1.0f;
    if (target != rackTarget) { rackTarget = target; rackRemaining = fadeLength; rackStep = (target - rackMix) / fadeLength; }
    for (int i = 0; i < count; ++i) {
        if (fading) {
            const float mix = std::min(1.0f, static_cast<float>(++fadePosition) / fadeLength);
            l[i] = blend(oldL[i], l[i], mix); r[i] = blend(oldR[i], r[i], mix);
        }
        if (rackRemaining > 0) { rackMix += rackStep; if (--rackRemaining == 0) rackMix = rackTarget; }
        l[i] = blend(dryL[i], l[i], rackMix); r[i] = blend(dryR[i], r[i], rackMix);
    }
    if (fading && fadePosition >= fadeLength) { retire(fading); fading = nullptr; reportedTail.store(active->tailBound, std::memory_order_relaxed); acknowledged.store(active->generation, std::memory_order_release); }
}
void RackEngine::updateCommitted() {
    const auto ack = acknowledged.load(std::memory_order_acquire);
    if (ack == state.committedGeneration) return;
    for (const auto& graph : owned) if (graph && graph->generation == ack) {
        state.committed = graph->definition; state.committedGeneration = ack;
        if (graph->tone) {
            state.committedTone = graph->tone; state.toneGeneration = graph->toneRequestGeneration;
            if (ack == state.requestedGeneration) { state.requested = graph->definition; requestedTone.reset(); state.tonePending = false; }
        }
        break;
    }
}
void RackEngine::stoppedMaintenance() {
    if (resetRequested) {
        if (fading) { retire(fading); fading = nullptr; }
        if (active) { active->reset(); acknowledged.store(active->generation); }
        rackMix = rackTarget = bank.rackBypassed() ? 0.0f : 1.0f; rackRemaining = 0;
        resetRequested = false; wake.notify_all();
    }
    activatePending();
    if (fading) { retire(fading); fading = nullptr; }
    reportedTail.store(active ? active->tailBound : 0, std::memory_order_relaxed);
    if (active) acknowledged.store(active->generation);
}
void RackEngine::workerLoop() {
    std::unique_lock lock(mutex);
    while (!quitting) {
        if (!live) stoppedMaintenance();
        updateCommitted();
        if (requestedTone && rejectedTone.load(std::memory_order_acquire) == state.requestedGeneration) {
            state.error="Preset was superseded by a newer control edit.";
            requestedTone.reset(); state.tonePending=false;
        }
        auto r = readIndex.load(std::memory_order_relaxed);
        const auto w = writeIndex.load(std::memory_order_acquire);
        while (r != w) {
            auto* dead = retirement[r % retirement.size()];
            for (auto& graph : owned) if (graph.get() == dead) { graph.reset(); ++state.reclaimed; --state.graphs; break; }
            ++r;
        }
        readIndex.store(r, std::memory_order_release);
        if (attempted != state.requestedGeneration && pending.load() == nullptr && state.graphs < owned.size()) {
            const auto generation = state.requestedGeneration; const auto definition = requestedTone ? requestedTone->rack : state.requested; const auto tone = requestedTone; const auto buildRate = rate;
            attempted = generation;
            // Reservation happens under the control mutex, before the slow factory
            // runs, so a removed slot cannot be reused while it is being built.
            auto graph = std::make_unique<PreparedRack>(bank, definition, generation);
            graph->tone = tone; graph->bankRevision=toneBankRevision; graph->globalRevision=toneGlobalRevision; graph->toneRequestGeneration=tone ? toneRequestGeneration : 0;
            ++state.graphs; ++state.built; state.peakGraphs = std::max(state.peakGraphs, state.graphs);
            buildingDefinition=definition;
            lock.unlock();
            std::string failure;
            try { graph->build(effects, buildRate); } catch (const std::exception& e) { failure = e.what(); } catch (...) { failure = "Effect preparation threw an unknown exception."; }
            lock.lock(); buildingDefinition.reset();
            if (generation != state.requestedGeneration || !failure.empty()) {
                graph.reset(); --state.graphs; ++state.reclaimed;
                // A callback may have accepted an earlier request just before
                // this request was published. Its activation must finish before
                // rollback selects the accepted destination. Only worker waits;
                // new audio entries see this failed generation and reject older
                // pending graphs. A new control request cancels this rollback.
                while (generation == state.requestedGeneration && activationReaders.load() != 0) {
                    lock.unlock(); std::this_thread::sleep_for(std::chrono::milliseconds(1)); lock.lock();
                }
                if (generation != state.requestedGeneration) ++state.stale;
                else {
                    updateCommitted(); state.error = failure; requestedTone.reset(); state.tonePending = false;
                    // Retain the last accepted destination if a fade is in flight.
                    state.requested = state.committed;
                    const auto accepted = activated.load(std::memory_order_acquire);
                    for (const auto& current : owned) if (current && current->generation == accepted) state.requested = current->definition;
                }
                wake.notify_all();
            } else {
                for (auto& place : owned) if (!place) { place = std::move(graph); pending.store(place.get(), std::memory_order_release); break; }
            }
            continue;
        }
        wake.notify_all();
        wake.wait_for(lock, std::chrono::milliseconds(2)); // callback never notifies/locks
    }
    pending.store(nullptr); active = fading = nullptr;
    for (auto& graph : owned) if (graph) { graph.reset(); ++state.reclaimed; --state.graphs; }
}
}
