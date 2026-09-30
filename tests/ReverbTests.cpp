#include "dsp/Reverb.h"
#include "HeapAudit.h"
#include <algorithm>
#include <array>
#include <cmath>
#include <iostream>
#include <limits>

namespace {
using namespace guitarrack;
int failures = 0;
void check(bool condition, const char* message) {
    if (!condition) { ++failures; std::cerr << "FAIL: " << message << '\n'; }
}
struct Reverb {
    ParameterBank bank;
    ReverbProcessor processor;
    std::array<float, 512> left{}, right{};
    bool heapFree = true;
    Reverb(double rate = 48000, SlotId slot = 0) : processor(bank, slot) {
        bank.setValue(slot, 0, 0.35f); bank.setValue(slot, 1, 0.55f); bank.setValue(slot, 2, 0.15f);
        check(processor.prepare(rate, 512), "Reverb preparation succeeds");
        heapAudit::begin(); processor.activate(nullptr); heapFree &= heapAudit::finish();
    }
    void set(unsigned index, float value) { check(bank.setValue(0, index, value/100), "validated control"); }
    void block(int count = 512) {
        heapAudit::begin(); processor.process(left.data(), right.data(), count); heapFree &= heapAudit::finish();
    }
    void silence(int count = 512) { left.fill(0); right.fill(0); block(count); }
};
struct Response { double early = 0, late = 0, veryLate = 0, energy = 0, differenceEnergy = 0; bool independent = true, finite = true; };
Response impulse(float size, float damping, double rate = 48000) {
    Reverb reverb(rate); reverb.set(0, size); reverb.set(1, damping); reverb.set(2, 100); reverb.processor.reset();
    Response response; float previous = 0;
    const int count = static_cast<int>(rate*4);
    for (int offset = 0; offset < count; offset += 127) {
        const int n = std::min(127, count-offset);
        reverb.left.fill(0); reverb.right.fill(0); if (offset == 0) reverb.left[0] = 1;
        reverb.block(n);
        for (int i = 0; i < n; ++i) {
            const double time = (offset+i)/rate, value = reverb.left[i], energy = value*value;
            if (time < 0.5) response.early += energy;
            if (time >= 2 && time < 3) response.late += energy;
            if (time >= 3) response.veryLate += energy;
            if (time >= 0.1) {
                response.energy += energy;
                response.differenceEnergy += (value-previous)*(value-previous);
            }
            previous = static_cast<float>(value);
            response.independent &= reverb.right[i] == 0;
            response.finite &= std::isfinite(value) && std::abs(value) < 1;
        }
    }
    check(reverb.heapFree, "Reverb first callback and complete impulse response have zero heap operations");
    return response;
}
void impulses() {
    const auto small = impulse(0, 0), large = impulse(100, 0), dark = impulse(100, 100);
    check(small.early > 1e-4 && large.early > 1e-4, "Reverb impulse produces real nonzero wet decay");
    check(large.late > 100*small.late && large.late > 1e-10, "larger Size measurably extends decay");
    check(large.veryLate < large.late && large.late < large.early*0.01, "maximum Size impulse energy decays strongly over time");
    check(dark.differenceEnergy/dark.energy < large.differenceEnergy/large.energy*0.5, "Damping attenuates high-frequency tail energy");
    check(small.independent && large.independent && dark.independent && small.finite && large.finite && dark.finite,
          "independent stereo impulse has no cross-channel collapse and bounded finite output");
    for (double rate : {8000., 44100., 96000., 192000., 768000.}) {
        Reverb reverb(rate); reverb.set(2, 100); reverb.processor.reset();
        bool quietBefore = true; double energy = 0;
        const int end = static_cast<int>(rate*0.07);
        const int first = static_cast<int>(std::round(1091*rate/44100));
        for (int offset = 0; offset < end; offset += 512) {
            const int n = std::min(512, end-offset);
            reverb.left.fill(0); reverb.right.fill(0); if (offset == 0) reverb.left[0] = 1;
            reverb.block(n);
            for (int i = 0; i < n; ++i) {
                if (offset+i < first) quietBefore &= reverb.left[i] == 0;
                energy += reverb.left[i]*reverb.left[i];
            }
        }
        check(quietBefore && energy > 1e-5 && reverb.heapFree, "musical delay timings scale across supported rates without added DSP latency or callback allocation");
        check(reverb.processor.latencySamples() == 0 && reverb.processor.tailSeconds() == 10, "Reverb reports zero processing latency and conservative tail bound");
    }
}
void stereoMixReset() {
    Reverb wet, half, dry;
    wet.set(2, 100); half.set(2, 50); dry.set(2, 0);
    wet.processor.reset(); half.processor.reset(); dry.processor.reset();
    bool separation = false, linearMix = true, exactDry = true;
    for (int offset = 0; offset < 48000; offset += 512) {
        for (int i = 0; i < 512; ++i) {
            const float input = offset == 0 && i == 0 ? 1.f : 0.f;
            wet.left[i] = wet.right[i] = half.left[i] = half.right[i] = dry.left[i] = dry.right[i] = input;
        }
        wet.block(); half.block(); dry.block();
        for (int i = 0; i < 512; ++i) {
            separation |= wet.left[i] != wet.right[i];
            const float input = offset == 0 && i == 0 ? 1.f : 0.f;
            linearMix &= std::abs(half.left[i] - 0.5f*(input+wet.left[i])) < 1e-7
                      && std::abs(half.right[i] - 0.5f*(input+wet.right[i])) < 1e-7;
            exactDry &= dry.left[i] == input && dry.right[i] == input;
        }
    }
    check(separation && linearMix && exactDry, "mono input widens to stereo; 50% Mix is linear, zero Mix exact dry");
    wet.processor.reset(); bool clear = true;
    for (int b = 0; b < 20; ++b) { wet.silence(); for (int i = 0; i < 512; ++i) clear &= wet.left[i] == 0 && wet.right[i] == 0; }
    check(clear, "off-audio reset clears all reverb history reliably");
    Reverb independent(48000, 1); independent.bank.setValue(1, 2, 1); independent.processor.reset();
    independent.silence();
    check(independent.left.back() == 0 && independent.right.back() == 0, "second instance retains independent prepared state and slot controls");
    dry.left.fill(0.123f); dry.right.fill(-0.2f); dry.block();
    check(dry.left.front() == 0.123f && dry.right.back() == -0.2f, "independent stereo dry samples stay distinct");
    dry.left.fill(1.0e-30f); dry.right.fill(-1.0e-30f); dry.block();
    check(dry.left.front() == 1.0e-30f && dry.right.back() == -1.0e-30f, "zero Mix preserves even tiny finite dry samples");
    check(wet.heapFree && half.heapFree && dry.heapFree && independent.heapFree, "stereo and Mix processing heap-free");
}
void partitionActivationRate() {
    Reverb full, split;
    full.set(2, 100); split.set(2, 100); full.processor.reset(); split.processor.reset();
    bool equal = true;
    for (int offset = 0; offset < 48000; offset += 512) {
        for (int i = 0; i < 512; ++i) {
            full.left[i] = static_cast<float>(0.1*std::sin((offset+i)*0.037));
            full.right[i] = static_cast<float>(0.05*std::cos((offset+i)*0.051));
        }
        split.left = full.left; split.right = full.right; full.block();
        for (int i = 0; i < 512; ++i) {
            heapAudit::begin(); split.processor.process(split.left.data()+i, split.right.data()+i, 1); split.heapFree &= heapAudit::finish();
        }
        for (int i = 0; i < 512; ++i) equal &= full.left[i] == split.left[i] && full.right[i] == split.right[i];
        full.block(0); split.block(0);
    }
    std::array<float, 2049> bigLeft{}, bigRight{}, smallLeft{}, smallRight{};
    for (unsigned i = 0; i < bigLeft.size(); ++i) { bigLeft[i] = smallLeft[i] = static_cast<float>(0.1*std::sin(i*0.02)); bigRight[i] = smallRight[i] = -bigLeft[i]; }
    heapAudit::begin(); full.processor.process(bigLeft.data(), bigRight.data(), static_cast<int>(bigLeft.size())); full.heapFree &= heapAudit::finish();
    for (unsigned offset = 0; offset < bigLeft.size(); offset += 127) {
        const int n = static_cast<int>(std::min<std::size_t>(127, bigLeft.size()-offset));
        heapAudit::begin(); split.processor.process(smallLeft.data()+offset, smallRight.data()+offset, n); split.heapFree &= heapAudit::finish();
    }
    check(equal && bigLeft == smallLeft && bigRight == smallRight && full.heapFree && split.heapFree,
          "one/zero/variable/oversized blocks preserve identical sample sequence and heap-free callbacks");
    auto replacement = std::make_unique<ReverbProcessor>(full.bank, 0); replacement->prepare(48000, 512);
    full.left.fill(0); full.right.fill(0);
    heapAudit::begin(); replacement->activate(&full.processor); replacement->process(full.left.data(), full.right.data(), 512);
    check(heapAudit::finish() && full.left.back() == 0 && full.right.back() == 0,
          "structural activation copies only controls, has empty tail and zero allocations/deallocations");
    Reverb fresh(44100); fresh.set(2, 100); fresh.processor.reset();
    check(full.processor.prepare(44100, 512), "sample-rate restart prepares new reverb storage off audio");
    bool rateEquivalent = true;
    for (int block = 0; block < 12; ++block) {
        full.left.fill(0); full.right.fill(0); fresh.left.fill(0); fresh.right.fill(0);
        if (block == 0) full.left[0] = fresh.left[0] = 1;
        full.block(); fresh.block();
        rateEquivalent &= full.left == fresh.left && full.right == fresh.right;
    }
    check(rateEquivalent, "sample-rate restart discards history and matches fresh processor through nonzero response");
    check(!full.processor.prepare(0, 512) && !full.processor.prepare(768001, 512) && !full.processor.prepare(48000, 0), "invalid prepare resources rejected");
}
void boundedLongRun() {
    Reverb reverb(8000); reverb.set(0, 100); reverb.set(1, 100); reverb.set(2, 100); reverb.processor.reset();
    bool bounded = true; float peak = 0;
    for (int b = 0; b < 800; ++b) {
        if (b < 400 && b%17 == 0) { reverb.set(0, b%34 ? 100 : 0); reverb.set(1, b%34 ? 0 : 100); reverb.set(2, b%34 ? 100 : 15); }
        for (int i = 0; i < 512; ++i) {
            reverb.left[i] = b < 400 ? static_cast<float>(0.1*std::sin((b*512+i)*0.053)) : 0;
            reverb.right[i] = b < 400 ? static_cast<float>(0.1*std::cos((b*512+i)*0.031)) : 0;
        }
        reverb.block();
        for (int i = 0; i < 512; ++i) {
            bounded &= std::isfinite(reverb.left[i]) && std::isfinite(reverb.right[i]) && std::abs(reverb.left[i]) < 1 && std::abs(reverb.right[i]) < 1;
            if (b > 600) peak = std::max({peak, std::abs(reverb.left[i]), std::abs(reverb.right[i])});
        }
    }
    check(bounded && peak < 1e-10 && reverb.heapFree, "51-second render with rapid controls stays bounded and tail decays below conservative declaration");
    reverb.left.fill(std::numeric_limits<float>::max()); reverb.right.fill(std::numeric_limits<float>::quiet_NaN()); reverb.block();
    bool finite = true;
    for (int i = 0; i < 512; ++i) finite &= std::isfinite(reverb.left[i]) && std::isfinite(reverb.right[i]);
    for (int b = 0; b < 100; ++b) { reverb.silence(); for (int i = 0; i < 512; ++i) finite &= std::isfinite(reverb.left[i]) && std::isfinite(reverb.right[i]); }
    check(finite && reverb.heapFree, "non-finite/extreme input never poisons reverb state or allocates");
}
void smoothControls() {
    Reverb reverb; reverb.set(0, 0); reverb.set(1, 0); reverb.set(2, 0); reverb.processor.reset();
    for (int block = 0; block < 200; ++block) { reverb.left.fill(0.1f); reverb.right.fill(-0.1f); reverb.block(); }
    float previous = reverb.left.back(); bool smooth = true;
    for (int block = 0; block < 100; ++block) {
        if (block%5 == 0) { reverb.set(0, block%10 ? 0 : 100); reverb.set(1, block%10 ? 0 : 100); reverb.set(2, block%10 ? 0 : 100); }
        reverb.left.fill(0.1f); reverb.right.fill(-0.1f); reverb.block();
        for (float value : reverb.left) { smooth &= std::isfinite(value) && std::abs(value-previous) < 0.003f; previous = value; }
    }
    check(smooth && reverb.heapFree, "simultaneous rapid Size/Damping/Mix changes stay continuous on settled constant input");
}
}
int main() {
    const auto descriptor = reverbDescriptor();
    check(descriptor.typeId == "reverb" && descriptor.version == 1 && descriptor.parameters.size() == 3,
          "distinct versioned Reverb descriptor with fixed Size/Damping/Mix bank");
    check(descriptor.parameters[0].bankIndex == 0 && descriptor.parameters[0].defaultValue == 35
       && descriptor.parameters[1].bankIndex == 1 && descriptor.parameters[1].defaultValue == 55
       && descriptor.parameters[2].bankIndex == 2 && descriptor.parameters[2].defaultValue == 15,
          "stable conservative Reverb defaults and indices");
    impulses(); stereoMixReset(); partitionActivationRate(); boundedLongRun(); smoothControls();
    if (failures) { std::cerr << failures << " failed checks\n"; return 1; }
    std::cout << "PASS: Reverb impulse/decay/damping/stereo/mix/reset/rates/independent instances, bounded long render, variable blocks and activation/first-callback heap audit\n";
}
