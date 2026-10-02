#pragma once
#include <juce_audio_devices/juce_audio_devices.h>
#include "plugin/MonitorProcessor.h"
#include "core/Routing.h"
#include <memory>
#include <vector>

namespace guitarrack {
// Device ownership, discovery, open/close and strings stay on the message thread.
class AudioSession final : private juce::AudioIODeviceCallback {
public:
    AudioSession();
    ~AudioSession() override;
    void refresh();
    juce::StringArray backendNames() const;
    juce::AudioIODeviceType* backend(int index) const;
    juce::String probe(int type, const juce::String& input, const juce::String& output);
    juce::AudioIODevice* preview() const noexcept { return device.get(); }
    juce::String start(int inputChannel, StereoRoute outputs, double rate, int block);
    void requestStop() noexcept;
    void stopNow(); // GUI/lifecycle only, waits for backend callbacks to drain.
    // Poll on GUI timer: validates liveness/config; never starts/reopens a device.
    void poll();
    bool isRunning() const noexcept { return started && !stopping; }
    bool isBusy() const noexcept { return started; }
    const juce::String& status() const noexcept { return message; }
    juce::String configuration() const;
    MonitorProcessor processor;
private:
    friend struct AudioSessionTestAccess;
    explicit AudioSession(std::nullptr_t) {}
    void audioDeviceIOCallbackWithContext(const float* const*, int, float* const*, int, int,
                                         const juce::AudioIODeviceCallbackContext&) override;
    void audioDeviceAboutToStart(juce::AudioIODevice*) override;
    void audioDeviceStopped() override;
    void audioDeviceError(const juce::String&) override;

    std::vector<std::unique_ptr<juce::AudioIODeviceType>> types;
    std::unique_ptr<juce::AudioIODevice> device;
    int selectedBackend = -1;
    juce::String selectedInputDevice, selectedOutputDevice;
    std::atomic<bool> authorized{false};
    std::atomic<unsigned> fault{0}, heartbeat{0};
    bool started = false, stopping = false;
    int physicalInput = -1;
    StereoRoute route;
    int packedLeft = 0, packedRight = 1;
    double requestedRate = 0, actualRate = 0;
    int requestedBlock = 0, actualBlock = 0;
    unsigned lastHeartbeat = 0;
    double stopDeadline = 0, lastCallbackTime = 0;
    juce::String message{"Stopped. Select audio devices and channels, then press Start."};
};
}
