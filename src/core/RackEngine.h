#pragma once
#include "Effects.h"
#include <condition_variable>
#include <cstdint>
#include <mutex>
#include <thread>

namespace guitarrack {
class PreparedRack;
struct RackSnapshot {
    RackDefinition requested, committed;
    std::uint64_t requestedGeneration = 0, committedGeneration = 0;
    std::string error;
    unsigned graphs = 0, peakGraphs = 0;
    bool preparedWaiting = false;
    std::uint64_t built = 0, reclaimed = 0, stale = 0, coalesced = 0;
};

// One message/control producer; one audio consumer; one preparation/reclamation worker.
// prepare/stop/destruction require callbacks detached and drained by the adapter.
class RackEngine {
public:
    explicit RackEngine(EffectRegistry registry = EffectRegistry::builtIns());
    ~RackEngine();
    RackEngine(const RackEngine&) = delete;
    RackEngine& operator=(const RackEngine&) = delete;
    bool prepare(double sampleRate);
    void stop();
    bool add(const std::string& type = "gain");
    bool remove(const InstanceId&);
    bool move(const InstanceId&, int direction);
    bool submit(const RackDefinition&); // validated structural transaction, no parameter snapshot
    RackSnapshot snapshot() const;
    bool setParameter(const InstanceId&, unsigned bankIndex, float physicalValue);
    bool setBypassed(const InstanceId&, bool);
    const EffectRegistry& registry() const noexcept { return effects; }
    double tailSeconds() const noexcept { return reportedTail.load(std::memory_order_relaxed); }
    ParameterBank bank;
    // Callback entry once per device block, followed by <=512-sample chunks.
    void beginBlock() noexcept;
    void process(float* left, float* right, int samples) noexcept;
private:
    bool submitLocked(const RackDefinition&);
    void workerLoop();
    void activatePending() noexcept;
    void retire(PreparedRack*) noexcept;
    void updateCommitted(); // worker, under mutex
    void stoppedMaintenance(); // worker, under mutex, no callback can be active
    EffectRegistry effects;
    mutable std::mutex mutex;
    std::condition_variable wake;
    std::thread worker;
    bool quitting = false, live = false, resetRequested = false;
    double rate = 48000;
    RackSnapshot state;
    std::uint64_t attempted = 0;
    std::atomic<std::uint64_t> requested{0}, acknowledged{0}, activated{0};
    std::atomic<PreparedRack*> pending{nullptr};
    std::atomic<double> reportedTail{0}; // Conservative accepted/fading graph bound.
    std::atomic<unsigned> activationReaders{0}; // rollback waits off audio for activation acknowledgment
    // Sole audio producer / sole worker consumer. Four graphs total means the
    // four-entry retirement queue cannot overflow, even if worker stalls.
    std::array<PreparedRack*, 4> retirement{};
    std::atomic<unsigned> writeIndex{0}, readIndex{0};
    std::array<std::unique_ptr<PreparedRack>, 4> owned;
    PreparedRack* active = nullptr;
    PreparedRack* fading = nullptr;
    int fadePosition = 0, fadeLength = 960;
    float rackMix = 1, rackTarget = 1, rackStep = 0;
    int rackRemaining = 0;
    std::array<float, rackChunkSize> dryL{}, dryR{}, oldL{}, oldR{};
};
static_assert(std::atomic<std::uint64_t>::is_always_lock_free);
static_assert(std::atomic<double>::is_always_lock_free);
static_assert(std::atomic<PreparedRack*>::is_always_lock_free);
}
