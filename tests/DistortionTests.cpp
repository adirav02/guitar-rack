#include "dsp/Distortion.h"
#include "dsp/Overdrive.h"
#include "HeapAudit.h"
#include <algorithm>
#include <array>
#include <cmath>
#include <iostream>
#include <limits>
#include <numbers>
#include <vector>

namespace {
using namespace guitarrack;
int failures = 0;
void check(bool condition, const char* message) {
    if (!condition) { ++failures; std::cerr << "FAIL: " << message << '\n'; }
}
struct Effect {
    ParameterBank bank;
    EffectDescriptor descriptor = distortionDescriptor();
    DistortionProcessor processor{bank, 0};
    bool heapFree = true;
    explicit Effect(double rate = 48000) {
        for (const auto& p : descriptor.parameters) set(p.bankIndex, p.defaultValue);
        check(processor.prepare(rate, 512), "Distortion prepares bounded resources");
        heapAudit::begin(); processor.activate(nullptr); heapFree &= heapAudit::finish();
    }
    void set(unsigned index, float value) { check(bank.setValue(0, index, descriptor.parameters[index].normalize(value)), "valid stable parameter mapping"); }
    void process(float* l, float* r, int n) {
        heapAudit::begin(); processor.process(l, r, n); heapFree &= heapAudit::finish();
    }
};
std::vector<float> render(Effect& e, double frequency, double rate = 48000, float amplitude = 0.2f, int samples = 8192) {
    std::vector<float> result(static_cast<std::size_t>(samples));
    std::array<float, 512> left{}, right{};
    e.processor.reset();
    for (int offset = -8192; offset < samples; offset += 512) {
        const int n = std::min(512, samples - offset);
        for (int i = 0; i < n; ++i) left[i] = right[i] = static_cast<float>(amplitude * std::sin(2 * std::numbers::pi * frequency * (offset + i) / rate));
        e.process(left.data(), right.data(), n);
        if (offset >= 0) std::copy_n(left.begin(), n, result.begin() + offset);
    }
    return result;
}
double energy(const std::vector<float>& data, double frequency, double rate = 48000) {
    double real = 0, imag = 0;
    for (unsigned i = 0; i < data.size(); ++i) {
        const double angle = 2 * std::numbers::pi * frequency * i / rate;
        real += data[i] * std::cos(angle); imag += data[i] * std::sin(angle);
    }
    return 4 * (real * real + imag * imag) / (data.size() * data.size());
}
void controlsAndTransfer() {
    Effect dry, wet, mixed, quiet;
    for (auto* e : {&dry, &wet, &mixed, &quiet}) { e->set(0, 24); e->set(1, 8000); e->set(2, 0); }
    dry.set(3, 0); wet.set(3, 100); mixed.set(3, 50); quiet.set(3, 100); quiet.set(2, -6);
    const auto a = render(dry, 750), b = render(wet, 750), c = render(mixed, 750), d = render(quiet, 750);
    bool linearMix = true, level = true;
    for (unsigned i = 0; i < a.size(); ++i) {
        linearMix &= std::abs(c[i] - (a[i] + b[i]) * 0.5f) < 2e-7f;
        level &= std::abs(d[i] - b[i] * std::pow(10.f, -6.f / 20)) < 5e-7f;
    }
    check(linearMix && level, "Mix is aligned linear dry/wet; Level scales only wet in dB");
    const double fundamental = energy(b, 750), third = energy(b, 2250), fifth = energy(b, 3750);
    check(fundamental > 0.9 && third / fundamental > 0.04 && fifth / fundamental > 0.002,
          "strong hard clipping produces bounded fundamental and odd harmonic content");
    ParameterBank bank;
    const auto overdrive = overdriveDescriptor();
    const std::array<float, 4> settings{24, 8000, 0, 100};
    for (unsigned i = 0; i < settings.size(); ++i) bank.setValue(0, i, overdrive.parameters[i].normalize(settings[i]));
    OverdriveProcessor soft{bank, 0}; soft.prepare(48000, 512);
    std::array<float, 512> left{}, right{};
    std::vector<float> reference(8192);
    for (int offset = -8192; offset < 8192; offset += 512) {
        for (int i = 0; i < 512; ++i) left[i] = right[i] = static_cast<float>(0.2 * std::sin(2 * std::numbers::pi * 750 * (offset + i) / 48000));
        soft.process(left.data(), right.data(), 512);
        if (offset >= 0) std::copy(left.begin(), left.end(), reference.begin() + offset);
    }
    const double softFifth = energy(reference, 3750) / energy(reference, 750);
    check(fifth / fundamental > softFifth * 1.10, "hard clipper has stronger fifth-harmonic ratio than matched tanh Overdrive");
    wet.set(0, 0); const auto small = render(wet, 750, 48000, 0.001f);
    wet.set(0, 12); const auto driven = render(wet, 750, 48000, 0.001f);
    check(std::abs(std::sqrt(energy(driven, 750) / energy(small, 750)) - std::pow(10., 12. / 20)) < 1e-4,
          "unclipped Drive provides documented pre-gain");
    wet.set(0, 48); const auto saturated = render(wet, 750, 48000, 10);
    check(*std::max_element(saturated.begin(), saturated.end()) < 1.1f,
          "hard clipping and filters bound wet gain independently of drive/input");
    check(dry.heapFree && wet.heapFree && mixed.heapFree && quiet.heapFree, "initial and steady callback processing has no C++ heap activity");
}
void latencyResetAndPartition() {
    for (double rate : {8000., 44100., 48000., 96000., 192000., 768000.}) {
        Effect e(rate); e.set(3, 0); e.processor.reset();
        std::array<float, 1200> left{}, right{}; left[0] = 0.4f; right[0] = -0.7f;
        e.process(left.data(), right.data(), 0);
        check(left[0] == 0.4f, "zero block does not advance delayed dry state");
        e.process(left.data(), right.data(), 1200);
        bool impulse = true;
        for (unsigned i = 0; i < left.size(); ++i) impulse &= left[i] == (i == 8 ? 0.4f : 0) && right[i] == (i == 8 ? -0.7f : 0);
        check(impulse && e.processor.latencySamples() == 8, "Mix zero is exact reported eight-sample stereo delay at all accepted rates");
        e.set(3, 100); e.set(0, 0); e.set(1, 8000); e.set(2, 0); e.processor.reset();
        left.fill(0); right.fill(0); left[0] = 0.001f;
        e.process(left.data(), right.data(), 1200);
        const auto peak = std::max_element(left.begin(), left.begin() + 32);
        check(std::distance(left.begin(), peak) >= 8 && std::distance(left.begin(), peak) <= 9,
              "small-signal wet impulse peak agrees with FIR group delay plus tone-filter phase");
        check(std::all_of(right.begin(), right.end(), [](float v) { return v == 0; }), "Distortion channel histories preserve independent stereo input");
        e.processor.reset(); left.fill(0); e.process(left.data(), right.data(), 1200);
        check(std::all_of(left.begin(), left.end(), [](float v) { return v == 0; }), "reset clears FIR, dry, voicing and tone state");
        check(e.heapFree, "zero, oversized and rate-specific processing is heap-free");
    }
    Effect a, b;
    std::array<float, 1200> al{}, ar{}, bl{}, br{};
    bool same = true;
    for (int block = 0; block < 8; ++block) {
        for (unsigned i = 0; i < al.size(); ++i) { al[i] = bl[i] = static_cast<float>(0.2 * std::sin((block * al.size() + i) * 0.04)); ar[i] = br[i] = -al[i]; }
        a.process(al.data(), ar.data(), 1200);
        for (unsigned i = 0; i < al.size(); ++i) b.process(bl.data() + i, br.data() + i, 1);
        same &= al == bl && ar == br;
    }
    check(same, "fixed controls give identical samples across variable block partitions");
    DistortionProcessor replacement{a.bank, 0}; replacement.prepare(48000, 512);
    heapAudit::begin(); replacement.activate(&a.processor); const bool activationHeapFree = heapAudit::finish();
    al.fill(0.1f); ar.fill(-0.1f); bl = al; br = ar;
    a.process(al.data(), ar.data(), 1200);
    heapAudit::begin(); replacement.process(bl.data(), br.data(), 1200); const bool replacementHeapFree = heapAudit::finish();
    check(al == bl && ar == br && activationHeapFree && replacementHeapFree,
          "bounded activation preserves matching instance FIR history and smoother state without heap operations");
    check(replacement.prepare(44100, 1) && !replacement.prepare(0, 512) && !replacement.prepare(48000, 0),
          "reprepare accepts changed rates and rejects invalid contracts");
}
void smoothingFiniteAndInstances() {
    Effect e;
    e.set(3, 0); e.processor.reset();
    std::array<float, 512> left{}, right{};
    for (int b = 0; b < 8; ++b) { left.fill(0.2f); right.fill(-0.1f); e.process(left.data(), right.data(), 512); }
    e.set(0, 48); e.set(1, 400); e.set(2, 6); e.set(3, 100);
    float previous = left.back(); bool continuous = true;
    for (int b = 0; b < 8; ++b) {
        left.fill(0.2f); right.fill(-0.1f); e.process(left.data(), right.data(), 512);
        for (auto v : left) { continuous &= std::abs(v - previous) < 0.015f; previous = v; }
    }
    check(continuous && std::abs(previous) < 0.001f, "20ms control ramps remain continuous and bass-tightening rejects DC after settling");
    const std::array<float, 5> abnormal{std::numeric_limits<float>::infinity(), std::numeric_limits<float>::quiet_NaN(),
        std::numeric_limits<float>::max(), -std::numeric_limits<float>::max(), 0};
    bool finite = true;
    for (int b = 0; b < 40; ++b) {
        e.set(3, b % 2 ? 0 : 100);
        for (unsigned i = 0; i < left.size(); ++i) { left[i] = abnormal[i % abnormal.size()]; right[i] = -left[i]; }
        e.process(left.data(), right.data(), 512);
        for (unsigned i = 0; i < left.size(); ++i) finite &= std::isfinite(left[i]) && std::isfinite(right[i]) && std::abs(left[i]) <= 1000001;
    }
    check(finite && e.heapFree, "non-finite/extreme input, rapid mix changes and state remain bounded without heap activity");
    Effect first, other, untouched;
    first.set(0, 40); first.set(1, 400); first.set(3, 100);
    render(first, 1500);
    const auto second = render(other, 750), third = render(untouched, 750);
    check(second == third, "each instance owns controls and temporal histories independently");
}
void aliasEvaluation() {
    Effect e; e.set(0, 24); e.set(1, 8000); e.set(2, 0); e.set(3, 100);
    const auto oversampled = render(e, 9000);
    std::vector<float> native(8192);
    const double hpPole = std::exp(-2 * std::numbers::pi * 90 / 48000), lpPole = std::exp(-2 * std::numbers::pi * 8000 / 48000);
    double hp = 0, last = 0, tone = 0;
    for (int i = -8192; i < 8192; ++i) {
        const double input = 0.2 * std::sin(2 * std::numbers::pi * 9000 * i / 48000);
        hp = hpPole * (hp + input - last); last = input;
        tone = (1 - lpPole) * std::clamp(hp * std::pow(10., 24. / 20), -1., 1.) + lpPole * tone;
        if (i >= 0) native[static_cast<unsigned>(i)] = static_cast<float>(tone);
    }
    // 9kHz at 48kHz has no desired higher harmonic below Nyquist. Its folded
    // odd harmonics land at 3/15/21kHz; compare relative to each fundamental.
    double osAlias = 0, nativeAlias = 0;
    for (double f : {3000., 15000., 21000.}) { osAlias += energy(oversampled, f); nativeAlias += energy(native, f); }
    osAlias /= energy(oversampled, 9000); nativeAlias /= energy(native, 9000);
    const double reduction = 10 * std::log10(nativeAlias / osAlias);
    std::cout << "Distortion coherent 9kHz alias/fundamental: native " << 10 * std::log10(nativeAlias)
              << " dB; 4x FIR " << 10 * std::log10(osAlias) << " dB; reduction " << reduction << " dB\n";
    check(reduction > 10 && std::isfinite(reduction), "fixed 4x FIR measurably reduces folded high-frequency clipping harmonics");
}
}
int main() {
    controlsAndTransfer(); latencyResetAndPartition(); smoothingFiniteAndInstances(); aliasEvaluation();
    if (failures == 0) std::cout << "Distortion signal and real-time checks passed\n";
    return failures == 0 ? 0 : 1;
}
