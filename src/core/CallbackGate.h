#pragma once
#include <atomic>
#include <thread>

namespace guitarrack {
// Exactly one control producer; any reader is counted before observing the pointer.
// The producer detaches then drains OUTSIDE the audio callback before preparation
// or destruction. Sequential consistency makes that ordering explicit.
template <typename T> class CallbackGate {
public:
    static_assert(std::atomic<T*>::is_always_lock_free);
    static_assert(std::atomic<unsigned>::is_always_lock_free);
    class Lease {
    public:
        explicit Lease(CallbackGate& owner) noexcept : gate(owner) {
            gate.readers.fetch_add(1);
            pointer = gate.pointer.load();
        }
        ~Lease() { gate.readers.fetch_sub(1); }
        Lease(const Lease&) = delete;
        Lease& operator=(const Lease&) = delete;
        T* get() const noexcept { return pointer; }
    private:
        CallbackGate& gate;
        T* pointer;
    };
    T* get() const noexcept { return pointer.load(); }
    // Control/lifecycle thread only. Never call from a Lease's reader thread.
    void replace(T* next) noexcept {
        pointer.store(nullptr);
        while (readers.load() != 0)
            std::this_thread::yield();
        pointer.store(next);
    }
private:
    std::atomic<T*> pointer{nullptr};
    std::atomic<unsigned> readers{0};
};
}
