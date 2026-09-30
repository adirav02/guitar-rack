#include "MainComponent.h"
#include <cmath>

namespace guitarrack {
void PeakMeter::update(float peak, bool clipped) {
    value = std::max(peak, value * 0.82f);
    clip = clipped;
    repaint();
}
void PeakMeter::paint(juce::Graphics& g) {
    auto box = getLocalBounds().toFloat();
    g.setColour(juce::Colour(0xff17212d)); g.fillRoundedRectangle(box, 4);
    const auto db = value > 0 ? 20 * std::log10(value) : -60.0f;
    const auto fill = std::clamp((db + 60) / 60, 0.0f, 1.0f);
    g.setColour(clip ? juce::Colour(0xffed655f) : juce::Colour(0xff5ad7b0));
    g.fillRoundedRectangle(box.withWidth(box.getWidth() * fill), 4);
    g.setColour(juce::Colours::white);
    g.drawText(value > 0.001f ? juce::String(db, 1) + " dBFS" : "< -60 dBFS", getLocalBounds().reduced(8), juce::Justification::centredRight);
}
void MainComponent::label(juce::Label& item, const juce::String& text) {
    item.setText(text, juce::dontSendNotification);
    item.setColour(juce::Label::textColourId, juce::Colour(0xffdde5ef));
    item.setJustificationType(juce::Justification::centredLeft);
    addAndMakeVisible(item);
}
MainComponent::MainComponent() {
    setWantsKeyboardFocus(true);
    label(title, "GuitarRack | Clean monitoring");
    title.setFont(juce::FontOptions(24));
    label(hint, "M-Track Solo: guitar in physical Input 2, Instrument mode, Direct/USB switch on USB.\nUse wired headphones or monitors on your interface. Start with low headphone volume.");
    label(backendLabel, "Audio backend");
    label(inputDeviceLabel, "Input device"); label(outputDeviceLabel, "Output device");
    label(inputLabel, "Mono guitar input"); label(leftLabel, "Left output"); label(rightLabel, "Right output");
    label(rateLabel, "Requested sample rate"); label(bufferLabel, "Requested buffer");
    label(inputGainLabel, "Input trim (-24 to +24 dB)"); label(outputGainLabel, "Output level (-60 to +6 dB)");
    label(inputMeterLabel, "Input peak after trim"); label(outputMeterLabel, "Output peak");
    label(statusLabel, session.status()); label(configLabel, session.configuration()); label(clipLabel, "No clipping detected.");

    for (auto* combo : {&backendBox, &inputDevice, &outputDevice, &inputChannel, &leftChannel, &rightChannel, &rateBox, &bufferBox}) {
        combo->setTextWhenNothingSelected("Select...");
        addAndMakeVisible(combo);
    }
    backendBox.onChange = [this] { backendChanged(); };
    inputDevice.onChange = outputDevice.onChange = [this] { devicesChanged(); };
    for (auto* combo : {&inputChannel, &leftChannel, &rightChannel, &rateBox, &bufferBox})
        combo->onChange = [this] { setupError.clear(); updateEnabled(); };

    for (auto* button : {&startButton, &stopButton, &muteButton, &refreshButton, &resetClips}) addAndMakeVisible(button);
    startButton.onClick = [this] { startAudio(); };
    stopButton.onClick = [this] { session.requestStop(); updateEnabled(); };
    muteButton.setClickingTogglesState(true);
    muteButton.onClick = [this] {
        session.processor.controls.setMuted(muteButton.getToggleState());
        muteButton.setButtonText(muteButton.getToggleState() ? "Unmute" : "Mute");
    };
    refreshButton.onClick = [this] { refreshDevices(); };
    resetClips.onClick = [this] { clipFlags = 0; };
    inputGain.setRange(-24, 24, 0.1); outputGain.setRange(-60, 6, 0.1);
    inputGain.setValue(0); outputGain.setValue(-12);
    for (auto* slider : {&inputGain, &outputGain}) {
        slider->setSliderStyle(juce::Slider::LinearHorizontal);
        slider->setTextBoxStyle(juce::Slider::TextBoxRight, false, 90, 26);
        slider->setTextValueSuffix(" dB"); addAndMakeVisible(slider);
    }
    inputGain.setDoubleClickReturnValue(true, 0); outputGain.setDoubleClickReturnValue(true, -12);
    inputGain.onValueChange = [this] { session.processor.controls.setInputDb(static_cast<float>(inputGain.getValue())); };
    outputGain.onValueChange = [this] { session.processor.controls.setOutputDb(static_cast<float>(outputGain.getValue())); };
    addAndMakeVisible(inputMeter); addAndMakeVisible(outputMeter);
    inputChannel.setTooltip("Choose one physical input. For M-Track Solo guitar, choose channel 2.");
    rateBox.setTooltip("JACK uses the existing server rate; GuitarRack does not change global audio policy.");
    bufferBox.setTooltip("JACK uses the existing server buffer. ALSA exposes the device's supported choices.");
    outputGain.setTooltip("Defaults to -12 dB. Output protection clamps peaks at 0.98; clipping changes tone.");
    refreshDevices();
    setSize(920, 750);
    startTimerHz(20);
}
MainComponent::~MainComponent() { stopTimer(); session.stopNow(); }
void MainComponent::paint(juce::Graphics& g) { g.fillAll(juce::Colour(0xff202b3a)); }
void MainComponent::refreshDevices() {
    if (session.isBusy()) return;
    const auto oldBackend = backendBox.getText();
    session.refresh();
    backendBox.clear(juce::dontSendNotification);
    backendBox.addItemList(session.backendNames(), 1);
    int index = session.backendNames().indexOf(oldBackend);
    if (index < 0 && !session.backendNames().isEmpty()) index = 0;
    backendBox.setSelectedItemIndex(index, juce::dontSendNotification);
    backendChanged();
}
void MainComponent::backendChanged() {
    const auto oldInput = inputDevice.getText(), oldOutput = outputDevice.getText();
    inputDevice.clear(juce::dontSendNotification); outputDevice.clear(juce::dontSendNotification);
    if (auto* type = session.backend(backendBox.getSelectedItemIndex())) {
        const auto inputs = type->getDeviceNames(true), outputs = type->getDeviceNames(false);
        inputDevice.addItemList(inputs, 1); outputDevice.addItemList(outputs, 1);
        inputDevice.setSelectedItemIndex(inputs.indexOf(oldInput), juce::dontSendNotification);
        outputDevice.setSelectedItemIndex(outputs.indexOf(oldOutput), juce::dontSendNotification);
    }
    devicesChanged();
}
void MainComponent::devicesChanged() {
    setupError = session.probe(backendBox.getSelectedItemIndex(), inputDevice.getText(), outputDevice.getText());
    for (auto* combo : {&inputChannel, &leftChannel, &rightChannel, &rateBox, &bufferBox}) combo->clear(juce::dontSendNotification);
    rates.clear(); buffers.clear();
    if (auto* device = session.preview()) {
        const auto inputNames = device->getInputChannelNames(), outputNames = device->getOutputChannelNames();
        for (int i = 0; i < inputNames.size(); ++i) inputChannel.addItem(juce::String(i + 1) + ": " + inputNames[i], i + 1);
        for (int i = 0; i < outputNames.size(); ++i) {
            const auto text = juce::String(i + 1) + ": " + outputNames[i];
            leftChannel.addItem(text, i + 1); rightChannel.addItem(text, i + 1);
        }
        if (outputNames.size() >= 2) {
            leftChannel.setSelectedId(1, juce::dontSendNotification);
            rightChannel.setSelectedId(2, juce::dontSendNotification);
        }
        rates = device->getAvailableSampleRates(); buffers = device->getAvailableBufferSizes();
        for (int i = 0; i < rates.size(); ++i) rateBox.addItem(juce::String(rates[i], 0) + " Hz", i + 1);
        for (int i = 0; i < buffers.size(); ++i) bufferBox.addItem(juce::String(buffers[i]) + " frames", i + 1);
        rateBox.setSelectedItemIndex(rates.contains(48000) ? rates.indexOf(48000) : (rates.isEmpty() ? -1 : 0), juce::dontSendNotification);
        int buffer = buffers.indexOf(128);
        if (buffer < 0) buffer = buffers.indexOf(256);
        if (buffer < 0) buffer = buffers.indexOf(device->getDefaultBufferSize());
        if (buffer < 0 && !buffers.isEmpty()) buffer = 0;
        bufferBox.setSelectedItemIndex(buffer, juce::dontSendNotification);
    }
    updateEnabled();
}
void MainComponent::startAudio() {
    const int rateIndex = rateBox.getSelectedItemIndex(), bufferIndex = bufferBox.getSelectedItemIndex();
    if (rateIndex < 0 || bufferIndex < 0) { setupError = "Select a supported sample rate and buffer."; return; }
    setupError = session.start(inputChannel.getSelectedItemIndex(),
        {leftChannel.getSelectedItemIndex(), rightChannel.getSelectedItemIndex()}, rates[rateIndex], buffers[bufferIndex]);
    updateEnabled();
}
void MainComponent::updateEnabled() {
    const bool busy = session.isBusy();
    for (auto* combo : {&backendBox, &inputDevice, &outputDevice, &inputChannel, &leftChannel, &rightChannel, &rateBox, &bufferBox}) combo->setEnabled(!busy);
    refreshButton.setEnabled(!busy); stopButton.setEnabled(session.isRunning());
    startButton.setEnabled(!busy && session.preview() && inputChannel.getSelectedItemIndex() >= 0
        && leftChannel.getSelectedItemIndex() >= 0 && rightChannel.getSelectedItemIndex() >= 0
        && leftChannel.getSelectedItemIndex() != rightChannel.getSelectedItemIndex()
        && rateBox.getSelectedItemIndex() >= 0 && bufferBox.getSelectedItemIndex() >= 0);
}
void MainComponent::timerCallback() {
    session.poll(); updateEnabled();
    const auto meter = session.processor.engine.consumeMeters();
    clipFlags |= meter.flags;
    inputMeter.update(meter.inputPeak, (clipFlags & MonitorEngine::inputClipped) != 0);
    outputMeter.update(meter.outputPeak, (clipFlags & MonitorEngine::outputProtected) != 0);
    juce::String clips;
    if (clipFlags & MonitorEngine::inputClipped) clips += "Input clipping: reduce interface gain or trim. ";
    if (clipFlags & MonitorEngine::outputProtected) clips += "Peak protection active: lower trim/output. ";
    if (clipFlags & MonitorEngine::nonFiniteInput) clips += "Invalid input samples silenced. ";
    clipLabel.setText(clips.isEmpty() ? "No clipping detected." : clips, juce::dontSendNotification);
    statusLabel.setText(setupError.isNotEmpty() ? setupError : session.status(), juce::dontSendNotification);
    configLabel.setText(session.configuration(), juce::dontSendNotification);
}
bool MainComponent::keyPressed(const juce::KeyPress& key) {
    if (key == juce::KeyPress::escapeKey) { session.requestStop(); return true; }
    return false;
}
void MainComponent::resized() {
    auto area = getLocalBounds().reduced(20);
    title.setBounds(area.removeFromTop(36));
    hint.setBounds(area.removeFromTop(52)); area.removeFromTop(8);
    auto backendRow = area.removeFromTop(32);
    backendLabel.setBounds(backendRow.removeFromLeft(150)); backendBox.setBounds(backendRow);
    area.removeFromTop(12);
    const int gap = 18, half = (area.getWidth() - gap) / 2;
    auto names = area.removeFromTop(24);
    inputDeviceLabel.setBounds(names.removeFromLeft(half)); names.removeFromLeft(gap); outputDeviceLabel.setBounds(names);
    auto devices = area.removeFromTop(30);
    inputDevice.setBounds(devices.removeFromLeft(half)); devices.removeFromLeft(gap); outputDevice.setBounds(devices);
    area.removeFromTop(12);
    const int third = (area.getWidth() - 2 * gap) / 3;
    auto channelsText = area.removeFromTop(24);
    inputLabel.setBounds(channelsText.removeFromLeft(third)); channelsText.removeFromLeft(gap);
    leftLabel.setBounds(channelsText.removeFromLeft(third)); channelsText.removeFromLeft(gap); rightLabel.setBounds(channelsText);
    auto channels = area.removeFromTop(30);
    inputChannel.setBounds(channels.removeFromLeft(third)); channels.removeFromLeft(gap);
    leftChannel.setBounds(channels.removeFromLeft(third)); channels.removeFromLeft(gap); rightChannel.setBounds(channels);
    area.removeFromTop(12);
    auto configText = area.removeFromTop(24);
    rateLabel.setBounds(configText.removeFromLeft(half)); configText.removeFromLeft(gap); bufferLabel.setBounds(configText);
    auto config = area.removeFromTop(30);
    rateBox.setBounds(config.removeFromLeft(half)); config.removeFromLeft(gap); bufferBox.setBounds(config);
    area.removeFromTop(20);
    auto buttons = area.removeFromTop(36);
    for (auto* button : {&startButton, &stopButton, &muteButton}) { button->setBounds(buttons.removeFromLeft(110)); buttons.removeFromLeft(12); }
    refreshButton.setBounds(buttons.removeFromLeft(190));
    area.removeFromTop(16);
    auto gainText = area.removeFromTop(24);
    inputGainLabel.setBounds(gainText.removeFromLeft(half)); gainText.removeFromLeft(gap); outputGainLabel.setBounds(gainText);
    auto gains = area.removeFromTop(36);
    inputGain.setBounds(gains.removeFromLeft(half)); gains.removeFromLeft(gap); outputGain.setBounds(gains);
    area.removeFromTop(12);
    auto meterText = area.removeFromTop(24);
    inputMeterLabel.setBounds(meterText.removeFromLeft(half)); meterText.removeFromLeft(gap); outputMeterLabel.setBounds(meterText);
    auto meters = area.removeFromTop(28);
    inputMeter.setBounds(meters.removeFromLeft(half)); meters.removeFromLeft(gap); outputMeter.setBounds(meters);
    area.removeFromTop(12);
    auto clipping = area.removeFromTop(44);
    resetClips.setBounds(clipping.removeFromRight(130).withHeight(30)); clipLabel.setBounds(clipping);
    statusLabel.setBounds(area.removeFromTop(52));
    configLabel.setBounds(area);
}
}
