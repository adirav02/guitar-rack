#include "MonitorProcessor.h"

namespace guitarrack {
MonitorProcessor::MonitorProcessor()
    : AudioProcessor(BusesProperties()
        .withInput("Input", juce::AudioChannelSet::stereo(), true)
        .withOutput("Output", juce::AudioChannelSet::stereo(), true)) {}
bool MonitorProcessor::isBusesLayoutSupported(const BusesLayout& layout) const {
    if (layout.inputBuses.size() != 1 || layout.outputBuses.size() != 1) return false;
    const auto in = layout.getMainInputChannelSet();
    return (in == juce::AudioChannelSet::mono() || in == juce::AudioChannelSet::stereo())
        && layout.getMainOutputChannelSet() == juce::AudioChannelSet::stereo();
}
void MonitorProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer&) {
    juce::ScopedNoDenormals noDenormals;
    if (buffer.getNumChannels() < 2) { buffer.clear(); return; }
    auto* l = buffer.getWritePointer(0);
    auto* r = buffer.getWritePointer(1);
    engine.process(l, getTotalNumInputChannels() == 1 ? l : r, l, r, buffer.getNumSamples());
    for (int ch = 2; ch < buffer.getNumChannels(); ++ch) buffer.clear(ch, 0, buffer.getNumSamples());
}
}
