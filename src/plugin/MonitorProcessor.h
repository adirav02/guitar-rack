#pragma once
#include <juce_audio_processors_headless/juce_audio_processors_headless.h>
#include "core/MonitorEngine.h"

namespace guitarrack {
// Shared JUCE adapter. M3 uses the fixed bank internally; host publication is M6.
class MonitorProcessor final : public juce::AudioProcessor {
public:
    MonitorProcessor();
    const juce::String getName() const override { return "GuitarRack"; }
    void prepareToPlay(double rate, int) override { engine.prepare(rate); }
    void releaseResources() override { engine.reset(); }
    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    bool isBusesLayoutSupported(const BusesLayout&) const override;
    double getTailLengthSeconds() const override { return rack.tailSeconds(); }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    bool hasEditor() const override { return false; }
    juce::AudioProcessorEditor* createEditor() override { return nullptr; }
    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram(int) override {}
    const juce::String getProgramName(int) override { return "Clean monitoring"; }
    void changeProgramName(int, const juce::String&) override {}
    void getStateInformation(juce::MemoryBlock& block) override { block.reset(); }
    void setStateInformation(const void*, int) override {}

    MonitorControls controls;
    RackEngine rack;
    MonitorEngine engine{controls, &rack};
};
}
