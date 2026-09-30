#include "core/Effects.h"
#include "HeapAudit.h"
#include <algorithm>
#include <array>
#include <cmath>
#include <iostream>
#include <limits>
#include <numbers>

namespace {
using namespace guitarrack;
int failures = 0;
void check(bool condition, const char* message) {
    if (!condition) { ++failures; std::cerr << "FAIL: " << message << '\n'; }
}
struct Effect {
    EffectRegistry registry = EffectRegistry::builtIns();
    ParameterBank bank;
    const EffectDescriptor* descriptor;
    std::unique_ptr<EffectProcessor> processor;
    std::array<float, 512> l{}, r{};
    bool heapFree = true;
    Effect(const char* type, double rate = 48000) : descriptor(registry.find(type, 1)) {
        for (const auto& p : descriptor->parameters) set(p.bankIndex, p.defaultValue);
        processor = descriptor->factory(bank, 0);
        check(processor->prepare(rate, 512), "effect prepared"); processor->activate(nullptr);
    }
    void set(unsigned index, float value) {
        for (const auto& p : descriptor->parameters) if (p.bankIndex == index) check(bank.setValue(0, index, p.normalize(value)), "valid parameter mapping");
    }
    void block(int n = 512) {
        heapAudit::begin(); processor->process(l.data(), r.data(), n); heapFree &= heapAudit::finish();
    }
    void constant(float value, int n = 512) { l.fill(value); r.fill(value); block(n); }
    void reset() { processor->reset(); }
};
void overdrive() {
    Effect e("overdrive");
    e.set(3, 0); e.reset();
    e.l.fill(0.123f); e.r.fill(-0.2f); e.block();
    check(e.l[0] == 0.123f && e.r[0] == -0.2f, "Overdrive zero mix returns exact dry independently on both channels");
    e.set(0, 0); e.set(2, 0); e.set(3, 100); e.reset();
    for (int i = 0; i < 8; ++i) e.constant(0.001f);
    check(std::abs(e.l.back() - 0.001f) < 1e-7, "minimum drive is approximately neutral for small signals");
    e.set(0, 36); e.set(2, -6); e.reset();
    for (int i = 0; i < 8; ++i) e.constant(10);
    const float reference = e.l.back();
    check(std::abs(reference - std::pow(10.f, -6.f/20)) < 1e-6, "soft clip saturates at wet level without unbounded drive gain");
    e.set(2, -12); for (int i = 0; i < 8; ++i) e.constant(10);
    check(std::abs(e.l.back()/reference - std::pow(10.f, -6.f/20)) < 1e-5, "wet Level has measured dB handling");
    e.set(3, 50); for (int i = 0; i < 8; ++i) e.constant(0.2f);
    const float half = e.l.back();
    e.set(3, 100); for (int i = 0; i < 8; ++i) e.constant(0.2f);
    check(std::abs(half - (0.2f + e.l.back())/2) < 1e-6, "Overdrive Mix is linear dry/wet and Level only scales wet");
    e.set(3, 100); e.set(2, 6); e.reset();
    for (int block = 0; block < 20; ++block) {
        e.l.fill(std::numeric_limits<float>::max()); e.r.fill(-std::numeric_limits<float>::max()); e.block();
        for (int i = 0; i < 512; ++i) check(std::isfinite(e.l[i]) && std::abs(e.l[i]) <= 2 && std::abs(e.l[i]+e.r[i]) < 1e-6, "extreme wet input finite, bounded and symmetric");
    }
    e.set(0, 0); e.set(1, 200); e.set(2, -24); e.set(3, 0);
    float previous = e.l.back(); bool smooth = true;
    // Use a steady input with settled state before checking audible parameter ramps.
    e.reset(); for (int i = 0; i < 8; ++i) e.constant(0.1f); previous = e.l.back();
    e.set(0, 36); e.set(1, 8000); e.set(2, 6); e.set(3, 100);
    for (int b = 0; b < 8; ++b) { e.constant(0.1f); for (float v : e.l) { smooth &= std::abs(v-previous) < 0.02f; previous = v; } }
    check(smooth && e.heapFree, "Overdrive parameter ramps continuous and processing heap-free");
}
double response(Effect& e, double rate, double frequency) {
    double inputEnergy = 0, outputEnergy = 0;
    const int samples = static_cast<int>(rate * 0.5);
    e.reset();
    for (int offset = 0; offset < samples; offset += 127) {
        const int n = std::min(127, samples-offset);
        for (int i = 0; i < n; ++i) e.l[i] = e.r[i] = static_cast<float>(0.05 * std::sin(2*std::numbers::pi*frequency*(offset+i)/rate));
        const auto input = e.l; e.block(n);
        if (offset > samples/2) for (int i = 0; i < n; ++i) { inputEnergy += input[i]*input[i]; outputEnergy += e.l[i]*e.l[i]; }
    }
    return 10*std::log10(outputEnergy/inputEnergy);
}
void eq() {
    for (double rate : {8000., 44100., 48000., 96000., 192000.}) {
        Effect e("eq", rate);
        e.l.fill(0.123f); e.r.fill(-0.2f); e.block();
        check(e.l[0] == 0.123f && e.r.back() == -0.2f, "EQ neutral is exact wire on both channels");
        const double high = std::min(4000., rate*0.45);
        for (unsigned band = 0; band < 3; ++band) {
            const double frequency = band == 0 ? 150 : band == 1 ? 1000 : high;
            for (float db : {-12.f, 6.f, 12.f}) {
                for (unsigned j = 0; j < 3; ++j) e.set(j, 0);
                e.set(band, db);
                const double measured = response(e, rate, frequency), expected = band == 1 ? db : db/2;
                check(std::abs(measured-expected) < 0.025, "measured EQ shelf midpoint/peaking center matches requested dB");
            }
        }
        e.set(0, 6); e.set(1, 0); e.set(2, 0);
        check(std::abs(response(e, rate, 25)-6) < 0.03 && std::abs(response(e, rate, 2000)) < 0.03, "low shelf boosts bass and retains high frequencies");
        if (rate >= 44100) {
            e.set(0, 0); e.set(2, -6);
            check(std::abs(response(e, rate, 16000)+6) < 0.04 && std::abs(response(e, rate, 150)) < 0.03, "high shelf cuts treble and retains bass");
        }
        e.set(0, 0); e.set(1, 0); e.set(2, 0); e.reset();
        float previous = 0; bool bounded = true;
        for (int b = 0; b < 120; ++b) {
            for (unsigned j = 0; j < 3; ++j) e.set(j, b%2 ? -12 : 12);
            for (int i = 0; i < 127; ++i) e.l[i] = e.r[i] = static_cast<float>(0.01*std::sin(2*std::numbers::pi*80*(b*127+i)/rate));
            e.block(127);
            for (int i = 0; i < 127; ++i) { bounded &= std::isfinite(e.l[i]) && std::abs(e.l[i]) < 0.2f && std::abs(e.l[i]-previous) < 0.02f; previous = e.l[i]; }
        }
        check(bounded && e.heapFree, "rapid EQ coefficient changes stay stable, continuous and heap-free across sample rates");
    }
}
void delay() {
    for (double rate : {8000., 44100., 48000., 96000., 192000.}) {
        Effect e("delay", rate); e.set(0, 10.5f); e.set(1, 50); e.set(2, 100); e.reset();
        double firstEnergy = 0, secondEnergy = 0, thirdEnergy = 0, firstMoment = 0;
        const double position = rate * 0.0105;
        bool quietBefore = true, stereoIndependent = true;
        for (int offset = 0; offset < static_cast<int>(4*position+10); offset += 127) {
            const int n = std::min(127, static_cast<int>(4*position+10)-offset);
            e.l.fill(0); e.r.fill(0); if (offset == 0) e.l[0] = 1;
            e.block(n);
            for (int i = 0; i < n; ++i) {
                const int sample = offset+i; const double v = e.l[i];
                if (sample < std::floor(position)-1) quietBefore &= std::abs(v) < 1e-7;
                if (std::abs(sample-position) <= 2) { firstEnergy += v; firstMoment += sample*v; }
                if (std::abs(sample-2*position) <= 3) secondEnergy += v;
                if (std::abs(sample-3*position) <= 4) thirdEnergy += v;
                stereoIndependent &= e.r[i] == 0;
            }
        }
        check(quietBefore && std::abs(firstEnergy-1) < 0.0001 && std::abs(firstMoment/firstEnergy-position) < 0.002, "delay interpolated impulse timing in milliseconds at actual rate");
        check(std::abs(secondEnergy-0.5) < 0.0001 && std::abs(thirdEnergy-0.25) < 0.0001 && stereoIndependent, "feedback repeats decay geometrically and stereo history is independent");
        e.reset(); e.constant(0); check(e.l.back() == 0, "Delay reset clears history off audio");
        e.set(2, 0); e.reset(); e.constant(0.123f); check(e.l.back() == 0.123f, "Delay zero mix exact dry");
        e.set(0, 1); e.set(1, 85); e.set(2, 100); e.reset();
        bool finite = true;
        for (int b = 0; b < static_cast<int>(rate/512)+1; ++b) {
            e.constant(0.1f); for (float v : e.l) finite &= std::isfinite(v) && std::abs(v) < 0.668f;
        }
        check(finite && std::abs(e.l.back() - 0.1f/0.15f) < 0.001, "maximum feedback converges below instability");
        e.set(0, 10); e.set(1, 0); e.reset();
        float previous = 0; bool continuous = true;
        for (int b = 0; b < static_cast<int>(rate/127); ++b) {
            if (b%19 == 0) e.set(0, b%38 == 0 ? 1 : 2000);
            for (int i = 0; i < 127; ++i) e.l[i] = e.r[i] = static_cast<float>(0.1*std::sin(2*std::numbers::pi*40*(b*127+i)/rate));
            e.block(127);
            for (int i = 0; i < 127; ++i) { continuous &= std::isfinite(e.l[i]) && std::abs(e.l[i]-previous) < 0.005f; previous = e.l[i]; }
        }
        check(continuous && e.heapFree, "rapid Delay time edits crossfade without abrupt jumps or heap operations");
        // Stop/reprepare must discard old-rate temporal resources and use new timing.
        e.set(0, 10); e.set(1, 0); e.set(2, 100);
        check(e.processor->prepare(48000, 512), "Delay reprepare after sample-rate change");
        e.l.fill(0); e.r.fill(0); e.l[0] = 1; e.block();
        check(std::abs(e.l[480]-1) < 0.0001 && e.l[0] == 0, "sample-rate reprepare resets and recalculates impulse position");
        // Maximum time must survive ring wrap, including non-power-of-two rates.
        e.set(0, 2000); e.reset(); bool onTime = true; double energy = 0;
        for (int offset = 0; offset < 96500; offset += 512) {
            e.l.fill(0); e.r.fill(0); if (offset == 0) e.l[0] = 1;
            e.block();
            for (int i = 0; i < 512; ++i) { if (offset+i != 96000) onTime &= std::abs(e.l[i]) < 1e-7; else energy += e.l[i]; }
        }
        check(onTime && std::abs(energy-1) < 0.0001, "maximum delay fits prepared storage with correct ring wrap");
    }
}
void partitionAndActivation() {
    for (const auto* type : {"overdrive", "eq", "delay"}) {
        Effect a(type), b(type);
        if (std::string(type) == "eq") { a.set(1, 6); b.set(1, 6); }
        if (std::string(type) == "delay") { a.set(0, 5); b.set(0, 5); }
        a.reset(); b.reset(); bool equal = true;
        for (int offset = 0; offset < 4096; offset += 512) {
            for (int i = 0; i < 512; ++i) a.l[i] = a.r[i] = static_cast<float>(0.1*std::sin((offset+i)*0.04));
            b.l = a.l; b.r = a.r; a.block();
            for (int i = 0; i < 512; ++i) { heapAudit::begin(); b.processor->process(b.l.data()+i, b.r.data()+i, 1); b.heapFree &= heapAudit::finish(); }
            for (int i = 0; i < 512; ++i) equal &= a.l[i] == b.l[i];
        }
        heapAudit::begin(); a.processor->process(a.l.data(), a.r.data(), 0); check(heapAudit::finish(), "zero block heap-free");
        auto replacement = a.descriptor->factory(a.bank, 0); replacement->prepare(48000, 512);
        heapAudit::begin(); replacement->activate(a.processor.get()); replacement->process(a.l.data(), a.r.data(), 512);
        check(heapAudit::finish() && equal && a.heapFree && b.heapFree, "effect variable block partition and bounded activation audit");
    }
}
}
int main() {
    overdrive(); eq(); delay(); partitionAndActivation();
    if (failures) { std::cerr << failures << " failed checks\n"; return 1; }
    std::cout << "PASS: Overdrive dry/mix/level/bounds, measured EQ responses/stability, Delay impulses/decay/time edits/reset/rates, variable blocks, activation heap audit\n";
}
