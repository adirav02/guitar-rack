#pragma once
#include <juce_gui_basics/juce_gui_basics.h>
#include "standalone/AudioSession.h"
#include "RackEditor.h"
#include "PresetBar.h"
#include "state/Preferences.h"

namespace guitarrack {
class PeakMeter final : public juce::Component, public juce::SettableTooltipClient {
public:
    void update(float peak, bool clipped);
    void paint(juce::Graphics&) override;
private:
    float value = 0;
    bool clip = false;
};

class MainComponent final : public juce::Component, private juce::Timer {
public:
    MainComponent();
    ~MainComponent() override;
    void paint(juce::Graphics&) override;
    void resized() override;
    bool keyPressed(const juce::KeyPress&) override;
    void requestClose(std::function<void()>);
    void persistLocal();
private:
    void timerCallback() override;
    void refreshDevices();
    void backendChanged();
    void devicesChanged();
    void startAudio();
    void updateEnabled();
    void updateConfigurationLabel();
    void restorePreferences();
    std::optional<Preferences> remembered;
    void label(juce::Label&, const juce::String&);
    bool audioCollapsed=true;
    juce::Component console;
    juce::Viewport consoleViewport;
    juce::ComboBox latencyProfile;
    juce::TextButton audioSetup{"Audio Setup"};
    AudioSession session;
    PresetBar presetBar{session.processor.rack};
    RackEditor rackEditor{session.processor.rack,presetBar.presets};
    juce::Label title, hint, backendLabel, inputDeviceLabel, outputDeviceLabel;
    juce::Label inputLabel, leftLabel, rightLabel, rateLabel, bufferLabel;
    juce::Label inputGainLabel, outputGainLabel, inputMeterLabel, outputMeterLabel;
    juce::Label statusLabel, configLabel, clipLabel;
    juce::ComboBox backendBox, inputDevice, outputDevice, inputChannel, leftChannel, rightChannel, rateBox, bufferBox;
    juce::TextButton startButton{"Start"}, stopButton{"Stop"}, muteButton{"Mute"}, refreshButton{"Refresh / Reconnect"}, resetClips{"Clear clipping"};
    juce::Slider inputGain, outputGain;
    PeakMeter inputMeter, outputMeter;
    juce::Array<double> rates;
    juce::Array<int> buffers;
    juce::String setupError;
    unsigned clipFlags = 0;
};
}
