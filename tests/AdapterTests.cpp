#include "standalone/AudioSession.h"
#include "HeapAudit.h"
#include <array>
#include <cmath>
#include <iostream>
#include <chrono>
#include <thread>

namespace guitarrack {
// Deterministic device substitute: opens no real backend and starts no threads.
class FakeDevice final : public juce::AudioIODevice {
public:
    FakeDevice() : AudioIODevice("Synthetic device", "Test") {}
    juce::StringArray getOutputChannelNames() override { return {"Out 1", "Out 2", "Out 3", "Out 4"}; }
    juce::StringArray getInputChannelNames() override { return {"Input 1", "Input 2"}; }
    juce::Array<double> getAvailableSampleRates() override { return {44100, 48000, 96000}; }
    juce::Array<int> getAvailableBufferSizes() override { return {128, 256}; }
    int getDefaultBufferSize() override { return 256; }
    juce::String open(const juce::BigInteger& ins, const juce::BigInteger& outs, double sr, int n) override {
        ++opens;
        if (failOpen) return "Synthetic open failure";
        inputs = ins; outputs = outs; rate = sr; block = n; opened = true;
        if (substituteChannels) inputs.setBit(0);
        return {};
    }
    void close() override { opened = false; }
    bool isOpen() override { return opened; }
    void start(juce::AudioIODeviceCallback* next) override { callback = next; next->audioDeviceAboutToStart(this); }
    void stop() override { if (callback) { auto* old = callback; callback = nullptr; old->audioDeviceStopped(); } }
    bool isPlaying() override { return callback != nullptr && opened; }
    juce::String getLastError() override { return {}; }
    int getCurrentBufferSizeSamples() override { return block; }
    double getCurrentSampleRate() override { return rate; }
    int getCurrentBitDepth() override { return 32; }
    juce::BigInteger getActiveOutputChannels() const override { return outputs; }
    juce::BigInteger getActiveInputChannels() const override { return inputs; }
    int getOutputLatencyInSamples() override { return 128; }
    int getInputLatencyInSamples() override { return 128; }
    int getXRunCount() const noexcept override { return -1; }
    juce::AudioIODeviceCallback* callback = nullptr;
    juce::BigInteger inputs, outputs;
    double rate = 48000;
    int block = 128, opens = 0;
    bool opened = false, failOpen = false, substituteChannels = false;
};
struct AudioSessionTestAccess {
    static std::unique_ptr<AudioSession> make() { return std::unique_ptr<AudioSession>(new AudioSession(nullptr)); }
    static FakeDevice& install(AudioSession& session) {
        auto fake = std::make_unique<FakeDevice>();
        auto& result = *fake; session.device = std::move(fake); return result;
    }
    static void process(AudioSession& session, const float* const* in, int ins, float* const* out, int outs, int frames) {
        session.audioDeviceIOCallbackWithContext(in, ins, out, outs, frames, {});
    }
    static void error(AudioSession& session) { session.audioDeviceError("Synthetic disconnect"); }
};
}
int main() {
    using namespace guitarrack;
    int failures = 0;
    const auto check = [&](bool value, const char* detail) { if (!value) { ++failures; std::cerr << "FAIL: " << detail << '\n'; } };
    std::array<float, 1200> selected{}, out1{}, out2{}, extra{};
    selected.fill(0.2f);
    const float* in[]{selected.data()}; float* out[]{out1.data(), out2.data()};
    auto session = AudioSessionTestAccess::make(); auto& device = AudioSessionTestAccess::install(*session);
    check(!session->isBusy() && device.opens == 0 && device.callback == nullptr, "launch opens no stream");
    check(session->start(-1, {0, 1}, 48000, 128).isNotEmpty() && device.opens == 0, "invalid selection never opens input");
    device.failOpen = true;
    check(session->start(1, {3, 1}, 48000, 128).contains("Synthetic open failure") && !session->isBusy(), "open error retained and stopped");
    device.failOpen = false; device.substituteChannels = true;
    check(session->start(1, {3, 1}, 48000, 128).isNotEmpty() && !device.opened, "backend channel substitution rejected");
    device.substituteChannels = false;
    check(session->start(1, {3, 1}, 48000, 128).isEmpty(), "explicit Start succeeds");
    check(device.inputs.countNumberOfSetBits() == 1 && device.inputs[1] && device.outputs[3] && device.outputs[1], "physical channel masks exact");
    heapAudit::begin();
    AudioSessionTestAccess::process(*session, in, 1, out, 2, 1200);
    check(heapAudit::finish(), "standalone callback performs no heap operations");
    const float expected = 0.2f * std::pow(10.f, -12.f / 20);
    check(std::abs(out1[1199] - expected) < 0.00001f && out1 == out2, "actual standalone callback routes packed mono to both selected outputs");
    check(session->configuration().contains("Round trip: not measured") && session->configuration().contains("unavailable"), "unknown measurements labeled honestly");
    check(session->processor.rack.add() && session->processor.rack.add(), "live standalone rack edits accepted");
    const auto rackDefinition = session->processor.rack.snapshot().requested;
    session->processor.rack.setParameter(rackDefinition.order[0], 0, -6);
    session->processor.rack.setParameter(rackDefinition.order[1], 0, 3);
    bool committed = false, heapFree = true;
    const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(4);
    do {
        heapAudit::begin(); AudioSessionTestAccess::process(*session, in, 1, out, 2, 1200);
        heapFree &= heapAudit::finish();
        const auto s = session->processor.rack.snapshot(); committed = s.requestedGeneration == s.committedGeneration;
        if (!committed) std::this_thread::sleep_for(std::chrono::milliseconds(1));
    } while (!committed && std::chrono::steady_clock::now() < deadline);
    AudioSessionTestAccess::process(*session, in, 1, out, 2, 1200);
    check(committed && heapFree, "actual device callback swaps live racks without heap operations");
    check(std::abs(out1.back() - expected * std::pow(10.f, -3.f / 20)) < 0.00001f && out1 == out2,
          "multiple Gain instances preserve selected mono routing and output level");
    for (const auto* type : {"overdrive", "eq", "delay"}) check(session->processor.rack.add(type), "M3 effect added to device adapter");
    committed = false;
    const auto effectDeadline = std::chrono::steady_clock::now() + std::chrono::seconds(4);
    do {
        heapAudit::begin(); AudioSessionTestAccess::process(*session, in, 1, out, 2, 1200);
        heapFree &= heapAudit::finish();
        const auto state = session->processor.rack.snapshot(); committed = state.requestedGeneration == state.committedGeneration;
        if (!committed) std::this_thread::sleep_for(std::chrono::milliseconds(1));
    } while (!committed && std::chrono::steady_clock::now() < effectDeadline);
    check(committed && heapFree && out1 == out2 && std::isfinite(out1.back()), "actual standalone M3 callback preserves stereo routing and heap-free transitions");
    session->requestStop(); AudioSessionTestAccess::process(*session, in, 1, out, 2, 1200); session->poll();
    check(out1[1199] == 0 && !session->isBusy() && !device.opened, "Stop fades then closes stream");
    check(session->start(1, {0, 1}, 48000, 128).isEmpty(), "restart requires Start");
    AudioSessionTestAccess::process(*session, in, 1, out, 2, 1200);
    AudioSessionTestAccess::error(*session); out1.fill(7); out2.fill(7); extra.fill(7);
    float* faultOutputs[]{out1.data(), out2.data(), extra.data()};
    heapAudit::begin();
    AudioSessionTestAccess::process(*session, in, 1, faultOutputs, 3, 1200);
    check(heapAudit::finish(), "failure silence path performs no heap operations");
    check(out1[1199] == 0 && out2[1199] == 0 && extra[1199] == 0, "failure immediately silences all outputs");
    session->poll(); check(!session->isBusy() && session->status().contains("Refresh"), "disconnect reports reconnect action");
    session->poll(); check(!device.opened, "poll never reconnects automatically");
    check(session->start(1, {0, 1}, 48000, 128).isEmpty(), "explicit reconnect can start");
    device.rate = 96000; session->poll();
    check(!session->isBusy() && !device.opened, "actual sample rate change safely stops monitoring");
    check(session->start(1, {0, 1}, 96000, 256).isEmpty(), "new rate and buffer accepted on explicit restart");
    device.block = 128; session->poll(); check(!session->isBusy(), "actual buffer change safely stops");

    MonitorProcessor processor;
    juce::AudioProcessor::BusesLayout mono, stereo, invalid;
    mono.inputBuses.add(juce::AudioChannelSet::mono()); mono.outputBuses.add(juce::AudioChannelSet::stereo());
    stereo.inputBuses.add(juce::AudioChannelSet::stereo()); stereo.outputBuses.add(juce::AudioChannelSet::stereo());
    invalid.inputBuses.add(juce::AudioChannelSet::stereo()); invalid.outputBuses.add(juce::AudioChannelSet::mono());
    check(processor.isBusesLayoutSupported(mono) && processor.isBusesLayoutSupported(stereo) && !processor.isBusesLayoutSupported(invalid), "shared processor validates intended bus layouts");
    auto extraBus = stereo; extraBus.inputBuses.add(juce::AudioChannelSet::mono());
    check(!processor.isBusesLayoutSupported(extraBus), "extra input buses rejected");
    processor.setBusesLayout(mono); processor.controls.setOutputDb(0); processor.prepareToPlay(48000, 128);
    juce::AudioBuffer<float> buffer(2, 1200); juce::MidiBuffer midi;
    buffer.clear(); processor.processBlock(buffer, midi);
    buffer.clear(); buffer.setSample(0, 55, 0.3f);
    heapAudit::begin(); processor.processBlock(buffer, midi);
    check(heapAudit::finish(), "shared JUCE processBlock performs no heap operations");
    check(buffer.getSample(0, 55) == 0.3f && buffer.getSample(1, 55) == 0.3f, "JUCE processor shares neutral mono processing and impulse timing");
    check(processor.getTailLengthSeconds() == 0, "empty shared processor reports no tail");
    for (const auto* type : {"overdrive", "eq", "delay"}) check(processor.rack.add(type), "M3 effect added to shared processor");
    committed = false; heapFree = true;
    const auto processorDeadline = std::chrono::steady_clock::now() + std::chrono::seconds(4);
    do {
        buffer.clear(); buffer.setSample(0, 55, 0.3f);
        heapAudit::begin(); processor.processBlock(buffer, midi); heapFree &= heapAudit::finish();
        const auto state = processor.rack.snapshot(); committed = state.requestedGeneration == state.committedGeneration;
        if (!committed) std::this_thread::sleep_for(std::chrono::milliseconds(1));
    } while (!committed && std::chrono::steady_clock::now() < processorDeadline);
    check(committed && heapFree && std::isfinite(buffer.getSample(0, 55)) && buffer.getSample(0, 55) == buffer.getSample(1, 55), "shared processBlock M3 activation and processing heap-free with mono-to-stereo bus");
    check(processor.getTailLengthSeconds() >= 116 && processor.getLatencySamples() == 0, "shared processor reports conservative stateful tail separately from zero processing latency");
    if (failures) return 1;
    std::cout << "PASS: no-auto-start, channel validation, real callback routing, Stop, failure silence, explicit reconnect, rate/buffer changes, processor buses\n";
}
