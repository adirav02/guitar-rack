#include "AudioSession.h"
#include <cmath>

namespace guitarrack {
AudioSession::AudioSession() : AudioSession(nullptr) { refresh(); }
AudioSession::~AudioSession() { stopNow(); }
void AudioSession::refresh() {
    if (started) return;
    device.reset();
    types.clear();
    if (auto* type = juce::AudioIODeviceType::createAudioIODeviceType_JACK()) types.emplace_back(type);
    if (auto* type = juce::AudioIODeviceType::createAudioIODeviceType_ALSA()) types.emplace_back(type);
    for (auto& type : types) type->scanForDevices();
}
juce::StringArray AudioSession::backendNames() const {
    juce::StringArray names;
    for (const auto& type : types) names.add(type->getTypeName());
    return names;
}
juce::AudioIODeviceType* AudioSession::backend(int index) const {
    return index >= 0 && index < static_cast<int>(types.size()) ? types[static_cast<std::size_t>(index)].get() : nullptr;
}
juce::String AudioSession::probe(int typeIndex, const juce::String& input, const juce::String& output) {
    if (started) return "Stop before changing devices.";
    device.reset();
    auto* type = backend(typeIndex);
    if (!type || input.isEmpty() || output.isEmpty()) return "Choose an input and an output device.";
    device.reset(type->createDevice(output, input));
    if (!device) return "Cannot create this device pair. Choose devices from the same backend.";
    return {};
}
juce::String AudioSession::start(int in, StereoRoute out, double rate, int block) {
    if (started) return "Already started.";
    if (!device) return "Select valid input and output devices first.";
    const int inputCount = device->getInputChannelNames().size();
    const int outputCount = device->getOutputChannelNames().size();
    if (in < 0 || in >= inputCount || !out.valid() || out.left >= outputCount || out.right >= outputCount)
        return "Select one mono input and two distinct output channels.";
    if (!std::isfinite(rate) || rate < 8000 || rate > 768000 || block <= 0)
        return "Select a supported sample rate and buffer size.";

    physicalInput = in; route = out; requestedRate = rate; requestedBlock = block;
    packedLeft = route.packedLeft(); packedRight = route.packedRight();
    juce::BigInteger inputs, outputs;
    inputs.setBit(in); outputs.setBit(out.left); outputs.setBit(out.right);
    // Opening the backend before registering our callback cannot pass input to output.
    auto error = device->open(inputs, outputs, rate, block);
    if (error.isNotEmpty()) { device->close(); return "Open failed: " + error; }
    actualRate = device->getCurrentSampleRate();
    actualBlock = device->getCurrentBufferSizeSamples();
    if (device->getActiveInputChannels() != inputs || device->getActiveOutputChannels() != outputs
        || !std::isfinite(actualRate) || actualRate < 8000 || actualBlock <= 0) {
        device->close();
        return "Device did not activate the selected mono input and stereo outputs. Refresh and reselect.";
    }
    fault.store(0); heartbeat.store(0); lastHeartbeat = 0;
    stopping = false; started = true; authorized.store(true);
    lastCallbackTime = juce::Time::getMillisecondCounterHiRes();
    device->start(this);
    if (!device->isPlaying() || fault.load() != 0) {
        stopNow();
        return "Device failed to start. Check connections, backend and channel availability.";
    }
    message = "Monitoring. Input " + juce::String(in + 1) + " -> outputs "
        + juce::String(out.left + 1) + " / " + juce::String(out.right + 1) + ".";
    return {};
}
void AudioSession::requestStop() noexcept {
    if (!started || stopping) return;
    authorized.store(false);
    stopping = true;
    // GUI thread: allow a 20ms audio ramp, with a bounded timeout if callbacks stop.
    stopDeadline = juce::Time::getMillisecondCounterHiRes() + 250;
}
void AudioSession::stopNow() {
    authorized.store(false);
    started = false; stopping = false;
    if (device) { device->stop(); device->close(); }
    processor.releaseResources();
}
void AudioSession::poll() {
    if (!started || !device) return;
    const auto now = juce::Time::getMillisecondCounterHiRes();
    const auto beat = heartbeat.load(std::memory_order_relaxed);
    if (beat != lastHeartbeat) { lastHeartbeat = beat; lastCallbackTime = now; }
    if (stopping) {
        if (processor.engine.isSilent() || now >= stopDeadline) {
            stopNow(); message = "Stopped. Press Start to monitor again.";
        }
        return;
    }
    if (fault.load() != 0 || !device->isOpen() || !device->isPlaying()) {
        stopNow();
        message = "Audio stopped: device failed or disconnected. Refresh / Reconnect, reselect, then Start.";
        return;
    }
    // Query only outside audio. Reconfiguration requires fresh explicit Start.
    juce::BigInteger expectedInput, expectedOutput;
    expectedInput.setBit(physicalInput);
    expectedOutput.setBit(route.left); expectedOutput.setBit(route.right);
    const bool changed = device->getCurrentSampleRate() != actualRate
        || device->getCurrentBufferSizeSamples() != actualBlock
        || device->getActiveInputChannels() != expectedInput
        || device->getActiveOutputChannels() != expectedOutput;
    const double timeout = std::max(2000.0, 4000.0 * actualBlock / actualRate);
    if (changed || now - lastCallbackTime > timeout) {
        stopNow();
        message = "Audio stopped: device failed, disconnected, or changed configuration. Refresh / Reconnect, reselect, then Start.";
    }
}
juce::String AudioSession::configuration() const {
    if (!started || !device) return "Device closed. DSP latency: 0 samples. Round-trip latency: not measured.";
    const auto latency = device->getInputLatencyInSamples() + device->getOutputLatencyInSamples();
    const auto xruns = device->getXRunCount();
    return "Requested: " + juce::String(requestedRate, 0) + " Hz / " + juce::String(requestedBlock)
        + " frames. Actual: " + juce::String(actualRate, 0) + " Hz / " + juce::String(actualBlock)
        + " frames.\nDevice-reported I/O estimate: " + juce::String(1000.0 * latency / actualRate, 2)
        + " ms. DSP: 0 samples. Round trip: not measured.\nBackend xruns: "
        + (xruns < 0 ? juce::String("unavailable") : juce::String(xruns));
}
void AudioSession::audioDeviceAboutToStart(juce::AudioIODevice* current) {
    const auto rate = current ? current->getCurrentSampleRate() : 0;
    juce::BigInteger expectedInput, expectedOutput;
    expectedInput.setBit(physicalInput); expectedOutput.setBit(route.left); expectedOutput.setBit(route.right);
    // Never grant authorization here: backend restarts cannot turn monitoring on.
    if (!processor.engine.prepare(rate) || !current
        || current->getActiveInputChannels() != expectedInput
        || current->getActiveOutputChannels() != expectedOutput) {
        authorized.store(false); fault.store(1);
    }
}
void AudioSession::audioDeviceStopped() { authorized.store(false); fault.store(1); }
void AudioSession::audioDeviceError(const juce::String&) { authorized.store(false); fault.store(1); }
void AudioSession::audioDeviceIOCallbackWithContext(const float* const* inputs, int numInputs,
        float* const* outputs, int numOutputs, int samples, const juce::AudioIODeviceCallbackContext&) {
    juce::ScopedNoDenormals noDenormals;
    heartbeat.fetch_add(1, std::memory_order_relaxed);
    if (samples <= 0) return;
    // Fail closed if backend connectivity changes the packed layout. No guessing.
    if (!inputs || !outputs || numInputs != 1 || numOutputs != 2 || !inputs[0]
        || !outputs[packedLeft] || !outputs[packedRight] || fault.load(std::memory_order_relaxed) != 0) {
        clearOutputs(outputs, numOutputs, samples);
        authorized.store(false); fault.store(1);
        return;
    }
    processor.engine.process(inputs[0], inputs[0], outputs[packedLeft], outputs[packedRight],
                             samples, authorized.load(std::memory_order_relaxed));
}
}
