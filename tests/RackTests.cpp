#include "core/RackEngine.h"
#include "core/MonitorEngine.h"
#include "HeapAudit.h"
#include <array>
#include <atomic>
#include <chrono>
#include <cmath>
#include <iostream>
#include <limits>
#include <thread>

namespace {
using namespace guitarrack;
int failures = 0;
void check(bool condition, const char* message) {
    if (!condition) { ++failures; std::cerr << "FAIL: " << message << '\n'; }
}
bool near(float a, float b) { return std::abs(a - b) < 0.00002f; }
template<class Predicate> bool waitFor(Predicate predicate) {
    const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(4);
    do { if (predicate()) return true; std::this_thread::sleep_for(std::chrono::milliseconds(1)); }
    while (std::chrono::steady_clock::now() < deadline);
    return false;
}
struct Audio {
    std::array<float, rackChunkSize> l{}, r{};
    bool noHeap = true;
    void block(RackEngine& rack, int count = rackChunkSize, float sample = 0.1f) {
        l.fill(sample); r.fill(sample);
        heapAudit::begin(); rack.beginBlock(); rack.process(l.data(), r.data(), count);
        noHeap &= heapAudit::finish();
    }
    bool commit(RackEngine& rack) {
        return waitFor([&] { block(rack); const auto s = rack.snapshot(); return s.committedGeneration == s.requestedGeneration; });
    }
};
struct Probe {
    std::atomic<bool> blocked{false}, entered{false}, release{false};
    std::atomic<unsigned> made{0}, destroyed{0}, destroyedInAudio{0};
};
class TestProcessor final : public EffectProcessor {
public:
    TestProcessor(Probe& p, float multiplier, float offset, bool fail, bool invalid)
        : probe(p), multiply(multiplier), add(offset), fail(fail), invalid(invalid) { ++probe.made; }
    ~TestProcessor() override { ++probe.destroyed; if (heapAudit::audit) ++probe.destroyedInAudio; }
    bool prepare(double, int) override {
        if (probe.blocked.load()) { probe.entered.store(true); while (!probe.release.load()) std::this_thread::sleep_for(std::chrono::milliseconds(1)); }
        if (fail) throw std::runtime_error("Injected preparation failure");
        return true;
    }
    void reset() noexcept override {}
    void activate(const EffectProcessor*) noexcept override {}
    void process(float* l, float* r, int n) noexcept override {
        for (int i = 0; i < n; ++i) { l[i] = invalid ? std::numeric_limits<float>::quiet_NaN() : l[i] * multiply + add; r[i] = l[i]; }
    }
private:
    Probe& probe;
    float multiply, add;
    bool fail, invalid;
};
void registerProbe(EffectRegistry& registry, Probe& probe, const std::string& type, float multiply, float add, bool fail = false, bool invalid = false) {
    check(registry.add({type, type, "Test only", 1, {}, [&probe, multiply, add, fail, invalid](const ParameterBank&, SlotId) {
        return std::make_unique<TestProcessor>(probe, multiply, add, fail, invalid);
    }}), "register test-only processor");
}
RackDefinition definition(std::initializer_list<std::string> types) {
    RackDefinition d; unsigned slot = 0;
    for (const auto& type : types) { auto id = newInstanceId(); d.instances.push_back({id, slot++, type, 1}); d.order.push_back(id); }
    return d;
}
void identityAndGain() {
    RackEngine rack; Audio audio;
    check(rack.prepare(48000), "prepare rack at actual sample rate");
    check(rack.add() && rack.add(), "multiple independent Gain instances accepted");
    check(audio.commit(rack), "independent Gains committed");
    const auto original = rack.snapshot().committed;
    const auto a = original.order[0], b = original.order[1];
    const auto slotA = original.find(a)->slot, slotB = original.find(b)->slot;
    check(a != b && slotA != slotB && validInstanceId(a), "unique persistent UUIDs and immutable slots");
    audio.block(rack); check(near(audio.l.back(), 0.1f), "neutral Gains transparent");
    check(rack.setParameter(a, 0, 6) && rack.setParameter(b, 0, -12), "independent level setters");
    for (int i = 0; i < 4; ++i) audio.block(rack);
    check(near(audio.l.back(), 0.1f * std::pow(10.f, -6.f / 20)), "Gain amplitudes compose in dB");
    check(!rack.setParameter(a, 0, 25) && !rack.setParameter(a, 0, std::numeric_limits<float>::quiet_NaN()), "invalid physical controls rejected");
    check(rack.move(b, -1), "reorder accepted");
    // Movement must not replay the parameter values from a structural snapshot.
    rack.bank.setValue(slotA, 0, (3.0f + 60.0f) / 84.0f);
    check(audio.commit(rack), "reorder committed");
    const auto reordered = rack.snapshot().committed;
    check(reordered.order[0] == b && reordered.instances == original.instances, "reorder modifies order only");
    for (int i = 0; i < 4; ++i) audio.block(rack);
    check(near(audio.l.back(), 0.1f * std::pow(10.f, -9.f / 20)), "activation resolves latest bank controls without stale overwrite");
    // A live level change must traverse a bounded monotonic amplitude ramp.
    rack.setParameter(a, 0, -3);
    float gainPrevious = audio.l.back(); bool gainSmooth = true;
    for (int block = 0; block < 3; ++block) {
        audio.block(rack);
        for (float sample : audio.l) { gainSmooth &= sample <= gainPrevious + 0.000001f && std::abs(sample - gainPrevious) < 0.0003f; gainPrevious = sample; }
    }
    check(gainSmooth && near(gainPrevious, 0.1f * std::pow(10.f, -15.f / 20)), "Gain level smoothing is monotonic and reaches target");
    rack.setParameter(a, 0, 3); for (int i = 0; i < 3; ++i) audio.block(rack);
    rack.setBypassed(b, true);
    float previous = audio.l.back(); bool continuous = true;
    for (int block = 0; block < 3; ++block) {
        audio.block(rack);
        for (float sample : audio.l) { continuous &= std::abs(sample - previous) < 0.0003f; previous = sample; }
    }
    check(continuous && near(previous, 0.1f * std::pow(10.f, 3.f / 20)), "individual bypass ramps continuously to dry");
    rack.bank.setRackBypassed(true); previous = audio.l.back(); continuous = true;
    for (int block = 0; block < 3; ++block) {
        audio.block(rack); for (float sample : audio.l) { continuous &= std::abs(sample - previous) < 0.0003f; previous = sample; }
    }
    check(continuous && near(previous, 0.1f), "rack bypass ramps continuously and independently");
    rack.bank.setRackBypassed(false); for (int i = 0; i < 3; ++i) audio.block(rack);
    check(rack.remove(a), "remove one of multiple Gains accepted");
    previous = audio.l.back(); continuous = true;
    check(waitFor([&] {
        audio.block(rack);
        for (float sample : audio.l) { continuous &= std::abs(sample - previous) < 0.0003f; previous = sample; }
        const auto s = rack.snapshot(); return s.requestedGeneration == s.committedGeneration;
    }), "remove one of multiple Gains committed");
    check(continuous && near(previous, 0.1f), "structural remove crossfade is continuous across block boundaries");
    const auto survivor = rack.snapshot().committed;
    check(survivor.instances.size() == 1 && *survivor.find(b) == *original.find(b)
          && near(rack.bank.value(slotB, 0), 48.0f / 84.0f) && rack.bank.bypassed(slotB), "remove preserves unrelated identity/level/bypass");
    auto bad = survivor; bad.instances[0].slot = slotA;
    check(!rack.submit(bad), "immutable identity mutation rejected");
    bad = survivor; bad.order.push_back(b); check(!rack.submit(bad), "duplicate order rejected transactionally");
    bad = survivor; bad.instances.push_back({newInstanceId(), slotB, "gain", 1}); bad.order.push_back(bad.instances.back().id);
    check(!rack.submit(bad), "duplicate slot rejected");
    check(rack.prepare(96000), "sample-rate restart re-prepares without changing identities");
    check(rack.snapshot().committed == survivor, "sample-rate restart preserves tone structure");
    check(audio.noHeap, "Gain, controls, swaps, bypass and retirement have no callback heap operations");
}
void orderRollbackAndStale() {
    Probe multiply, offset, failure, slow;
    auto registry = EffectRegistry::builtIns();
    registerProbe(registry, multiply, "multiply", 2, 0);
    registerProbe(registry, offset, "offset", 1, 0.25f);
    registerProbe(registry, failure, "fail", 1, 0, true);
    registerProbe(registry, slow, "slow", 1, 0);
    {
        RackEngine rack(std::move(registry)); Audio audio; rack.prepare(48000);
        auto d = definition({"multiply", "offset"});
        check(rack.submit(d) && audio.commit(rack), "noncommuting chain committed");
        audio.block(rack); check(near(audio.l.back(), 0.45f), "multiply then offset produces 0.45");
        rack.move(d.order[1], -1); check(audio.commit(rack), "noncommuting order changed");
        audio.block(rack); check(near(audio.l.back(), 0.7f), "offset then multiply produces 0.7");
        const auto good = rack.snapshot().committed;
        auto failed = good; const auto id = newInstanceId(); failed.instances.push_back({id, 2, "fail", 1}); failed.order.push_back(id);
        check(rack.submit(failed), "failing preparation requested");
        check(waitFor([&] { audio.block(rack); return rack.snapshot().error.find("Injected") != std::string::npos; }), "preparation exception surfaced outside callback");
        check(rack.snapshot().committed == good && rack.snapshot().requested == good, "failure rolls back editable state and retains committed graph");
        audio.block(rack); check(near(audio.l.back(), 0.7f), "failure retains previous audio");
        // Rollback during an already-accepted crossfade retains its destination,
        // rather than restoring the older UI acknowledgment.
        rack.move(good.order[0], 1);
        const auto accepted = rack.snapshot().requested;
        check(waitFor([&] { return rack.snapshot().preparedWaiting; }), "replacement waiting at handoff");
        audio.block(rack, 1); // start the fade but do not complete acknowledgment
        auto failDuringFade = accepted; const auto failedId = newInstanceId();
        failDuringFade.instances.push_back({failedId, 2, "fail", 1}); failDuringFade.order.push_back(failedId);
        rack.submit(failDuringFade);
        check(waitFor([&] { return rack.snapshot().error.find("Injected") != std::string::npos; }), "failure while crossfade is in progress");
        check(rack.snapshot().requested == accepted, "failure retains accepted in-flight destination");
        for (int i = 0; i < 4; ++i) audio.block(rack);
        check(waitFor([&] { return rack.snapshot().committed == accepted; }), "accepted fade commits after newer preparation fails");
        const auto retained = rack.snapshot().committed;
        slow.blocked.store(true);
        auto blocked = retained; const auto slowId = newInstanceId(); blocked.instances.push_back({slowId, 2, "slow", 1}); blocked.order.push_back(slowId);
        check(rack.submit(blocked), "slow build submitted");
        check(waitFor([&] { return slow.entered.load(); }), "worker preparing blocked graph off-thread");
        auto newer = retained; std::swap(newer.order[0], newer.order[1]);
        check(rack.submit(newer), "new request accepted while old worker is blocked");
        check(rack.add(), "new instance uses another unreserved slot during stale build");
        const auto latest = rack.snapshot().requested;
        check(latest.instances.back().slot != 2, "building graph reserves slots until off-thread destruction");
        slow.release.store(true);
        check(audio.commit(rack), "latest request commits after stale build");
        check(rack.snapshot().committed == latest && rack.snapshot().stale >= 1, "stale worker completion cannot overwrite new structure");
        check(waitFor([&] { const auto s = rack.snapshot(); return s.graphs == 1 && s.built == s.reclaimed + 1; }), "retired and rejected graphs fully reclaimed");
        check(audio.noHeap, "test-only noncommuting processors and rejected handoffs are callback heap-free");
    }
    check(multiply.made == multiply.destroyed && offset.made == offset.destroyed && failure.made == failure.destroyed && slow.made == slow.destroyed, "all constructed processors destroyed on teardown");
    check(multiply.destroyedInAudio == 0 && offset.destroyedInAudio == 0 && failure.destroyedInAudio == 0 && slow.destroyedInAudio == 0, "no DSP destruction occurs in audio");
}
void saturationAndStress() {
    RackEngine rack; Audio audio; rack.prepare(48000);
    rack.add(); check(audio.commit(rack), "stress seed committed");
    const auto first = rack.snapshot().requested.order[0]; rack.setParameter(first, 0, -6);
    for (int i = 0; i < 4; ++i) audio.block(rack);
    rack.add();
    check(waitFor([&] { return rack.snapshot().preparedWaiting; }), "single pending handoff filled without consuming callback");
    // No callback progress: all further commands must coalesce outside audio.
    for (int i = 0; i < 500; ++i) {
        const auto d = rack.snapshot().requested;
        rack.move(d.order.back(), -1); rack.move(d.order.front(), 1);
    }
    check(rack.snapshot().graphs <= 4 && rack.snapshot().coalesced > 0, "handoff saturation coalesces within strict graph bound");
    check(audio.commit(rack), "latest saturated request converges after callback resumes");
    // Simultaneous worker/control/audio traffic; only audio touches prepared DSP.
    std::atomic<bool> done{false}, heapFree{true};
    std::thread consumer([&] {
        Audio a;
        while (!done.load()) { a.block(rack, 127); std::this_thread::yield(); }
        heapFree.store(a.noHeap);
    });
    for (int i = 0; i < 1000; ++i) {
        auto d = rack.snapshot().requested;
        if (i % 5 == 0 && d.instances.size() < 10) rack.add(std::array<const char*, 3>{"overdrive", "eq", "delay"}[static_cast<unsigned>(i/5)%3]);
        else if (i % 5 == 1 && d.instances.size() > 2) rack.remove(d.order.back());
        else if (d.order.size() > 1) rack.move(d.order[0], 1);
        rack.setParameter(first, 0, static_cast<float>(i % 13 - 6));
        rack.setBypassed(first, (i % 7) == 0); rack.bank.setRackBypassed((i % 11) == 0);
    }
    done.store(true); consumer.join();
    check(audio.commit(rack), "rapid concurrent edits converge to final requested rack");
    check(waitFor([&] { return rack.snapshot().graphs == 1; }), "rapid edits reclaim all obsolete graphs");
    const auto state = rack.snapshot();
    check(state.peakGraphs <= 4 && state.built == state.reclaimed + state.graphs, "bounded ownership and graph accounting after rapid edits");
    check(state.committed.find(first) && state.committed.find(first)->slot == 0, "rapid unrelated edits preserve first identity/slot");
    check(heapFree && audio.noHeap, "concurrent structural/control edits have zero callback heap operations");
    rack.stop();
    for (int i = static_cast<int>(rack.snapshot().requested.instances.size()); i < 16; ++i) check(rack.add(), "fill fixed 16-slot capacity while stopped");
    check(waitFor([&] { auto s = rack.snapshot(); return s.committedGeneration == s.requestedGeneration; }), "stopped editing commits off-thread");
    check(!rack.add() && rack.snapshot().committed.instances.size() == 16, "17th instance rejected without replacing graph");
}
void independentCatalogInstances() {
    RackEngine rack; Audio audio; rack.prepare(48000);
    for (const auto* type : {"overdrive", "eq", "delay"}) {
        check(rack.add(type) && rack.add(type), "independent pairs of every M3 effect accepted");
        const auto d = rack.snapshot().requested;
        const auto a = d.order[d.order.size()-2], b = d.order.back();
        const auto slotA = d.find(a)->slot, slotB = d.find(b)->slot;
        const auto* descriptor = rack.registry().find(type, 1);
        const auto& control = descriptor->parameters.front();
        check(rack.setParameter(a, control.bankIndex, control.maximum), "first instance changes independently");
        check(slotA != slotB && a != b && rack.bank.value(slotA, control.bankIndex) == 1
              && near(rack.bank.value(slotB, control.bankIndex), control.normalize(control.defaultValue)), "second instance retains own default and identity");
        check(audio.commit(rack), "independent M3 pair commits");
        rack.move(b, -1); check(audio.commit(rack), "independent M3 pair reorders");
        check(*rack.snapshot().committed.find(a) == *d.find(a) && *rack.snapshot().committed.find(b) == *d.find(b), "M3 reorder preserves UUID/type/slot mappings");
        rack.remove(a); check(audio.commit(rack), "independent M3 instance removed");
        check(rack.snapshot().committed.find(b) && near(rack.bank.value(slotB, control.bankIndex), control.normalize(control.defaultValue)), "M3 removal leaves surviving controls intact");
    }
    check(audio.noHeap, "independent catalog instance transitions heap-free");
}

void statefulLifecycle() {
    Probe failure, slow; auto registry = EffectRegistry::builtIns();
    registerProbe(registry, failure, "fail", 1, 0, true);
    registerProbe(registry, slow, "slow", 1, 0);
    RackEngine rack(std::move(registry)); Audio audio; rack.prepare(48000);
    for (const auto* type : {"overdrive", "eq", "delay", "delay"}) check(rack.add(type), "stateful registry addition");
    auto d = rack.snapshot().requested;
    const auto od = d.order[0], eq = d.order[1], delay = d.order[2], other = d.order[3];
    rack.setParameter(od, 3, 0); rack.setParameter(delay, 0, 10); rack.setParameter(delay, 1, 50);
    rack.setParameter(delay, 2, 50); rack.setParameter(other, 0, 25); rack.setParameter(other, 2, 0);
    check(audio.commit(rack), "stateful graph commits");
    for (int i = 0; i < 30; ++i) audio.block(rack);
    check(near(audio.l.back(), 0.15f), "Delay feedback and mix produce expected steady level in live rack");
    const auto slot = d.find(delay)->slot, otherSlot = d.find(other)->slot;
    check(near(rack.bank.value(otherSlot, 0), 24.f/1999.f) && near(rack.bank.value(slot, 0), 9.f/1999.f), "multiple Delay instances retain independent times and slots");
    float previous = audio.l.back(); bool continuous = true;
    rack.setBypassed(delay, true);
    for (int b = 0; b < 4; ++b) { audio.block(rack); for (float v : audio.l) { continuous &= std::abs(v-previous) < 0.0003f; previous = v; } }
    check(continuous && near(previous, 0.1f), "stateful individual bypass crossfades continuously to dry");
    rack.setBypassed(delay, false);
    for (int b = 0; b < 4; ++b) audio.block(rack);
    check(near(audio.l.back(), 0.15f), "hidden Delay keeps processing while bypassed");
    previous = audio.l.back(); continuous = true;
    rack.move(eq, -1);
    check(waitFor([&] {
        audio.block(rack);
        for (float v : audio.l) { continuous &= std::abs(v-previous) < 0.001f; previous = v; }
        const auto st = rack.snapshot(); return st.committedGeneration == st.requestedGeneration;
    }), "stateful structural reorder commits while signal is held");
    for (int b = 0; b < 30; ++b) {
        audio.block(rack);
        for (float v : audio.l) { continuous &= std::abs(v-previous) < 0.001f; previous = v; }
    }
    check(continuous && near(previous, 0.15f), "empty replacement Delay history fades in without hard leading echo edges");
    // A rejected build must retain actual Delay history, not reset the active graph.
    const auto good = rack.snapshot().committed;
    auto failed = good; auto failId = newInstanceId(); failed.instances.push_back({failId, 4, "fail", 1}); failed.order.push_back(failId);
    rack.submit(failed);
    check(waitFor([&] { audio.block(rack); return !rack.snapshot().error.empty(); }), "stateful failure surfaced");
    check(rack.snapshot().committed == good && near(audio.l.back(), 0.15f), "failure rollback preserves stateful active audio");
    slow.blocked = true;
    auto blocked = good; auto slowId = newInstanceId(); blocked.instances.push_back({slowId, 4, "slow", 1}); blocked.order.push_back(slowId);
    rack.submit(blocked); check(waitFor([&] { return slow.entered.load(); }), "stateful stale build blocked off audio");
    auto newest = good; std::swap(newest.order[2], newest.order[3]);
    rack.submit(newest); rack.setParameter(delay, 1, 25); slow.release = true;
    check(audio.commit(rack), "stateful latest request commits after stale result");
    for (int b = 0; b < 30; ++b) audio.block(rack);
    check(near(audio.l.back(), 0.1f * (0.5f + 0.5f/0.75f)) && near(rack.bank.value(slot, 1), 25.f/85.f), "stale Delay graph does not overwrite current parameters");
    check(rack.snapshot().committed.instances == good.instances && rack.snapshot().stale > 0, "stateful reorder preserves identities and rejects stale build");
    // Capture a nonzero tail, then remove the Delay while silence enters the rack.
    previous = audio.l.back(); continuous = true; rack.remove(delay);
    check(waitFor([&] {
        audio.block(rack, 127, 0);
        for (int i = 0; i < 127; ++i) { continuous &= std::isfinite(audio.l[i]) && std::abs(audio.l[i]) < 0.2f; }
        auto state = rack.snapshot(); return state.requestedGeneration == state.committedGeneration;
    }), "stateful removal commits during tail");
    check(continuous && audio.l[126] == 0 && rack.snapshot().committed.find(other), "removed tail fades away and independent Delay survives");
    check(rack.prepare(96000), "stateful rack reprepare at new sample rate");
    audio.block(rack, 512, 0); check(audio.l.back() == 0, "Stop/sample-rate restart clears temporal history");
    check(audio.noHeap, "stateful live activation, bypass, stale rejection and removal do not allocate/deallocate");

    // The real production processors must change sound when their order changes.
    rack.setParameter(od, 0, 36); rack.setParameter(od, 2, -6); rack.setParameter(od, 3, 100);
    rack.setParameter(eq, 1, 12);
    auto energy = [&] {
        double result = 0;
        for (int b = 0; b < 40; ++b) {
            for (int i = 0; i < 512; ++i) audio.l[i] = audio.r[i] = 0.1f*std::sin(static_cast<float>((b*512+i)*2*3.141592653589793*1000/96000));
            heapAudit::begin(); rack.beginBlock(); rack.process(audio.l.data(), audio.r.data(), 512); audio.noHeap &= heapAudit::finish();
            if (b > 20) for (float v : audio.l) result += v*v;
        }
        return result;
    };
    rack.move(od, -1); check(audio.commit(rack), "Overdrive first for order signal test");
    const double before = energy(); rack.move(eq, -1); check(audio.commit(rack), "real Overdrive/EQ order change commits");
    const double after = energy();
    check(before > after*4 && audio.noHeap, "production Overdrive then EQ differs from EQ then Overdrive");
    check(waitFor([&] { auto st = rack.snapshot(); return st.graphs == 1 && st.built == st.reclaimed+1; }), "stateful rings are reclaimed off audio within four-graph bound");
}

void engineIntegration() {
    RackEngine rack; MonitorControls controls; controls.setOutputDb(0); MonitorEngine engine(controls, &rack);
    check(engine.prepare(48000), "shared engine prepares rack"); rack.add();
    std::array<float, 8193> in{}, l{}, r{}; in.fill(0.1f);
    check(waitFor([&] { engine.process(in.data(), in.data(), l.data(), r.data(), 8193); auto s = rack.snapshot(); return s.committedGeneration == s.requestedGeneration; }), "oversized blocks activate rack safely");
    const auto id = rack.snapshot().committed.order[0]; rack.setParameter(id, 0, 24);
    heapAudit::begin(); engine.process(in.data(), in.data(), l.data(), r.data(), 0); engine.process(in.data(), in.data(), l.data(), r.data(), 8193);
    check(heapAudit::finish(), "full monitoring/rack path chunking uses no callback heap operations");
    check(l.back() == 0.98f && l == r && (engine.consumeMeters().flags & MonitorEngine::outputProtected), "rack precedes final output protection and meters");
    rack.bank.setRackBypassed(true); engine.process(in.data(), in.data(), l.data(), r.data(), 8193);
    check(near(l.back(), 0.1f), "rack bypass retains trim/output routing");
    controls.setMuted(true); engine.process(in.data(), in.data(), l.data(), r.data(), 8193);
    check(l.back() == 0 && engine.consumeMeters().inputPeak > 0, "Mute independent of rack bypass and input meter");
    check(ParameterBank::parameterId(3, 0) == "slot_03_param_00" && ParameterBank::parameterName(3, 0) == "Slot 03 Control 01", "future host ID/name mapping is stable");
    // Nonfinite effect output is silenced and diagnosed after DSP, not propagated.
    Probe invalid; auto registry = EffectRegistry::builtIns(); registerProbe(registry, invalid, "invalid", 1, 0, false, true);
    RackEngine bad(std::move(registry)); MonitorControls c; c.setOutputDb(0); MonitorEngine e(c, &bad); e.prepare(48000);
    bad.submit(definition({"invalid"}));
    check(waitFor([&] { e.process(in.data(), in.data(), l.data(), r.data(), 8193); const auto s = bad.snapshot(); return s.committedGeneration == s.requestedGeneration; }), "invalid-output test graph committed");
    e.process(in.data(), in.data(), l.data(), r.data(), 8193);
    check(l.back() == 0 && r.back() == 0 && (e.consumeMeters().flags & MonitorEngine::nonFiniteDSP), "nonfinite DSP output silenced with off-thread diagnostic");
    bad.bank.setRackBypassed(true); e.process(in.data(), in.data(), l.data(), r.data(), 8193);
    check(near(l.back(), 0.1f), "fully bypassed rack returns exact dry even after invalid DSP");
    bad.bank.setRackBypassed(false); bad.setBypassed(bad.snapshot().committed.order[0], true);
    e.process(in.data(), in.data(), l.data(), r.data(), 8193);
    check(near(l.back(), 0.1f), "fully bypassed invalid effect returns exact dry");
}
}
int main() {
    identityAndGain(); orderRollbackAndStale(); saturationAndStress(); independentCatalogInstances(); statefulLifecycle(); engineIntegration();
    if (failures) { std::cerr << failures << " failed checks\n"; return 1; }
    std::cout << "PASS: independent Gains, identities, noncommuting order, smooth bypass, stale/failed requests, saturation, rapid edits, reclamation, RT audit, engine protection\n";
}
