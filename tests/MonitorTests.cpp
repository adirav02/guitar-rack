#include "core/MonitorEngine.h"
#include "core/Routing.h"
#include "core/CallbackGate.h"
#include "HeapAudit.h"
#include <array>
#include <atomic>
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <limits>
#include <new>
#include <thread>

namespace {
using namespace heapAudit;
int failures = 0;
void check(bool result, const char* what) {
    if (!result) { ++failures; std::cerr << "FAIL: " << what << '\n'; }
}
bool near(float a, float b, float tolerance = 0.00001f) { return std::abs(a - b) < tolerance; }
constexpr int capacity = 20000;
using Samples = std::array<float, capacity>;
void settle(guitarrack::MonitorEngine& e) {
    Samples zero{}, l{}, r{};
    e.process(zero.data(), zero.data(), l.data(), r.data(), capacity);
    e.consumeMeters();
}
}
int main() {
    using namespace guitarrack;
    Samples input{}, other{}, l{}, r{};
    input.fill(0.2f); other.fill(-0.4f);
    {
        MonitorControls c; MonitorEngine e(c);
        e.process(input.data(), input.data(), l.data(), r.data(), 128);
        check(l[0] == 0 && r[127] == 0, "startup without prepare is silent");
        check(c.outputDb() == -12, "conservative default output");
        e.prepare(48000); e.process(input.data(), input.data(), l.data(), r.data(), 960);
        check(l[0] > 0 && l[0] < 0.0001f, "Start fades from silence");
        for (int i = 1; i < 960; ++i) check(l[i] >= l[i - 1] && l[i] - l[i - 1] < 0.0001f, "Start ramp continuity");
        check(near(l[959], 0.2f * std::pow(10.f, -12.f / 20)), "default gain amplitude");
    }
    {
        MonitorControls c; c.setOutputDb(0); MonitorEngine e(c);
        for (double rate : {8000.0, 44100.0, 48000.0, 96000.0}) {
            check(e.prepare(rate), "supported sample rate preparation"); settle(e);
            e.process(input.data(), other.data(), l.data(), r.data(), 177);
            for (int i = 0; i < 177; ++i) check(l[i] == input[i] && r[i] == other[i], "neutral stereo transparency");
        }
        input.fill(0); input[123] = 0.4f;
        e.process(input.data(), input.data(), l.data(), r.data(), 513);
        for (int i = 0; i < 513; ++i) check(l[i] == input[i] && r[i] == input[i], "mono impulse timing and identical outputs");
        check(!e.prepare(std::numeric_limits<double>::quiet_NaN()), "invalid sample rate rejected");
        e.process(input.data(), input.data(), l.data(), r.data(), 513);
        check(l[123] == 0, "invalid preparation produces silence");
    }
    input.fill(0.2f);
    {
        MonitorControls c; c.setOutputDb(0); MonitorEngine e(c); e.prepare(48000); settle(e);
        check(c.setInputDb(6) && c.setOutputDb(-6), "bounded gain values accepted");
        settle(e); e.process(input.data(), input.data(), l.data(), r.data(), 129);
        check(near(l[128], 0.2f), "trim and level compose in dB");
        const auto gain = c.inputGain();
        check(!c.setInputDb(std::numeric_limits<float>::infinity()) && !c.setInputDb(25), "invalid input controls rejected");
        check(!c.setOutputDb(-61) && !c.setOutputDb(std::numeric_limits<float>::quiet_NaN()), "invalid output controls rejected");
        check(c.inputGain() == gain && c.outputDb() == -6, "rejected controls preserve previous values");
    }
    {
        MonitorControls c; MonitorEngine e(c); e.prepare(48000); settle(e);
        c.setOutputDb(0);
        float previous = 0.2f * c.outputGain();
        e.process(input.data(), input.data(), l.data(), r.data(), 0);
        e.process(input.data(), input.data(), l.data(), r.data(), 960);
        previous = l[0];
        for (int i = 1; i < 960; ++i) {
            check(l[i] >= previous && l[i] - previous < 0.0002f, "gain ramp has bounded sample steps");
            previous = l[i];
        }
        check(near(l[959], 0.2f), "gain ramp reaches target after 20 ms");
        e.consumeMeters(); c.setMuted(true);
        e.process(input.data(), input.data(), l.data(), r.data(), 960);
        for (int i = 1; i < 960; ++i) check(l[i] <= l[i - 1], "mute fades monotonically");
        check(l[959] == 0 && e.isSilent(), "mute reaches exact silence");
        e.consumeMeters(); e.process(input.data(), input.data(), l.data(), r.data(), 31);
        auto meters = e.consumeMeters();
        check(meters.inputPeak > 0 && meters.outputPeak == 0, "input meter remains active while muted");
        c.setMuted(false); e.process(input.data(), input.data(), l.data(), r.data(), 960);
        e.process(input.data(), input.data(), l.data(), r.data(), 960, false);
        check(l[959] == 0 && r[959] == 0, "Stop fades to silence");
        e.reset(); e.process(input.data(), input.data(), l.data(), r.data(), 1, false);
        check(l[0] == 0, "reset retains stopped silence");
    }
    {
        MonitorControls c; c.setInputDb(24); c.setOutputDb(6);
        MonitorEngine e(c); e.prepare(48000); settle(e);
        input[0] = std::numeric_limits<float>::quiet_NaN();
        input[1] = std::numeric_limits<float>::infinity();
        input[2] = std::numeric_limits<float>::max(); input[3] = -std::numeric_limits<float>::max();
        e.process(input.data(), input.data(), l.data(), r.data(), 100);
        for (int i = 0; i < 100; ++i) check(std::isfinite(l[i]) && std::abs(l[i]) <= 0.98f && l[i] == r[i], "finite bounded output protection");
        check(l[0] == 0 && l[1] == 0 && l[2] == 0.98f && l[3] == -0.98f, "invalid samples silent and protection ceiling exact");
        const auto meter = e.consumeMeters();
        check(meter.flags == (MonitorEngine::inputClipped | MonitorEngine::outputProtected | MonitorEngine::nonFiniteInput), "clipping and invalid input reported");
        check(e.consumeMeters().flags == 0, "GUI consumes latched diagnostics");
    }
    input.fill(0.2f);
    {
        MonitorControls c; c.setOutputDb(0); MonitorEngine e(c); e.prepare(48000); settle(e);
        StereoRoute route{7, 3};
        check(route.valid() && route.packedLeft() == 1 && route.packedRight() == 0, "physical-to-packed stereo mapping");
        const float* physicalInputs[]{other.data(), input.data()}; // physical Input 2
        const float* packedInput[]{physicalInputs[1]};
        float* packedOutputs[]{r.data(), l.data()};
        e.process(packedInput[0], packedInput[0], packedOutputs[route.packedLeft()], packedOutputs[route.packedRight()], 128);
        check(l[127] == 0.2f && r[127] == 0.2f, "physical Input 2 selected alone reaches both ears");
        check(!StereoRoute{2, 2}.valid() && !StereoRoute{-1, 0}.valid(), "invalid stereo routes rejected");
        float* outputs[]{l.data(), nullptr, r.data()};
        clearOutputs(outputs, 3, 128);
        check(l[127] == 0 && r[127] == 0, "failure clears every available output including unused channels");
        l.fill(7); r.fill(7);
        e.process(input.data(), input.data(), l.data(), r.data(), 0);
        check(l[0] == 7 && r[0] == 7, "zero-length blocks leave memory untouched");
        allocations = deallocations = 0; audit = true;
        e.process(input.data(), input.data(), l.data(), r.data(), 8193);
        e.process(input.data(), input.data(), l.data(), r.data(), 1);
        clearOutputs(outputs, 3, 32);
        CallbackGate<int> gate;
        { CallbackGate<int>::Lease lease(gate); }
        audit = false;
        check(allocations == 0 && deallocations == 0, "callback processing, routing and lease perform no heap operations");
        check(l[8192] == 0.2f && r[8192] == 0.2f && l[8193] == 7, "oversized block stays within boundaries");
        e.process(nullptr, nullptr, l.data(), r.data(), 128);
        check(l[127] == 0 && r[127] == 0, "missing inputs produce silence");
    }
    {
        // Concurrent callback detach/replacement: old callback destruction follows
        // reader acknowledgment, and callback work has no retries or waits.
        struct Object { std::atomic<unsigned> visits{0}; } first, second;
        CallbackGate<Object> gate; gate.replace(&first);
        std::atomic<bool> done{false}, entered{false};
        std::thread reader([&] {
            while (!done.load()) {
                CallbackGate<Object>::Lease lease(gate);
                if (auto* object = lease.get()) { object->visits.fetch_add(1); entered.store(true); }
            }
        });
        while (!entered.load()) std::this_thread::yield();
        for (int i = 0; i < 1000; ++i) gate.replace(i % 2 == 0 ? &second : &first);
        gate.replace(nullptr);
        const auto before = first.visits.load() + second.visits.load();
        for (int i = 0; i < 100; ++i) std::this_thread::yield();
        check(first.visits.load() + second.visits.load() == before, "detachment acknowledges all old callback accesses");
        done.store(true); reader.join();
    }
    if (failures) { std::cerr << failures << " failed checks\n"; return 1; }
    std::cout << "PASS: startup silence, routing, impulse timing, gain/mute/stop ramps, meters, protection, variable blocks, allocation audit, callback reclamation\n";
}
