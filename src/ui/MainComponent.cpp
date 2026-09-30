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
    label(title, "GuitarRack | Serial rack");
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
    backendBox.onChange = [this] { remembered.reset(); backendChanged(); };
    inputDevice.onChange = outputDevice.onChange = [this] { remembered.reset(); devicesChanged(); };
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
    refreshButton.onClick = [this] { refreshDevices(); if (remembered) restorePreferences(); };
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
    addAndMakeVisible(rackEditor); addAndMakeVisible(presetBar);
    refreshDevices();
    setSize(920, 1080); restorePreferences();
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
        int buffer = buffers.indexOf(256);
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
    startButton.setEnabled(!busy && !presetBar.presets.busy() && session.preview() && inputChannel.getSelectedItemIndex() >= 0
        && leftChannel.getSelectedItemIndex() >= 0 && rightChannel.getSelectedItemIndex() >= 0
        && leftChannel.getSelectedItemIndex() != rightChannel.getSelectedItemIndex()
        && rateBox.getSelectedItemIndex() >= 0 && bufferBox.getSelectedItemIndex() >= 0);
}
void MainComponent::timerCallback() {
    session.poll(); presetBar.poll(); updateEnabled();
    if (!presetBar.presets.loading()) rackEditor.poll();
    const bool loading = presetBar.presets.busy();
    rackEditor.setEnabled(!loading); inputGain.setEnabled(!loading); outputGain.setEnabled(!loading);
    if (!loading) {
        inputGain.setValue(session.processor.controls.inputDb(), juce::dontSendNotification);
        outputGain.setValue(session.processor.controls.outputDb(), juce::dontSendNotification);
    }
    const auto meter = session.processor.engine.consumeMeters();
    clipFlags |= meter.flags;
    inputMeter.update(meter.inputPeak, (clipFlags & MonitorEngine::inputClipped) != 0);
    outputMeter.update(meter.outputPeak, (clipFlags & MonitorEngine::outputProtected) != 0);
    juce::String clips;
    if (clipFlags & MonitorEngine::inputClipped) clips += "Input clipping: reduce interface gain or trim. ";
    if (clipFlags & MonitorEngine::outputProtected) clips += "Peak protection active: lower trim/output. ";
    if (clipFlags & MonitorEngine::nonFiniteInput) clips += "Invalid input samples silenced. ";
    if (clipFlags & MonitorEngine::nonFiniteDSP) clips += "Invalid DSP samples silenced. ";
    clipLabel.setText(clips.isEmpty() ? "No clipping detected." : clips, juce::dontSendNotification);
    statusLabel.setText(setupError.isNotEmpty() ? setupError : session.status(), juce::dontSendNotification);
    configLabel.setText(session.configuration(), juce::dontSendNotification);
}
void MainComponent::restorePreferences() {
    try {
        if (!remembered) {
            auto path=userConfigDirectory()/"settings.json";
            if (!std::filesystem::exists(path)) return;
            remembered=parsePreferences(readBounded(path));
            setSize(remembered->width,remembered->height);
        }
        const auto& p=*remembered;
        backendBox.setSelectedItemIndex(exactDeviceIndex(session.backendNames(),p.backend),juce::dontSendNotification);
        backendChanged();
        // Exact name match only; missing input/output is left unselected.
        auto choose=[](juce::ComboBox& box,const std::string& name) { juce::StringArray names; for(int i=0;i<box.getNumItems();++i) names.add(box.getItemText(i)); int match=exactDeviceIndex(names,name); box.setSelectedItemIndex(match,juce::dontSendNotification); return match>=0; };
        bool inputFound=choose(inputDevice,p.inputDevice), outputFound=choose(outputDevice,p.outputDevice);
        devicesChanged();
        inputChannel.setSelectedItemIndex(p.input,juce::dontSendNotification);
        leftChannel.setSelectedItemIndex(p.left,juce::dontSendNotification); rightChannel.setSelectedItemIndex(p.right,juce::dontSendNotification);
        rateBox.setSelectedItemIndex(rates.indexOf(p.rate),juce::dontSendNotification); bufferBox.setSelectedItemIndex(buffers.indexOf(p.buffer),juce::dontSendNotification);
        if(!inputFound||!outputFound) setupError="Remembered device is unavailable. Reconnect or explicitly select devices; monitoring remains stopped.";
        updateEnabled();
    } catch(const std::exception& e) { setupError="Preferences were not restored: "+juce::String(e.what()); }
}
void MainComponent::persistLocal() {
    Preferences p;
    p.backend=backendBox.getText().toStdString(); p.inputDevice=inputDevice.getText().toStdString(); p.outputDevice=outputDevice.getText().toStdString();
    p.input=inputChannel.getSelectedItemIndex(); p.left=leftChannel.getSelectedItemIndex(); p.right=rightChannel.getSelectedItemIndex();
    auto ri=rateBox.getSelectedItemIndex(), bi=bufferBox.getSelectedItemIndex();
    if(ri>=0) p.rate=rates[ri];
    if(bi>=0) p.buffer=buffers[bi];
    p.width=std::clamp(getWidth(),850,1600); p.height=std::clamp(getHeight(),1030,1400);
    // Keep missing remembered device names so reconnect can restore them next launch.
    if(remembered&&(p.inputDevice.empty()||p.outputDevice.empty())) { auto width=p.width,height=p.height; p=*remembered; p.width=width; p.height=height; }
    atomicWrite(userConfigDirectory()/"settings.json",serializePreferences(p));
    // Close after Discard recalls the saved tone, never the discarded edits.
    auto& presets=presetBar.presets;
    presets.persistRecall();
}
void MainComponent::requestClose(std::function<void()> close) {
    presetBar.guard([safe=juce::Component::SafePointer<MainComponent>(this),close=std::move(close)] {
        if(!safe) return;
        try { safe->persistLocal(); close(); }
        catch(const std::exception& e) { safe->presetBar.presets.error="Could not save local state: "+std::string(e.what()); }
    });
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
    configLabel.setBounds(area.removeFromTop(76));
    area.removeFromTop(4); presetBar.setBounds(area.removeFromTop(96));
    area.removeFromTop(6); rackEditor.setBounds(area);
}
}
