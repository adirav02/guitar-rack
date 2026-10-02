#include "MainComponent.h"
#include "StudioTheme.h"
#include <cmath>

namespace guitarrack {
void PeakMeter::update(float peak, bool clipped) {
    value = std::max(peak, value * 0.82f);
    clip = clipped;
    repaint();
}
void PeakMeter::paint(juce::Graphics& g) {
    auto box = getLocalBounds().toFloat().reduced(1);
    g.setColour(studio::inset); g.fillRoundedRectangle(box, 5);
    const auto db = value > 0 ? 20 * std::log10(value) : -60.0f;
    const auto fill = std::clamp((db + 60) / 60, 0.0f, 1.0f);
    const auto track=box.reduced(5,5).withTrimmedRight(92);
    const int segments=juce::jmax(1,int(track.getWidth()/7));
    for(int i=0;i<segments;++i) {
        const float fraction=float(i+1)/float(segments);
        const auto colour=clip?studio::danger:fraction>.88f?studio::amber:studio::accent;
        g.setColour(colour.withAlpha(fraction<=fill?.95f:.12f));
        g.fillRoundedRectangle(track.getX()+float(i)*track.getWidth()/float(segments),track.getY(),juce::jmax(1.0f,track.getWidth()/float(segments)-2),track.getHeight(),1);
    }
    g.setColour(clip?studio::danger:studio::text);g.setFont(12.0f);
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
    label(title, "GuitarRack");
    title.setFont(juce::FontOptions(26.0f,juce::Font::bold));
    title.setColour(juce::Label::textColourId,studio::accent);
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
    studio::primary(startButton);
    stopButton.setColour(juce::TextButton::textColourOffId,studio::danger);
    muteButton.setColour(juce::TextButton::buttonOnColourId,juce::Colour(0xff71442b));
    hint.setColour(juce::Label::textColourId,studio::muted);
    configLabel.setColour(juce::Label::textColourId,studio::muted);
    configLabel.setFont(juce::FontOptions(12.0f));
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
    addAndMakeVisible(latencyProfile);latencyProfile.addItem("Legacy: 128 DSP samples",1);latencyProfile.addItem("Creative: 4096 DSP samples",2);latencyProfile.setSelectedId(1,juce::dontSendNotification);
    latencyProfile.setTooltip("Stopped-state session setting. Creative enables spectral pitch; all sounds in this session use 4096 samples. This is processor latency, not total I/O latency. Not stored in portable tones.");
    latencyProfile.onChange=[this]{if(!session.processor.rack.setLatencyProfile(latencyProfile.getSelectedId()==2?creativeLatencySamples:rackLatencySamples)) {presetBar.presets.error="Latency profile could not change: "+session.processor.rack.snapshot().error;latencyProfile.setSelectedId(session.processor.rack.latencySamples()==128?1:2,juce::dontSendNotification);}else session.processor.setLatencySamples(session.processor.rack.latencySamples());};
    addAndMakeVisible(audioSetup);audioSetup.onClick=[this]{audioCollapsed=!audioCollapsed;resized();};
    // Audio controls remain reachable on short displays without taking the
    // pedalboard off-screen. Reparenting does not replace any action handlers.
    for(auto* c:std::initializer_list<juce::Component*>{&hint,&backendLabel,&inputDeviceLabel,&outputDeviceLabel,&inputLabel,&leftLabel,&rightLabel,&rateLabel,&bufferLabel,&backendBox,&inputDevice,&outputDevice,&inputChannel,&leftChannel,&rightChannel,&rateBox,&bufferBox,&inputGainLabel,&outputGainLabel,&inputGain,&outputGain,&inputMeterLabel,&outputMeterLabel,&inputMeter,&outputMeter,&resetClips,&clipLabel,&configLabel}) console.addAndMakeVisible(c);
    addAndMakeVisible(consoleViewport);consoleViewport.setViewedComponent(&console,false);
    consoleViewport.setScrollBarsShown(true,false);
    refreshDevices();
    setSize(1280, 720); restorePreferences();
    startTimerHz(20);
}
MainComponent::~MainComponent() { stopTimer(); session.stopNow(); consoleViewport.setViewedComponent(nullptr,false); }
void MainComponent::paint(juce::Graphics& g) {
    g.fillAll(studio::background);
    studio::surface(g,juce::Rectangle<float>(8,8,float(getWidth()-16),float(presetBar.getY()-16)));
    g.setColour(studio::accent.withAlpha(.7f));g.fillRoundedRectangle(20,20,3,24,1.5f);
}
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
    latencyProfile.setEnabled(!busy&&!presetBar.presets.busy());
    refreshButton.setEnabled(!busy); stopButton.setEnabled(session.isRunning());
    startButton.setEnabled(!busy && !presetBar.presets.busy() && session.preview() && inputChannel.getSelectedItemIndex() >= 0
        && leftChannel.getSelectedItemIndex() >= 0 && rightChannel.getSelectedItemIndex() >= 0
        && leftChannel.getSelectedItemIndex() != rightChannel.getSelectedItemIndex()
        && rateBox.getSelectedItemIndex() >= 0 && bufferBox.getSelectedItemIndex() >= 0);
}
void MainComponent::updateConfigurationLabel() {
    const auto diagnostics=session.configuration();
    auto selection=backendBox.getText()+" | "+(inputDevice.getText().isEmpty()?juce::String("Select audio device"):inputDevice.getText());
    if(outputDevice.getText().isNotEmpty()&&outputDevice.getText()!=inputDevice.getText())selection+=" → "+outputDevice.getText();
    auto channel=[](const juce::ComboBox& box){return box.getSelectedItemIndex()<0?juce::String("?"):juce::String(box.getSelectedItemIndex()+1);};
    selection+=" | In "+channel(inputChannel)+" / Out "+channel(leftChannel)+"+"+channel(rightChannel);
    configLabel.setText(audioCollapsed?selection+" | "+diagnostics.upToFirstOccurrenceOf("\n",false,false):diagnostics,juce::dontSendNotification);
    configLabel.setTooltip(selection+"\n"+diagnostics);
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
    clipLabel.setColour(juce::Label::textColourId,clips.isEmpty()?studio::muted:studio::danger);
    clipLabel.setText(clips.isEmpty() ? "No clipping detected." : clips, juce::dontSendNotification);
    statusLabel.setText(setupError.isNotEmpty() ? setupError : session.status(), juce::dontSendNotification);
    statusLabel.setTooltip(statusLabel.getText());
    updateConfigurationLabel();
}
void MainComponent::restorePreferences() {
    try {
        if (!remembered) {
            auto path=userConfigDirectory()/"settings.json";
            if (!std::filesystem::exists(path)) return;
            remembered=parsePreferences(readBounded(path));
            setSize(remembered->width,remembered->height);
            if(session.processor.rack.setLatencyProfile(remembered->latencyProfile)) {
                latencyProfile.setSelectedId(remembered->latencyProfile==128?1:2,juce::dontSendNotification);
                session.processor.setLatencySamples(session.processor.rack.latencySamples());
            }
            audioCollapsed=remembered->audioCollapsed;presetBar.restoreLocal(*remembered);rackEditor.restoreLocal(*remembered);resized();
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
    p.width=std::clamp(getWidth(),700,7680); p.height=std::clamp(getHeight(),450,4320);
    // Keep missing remembered device names so reconnect can restore them next launch.
    if(remembered&&(p.inputDevice.empty()||p.outputDevice.empty())) { auto width=p.width,height=p.height; p=*remembered; p.width=width; p.height=height; }
    p.latencyProfile=int(session.processor.rack.latencySamples());p.audioCollapsed=audioCollapsed;presetBar.captureLocal(p);rackEditor.captureLocal(p);
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
    auto area=getLocalBounds().reduced(20,16);
    const bool compact=audioCollapsed||getHeight()<600;
    auto header=area.removeFromTop(38);audioSetup.setBounds(header.removeFromRight(150));latencyProfile.setBounds(header.removeFromRight(220).reduced(4));title.setBounds(header.withTrimmedLeft(10));
    audioSetup.setButtonText(audioCollapsed?"Audio Setup ▸":"Audio Setup ▾");
    for(auto* c:std::initializer_list<juce::Component*>{&hint,&backendLabel,&inputDeviceLabel,&outputDeviceLabel,&inputLabel,&leftLabel,&rightLabel,&rateLabel,&bufferLabel,&backendBox,&inputDevice,&outputDevice,&inputChannel,&leftChannel,&rightChannel,&rateBox,&bufferBox}) c->setVisible(!audioCollapsed);
    auto buttons=area.removeFromTop(36);
    for(auto* button:{&startButton,&stopButton,&muteButton}){button->setBounds(buttons.removeFromLeft(100).reduced(2));}
    refreshButton.setBounds(buttons.removeFromLeft(175).reduced(2));statusLabel.setBounds(buttons);
    const int consoleHeight=(audioCollapsed?0:128)+(compact?112:158);
    const int visibleConsoleHeight=std::min(consoleHeight,std::max(68,getHeight()/3-36));
    consoleViewport.setBounds(area.removeFromTop(visibleConsoleHeight));
    console.setSize(consoleViewport.getWidth()-consoleViewport.getScrollBarThickness(),consoleHeight);
    auto workspace=area;
    area=console.getLocalBounds();
    const int gap=12,half=(area.getWidth()-gap)/2;
    if(!audioCollapsed) {
        hint.setBounds(area.removeFromTop(36));
        auto deviceLabels=area.removeFromTop(18);auto devices=area.removeFromTop(26);
        int third=(area.getWidth()-2*gap)/3;
        for(auto pair:{std::pair{&backendLabel,&backendBox},std::pair{&inputDeviceLabel,&inputDevice},std::pair{&outputDeviceLabel,&outputDevice}}) {
            pair.first->setBounds(deviceLabels.removeFromLeft(third));deviceLabels.removeFromLeft(gap);
            pair.second->setBounds(devices.removeFromLeft(third));devices.removeFromLeft(gap);
        }
        area.removeFromTop(4);auto labels=area.removeFromTop(18);auto selectors=area.removeFromTop(26);
        int fifth=(area.getWidth()-4*gap)/5;
        for(auto pair:{std::pair{&inputLabel,&inputChannel},std::pair{&leftLabel,&leftChannel},std::pair{&rightLabel,&rightChannel},std::pair{&rateLabel,&rateBox},std::pair{&bufferLabel,&bufferBox}}) {
            pair.first->setBounds(labels.removeFromLeft(fifth));labels.removeFromLeft(gap);
            pair.second->setBounds(selectors.removeFromLeft(fifth));selectors.removeFromLeft(gap);
        }
    }
    inputGainLabel.setText(compact?"Input trim":"Input trim (-24 to +24 dB)",juce::dontSendNotification);
    outputGainLabel.setText(compact?"Output level":"Output level (-60 to +6 dB)",juce::dontSendNotification);
    inputMeterLabel.setText(compact?"Input peak":"Input peak after trim",juce::dontSendNotification);
    inputMeterLabel.setTooltip("Input peak after trim. Raw full-scale input is also checked for clipping.");
    if(compact) {
        auto gains=area.removeFromTop(32);auto left=gains.removeFromLeft(half);gains.removeFromLeft(gap);
        inputGainLabel.setBounds(left.removeFromLeft(96));inputGain.setBounds(left);
        outputGainLabel.setBounds(gains.removeFromLeft(96));outputGain.setBounds(gains);
        auto meters=area.removeFromTop(26);left=meters.removeFromLeft(half);meters.removeFromLeft(gap);
        inputMeterLabel.setBounds(left.removeFromLeft(96));inputMeter.setBounds(left);
        outputMeterLabel.setBounds(meters.removeFromLeft(96));outputMeter.setBounds(meters);
    } else {
    auto gainsText=area.removeFromTop(18);inputGainLabel.setBounds(gainsText.removeFromLeft(half));gainsText.removeFromLeft(gap);outputGainLabel.setBounds(gainsText);
    auto gains=area.removeFromTop(30);inputGain.setBounds(gains.removeFromLeft(half));gains.removeFromLeft(gap);outputGain.setBounds(gains);
    auto labels=area.removeFromTop(18);inputMeterLabel.setBounds(labels.removeFromLeft(half));labels.removeFromLeft(gap);outputMeterLabel.setBounds(labels);
    auto meters=area.removeFromTop(24);inputMeter.setBounds(meters.removeFromLeft(half));meters.removeFromLeft(gap);outputMeter.setBounds(meters);
    }
    auto clips=area.removeFromTop(26);resetClips.setBounds(clips.removeFromRight(120));clipLabel.setBounds(clips);
    configLabel.setBounds(area.removeFromTop(compact?28:42));updateConfigurationLabel();
    area=workspace;area.removeFromTop(12);presetBar.setBounds(area.removeFromTop(72));area.removeFromTop(12);rackEditor.setBounds(area);
}
}
