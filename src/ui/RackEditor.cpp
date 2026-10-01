#include "RackEditor.h"
#include "core/SoundLibrary.h"
#include "core/CabinetAssets.h"
#include "state/PedalPresets.h"
#include <cmath>

namespace guitarrack {
namespace {
juce::Colour pedalColour(const std::string& type) {
    if(type=="overdrive") return juce::Colour(0xff3ea66d);
    if(type=="distortion" || type=="fuzz") return juce::Colour(0xffdc7848);
    if(type=="delay") return juce::Colour(0xff438dcc);
    if(type=="reverb" || type=="pitch") return juce::Colour(0xff966fcd);
    if(type=="chorus" || type=="tremolo" || type=="ring_mod") return juce::Colour(0xff35aaa7);
    if(type=="filter" || type=="eq") return juce::Colour(0xffd2ad52);
    return juce::Colour(0xff8998ac);
}
}
class RackEditor::Card final : public juce::Component {
public:
    Card(RackEngine& owner, PresetSession& session, const RackInstance& instance, int position, int count, std::uint64_t& revision,bool open,std::function<void(bool)> change)
        : rack(owner), presets(session), identity(instance), colour(pedalColour(instance.effectType)),
          store(userDataDirectory()/"pedals"),libraryRevision(revision),expanded(open),onExpansion(std::move(change)) {
        descriptor=rack.registry().find(identity.effectType,identity.effectVersion);
        title.setText(juce::String(position+1)+". "+descriptor->displayName,juce::dontSendNotification);
        title.setJustificationType(juce::Justification::centred);
        title.setColour(juce::Label::textColourId,colour.brighter());
        title.setTooltip("Move this pedal to change the signal chain. Settings and identity stay with it.");
        for(auto* c:std::initializer_list<juce::Component*>{&title,&up,&down,&remove,&bypass,&reset,&voices,&saveVoice,&expand,&tap}) addAndMakeVisible(c);
        expand.setButtonText(expanded?"Basic controls":"All controls");
        expand.onClick=[this]{expanded=!expanded;expand.setButtonText(expanded?"Basic controls":"All controls");onExpansion(expanded);};
        // Include Drive/Gain and Level/Output in every compact pedal.
        for(std::size_t i=0;i<descriptor->parameters.size();++i) {
            const auto& p=descriptor->parameters[i];
            if(i<2 || p.key=="level_db" || p.key=="output_db" || p.key=="drive_db" || p.key=="gain_db" || p.key=="gain_b_db" || p.key=="expression_percent") basic.push_back(i);
        }
        tap.setVisible(identity.effectType=="echo");
        tap.onClick=[this]{const double now=juce::Time::getMillisecondCounterHiRes();if(lastTap>0&&now-lastTap>=250&&now-lastTap<=1500){std::vector<float> values;for(const auto& p:descriptor->parameters){float value=p.denormalize(rack.bank.value(identity.slot,p.bankIndex));if(p.key=="bpm")value=static_cast<float>(60000/(now-lastTap));if(p.key=="clock")value=1;values.push_back(value);}rack.setParameters(identity.id,values);}lastTap=now;};
        up.setEnabled(position>0); down.setEnabled(position+1<count);
        up.onClick=[this]{rack.move(identity.id,-1);}; down.onClick=[this]{rack.move(identity.id,1);};
        remove.onClick=[this]{rack.remove(identity.id);};
        up.setTooltip("Move earlier in the chain."); down.setTooltip("Move later in the chain.");
        remove.setTooltip("Remove this pedal. Structural edits can shorten temporal tails.");
        bypass.setClickingTogglesState(true);
        bypass.setTooltip("Footswitch: engage or bypass this pedal with a smooth fade. Bypass hides tails while processing continues.");
        bypass.onClick=[this]{rack.setBypassed(identity.id,!bypass.getToggleState()); repaint();};
        reset.onClick=[this]{
            std::vector<float> values; for(const auto& p:descriptor->parameters) values.push_back(p.defaultValue);
            if(rack.setParameters(identity.id,values)) { voices.setSelectedId(0,juce::dontSendNotification); poll(); }
        };
        reset.setTooltip("Restore conservative controls; keep bypass, IR and temporal history.");
        voices.setTextWhenNothingSelected("Custom / defaults");
        voices.setTooltip("Apply a voicing to this pedal only. Order, bypass, selected IR and other pedals stay.");
        refreshVoices();
        voices.onChange=[this]{
            const int i=voices.getSelectedItemIndex();
            if(i>=0 && static_cast<std::size_t>(i)<voicings.size()) {
                if(!rack.setParameters(identity.id,voicings[static_cast<std::size_t>(i)].values))
                    presets.error="Wait for the rack transition to finish before applying a pedal voicing.";
                poll();
            }
        };
        saveVoice.onClick=[this]{saveVoicing();};
        saveVoice.setTooltip("Save these controls as a reusable pedal voicing. Cabinet IR selection belongs to the full rack preset.");
        for(const auto& p:descriptor->parameters) {
            auto control=std::make_unique<Control>(); control->parameter=p;
            control->label.setText(p.label,juce::dontSendNotification);
            control->label.setJustificationType(juce::Justification::centred);
            addAndMakeVisible(control->label);
            if(!p.choices.empty()) {
                for(std::size_t j=0;j<p.choices.size();++j) control->mode.addItem(p.choices[j],static_cast<int>(j+1));
                control->mode.onChange=[this,item=control.get()]{
                    const int value=item->mode.getSelectedItemIndex();
                    if(value>=0) rack.setParameter(identity.id,item->parameter.bankIndex,static_cast<float>(value));
                    voices.setSelectedId(0,juce::dontSendNotification);
                };
                addAndMakeVisible(control->mode);
            } else {
                auto& slider=control->slider;
                slider.setRange(p.minimum,p.maximum,.01);
                slider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
                slider.setTextBoxStyle(juce::Slider::TextBoxBelow,false,92,22);
                slider.setTextValueSuffix(" "+p.units);
                slider.setDoubleClickReturnValue(true,p.defaultValue);
                slider.setColour(juce::Slider::rotarySliderFillColourId,colour);
                if(p.minimum>0 && p.maximum/p.minimum>20)
                    slider.setSkewFactorFromMidPoint(std::sqrt(p.minimum*p.maximum));
                slider.setTooltip(juce::String(p.label)+": "+juce::String(p.minimum)+" to "+juce::String(p.maximum)+" "+p.units+". Numeric entry; double-click resets.");
                slider.onValueChange=[this,item=control.get()]{
                    rack.setParameter(identity.id,item->parameter.bankIndex,static_cast<float>(item->slider.getValue()));
                    voices.setSelectedId(0,juce::dontSendNotification);
                };
                addAndMakeVisible(slider);
            }
            controls.push_back(std::move(control));
        }
        if(identity.effectType=="cabinet") {
            addAndMakeVisible(loadIR); addAndMakeVisible(assetStatus);addAndMakeVisible(cabinetChoices);
            cabinetChoices.addItem("Original synthetic (legacy)",1);int choice=2;
            for(const auto& c:builtinCabinets())cabinetChoices.addItem("Synthetic: "+juce::String(c.name),choice++);
            int selected=identity.irAsset==syntheticIRId()?1:0;
            for(std::size_t i=0;i<builtinCabinets().size();++i)if(builtinCabinets()[i].id==identity.irAsset)selected=int(i)+2;
            cabinetChoices.setTextWhenNothingSelected("User IR");cabinetChoices.setSelectedId(selected,juce::dontSendNotification);
            cabinetChoices.onChange=[this]{const int choice=cabinetChoices.getSelectedId();if(choice>0)presets.selectBuiltinIR(identity.id,choice==1?syntheticIRId():builtinCabinets()[std::size_t(choice-2)].id);};
            loadIR.onClick=[this]{
                chooser=std::make_unique<juce::FileChooser>("Load mono cabinet IR (up to 500 ms)",juce::File{},"*.wav");
                chooser->launchAsync(juce::FileBrowserComponent::openMode|juce::FileBrowserComponent::canSelectFiles,
                    [safe=juce::Component::SafePointer<Card>(this)](const juce::FileChooser& dialog){
                        if(!safe) return;
                        const auto file=dialog.getResult();
                        if(file!=juce::File{}) safe->presets.loadIR(safe->identity.id,file.getFullPathName().toStdString());
                    });
            };
            assetStatus.setText(identity.irAsset==syntheticIRId()?"Synthetic cabinet filter":"User IR: "+identity.irAsset.substr(0,12),juce::dontSendNotification);
            for(const auto& c:builtinCabinets()) if(c.id==identity.irAsset) assetStatus.setText("Synthetic: "+juce::String(c.name),juce::dontSendNotification);
            assetStatus.setTooltip("Cabinet is linear convolution. Choose a measured IR for your preferred cabinet tone.");
        }
        poll();
    }
    const InstanceId& instanceId() const noexcept {return identity.id;}
    int preferredHeight() const { return 146+static_cast<int>(((expanded?controls.size():basic.size())+1)/2)*64+(identity.effectType=="cabinet"?78:0); }
    void poll() {
        if(seenRevision!=libraryRevision) { refreshVoices(); seenRevision=libraryRevision; }
        const bool engaged=!rack.bank.bypassed(identity.slot);
        bypass.setToggleState(engaged,juce::dontSendNotification);
        bypass.setButtonText(engaged?"ENGAGED":"BYPASSED");
        bypass.setColour(juce::TextButton::buttonOnColourId,colour.darker(.3f));
        for(auto& c:controls) {
            const float value=c->parameter.denormalize(rack.bank.value(identity.slot,c->parameter.bankIndex));
            if(!c->parameter.choices.empty()) c->mode.setSelectedItemIndex(static_cast<int>(std::round(value)),juce::dontSendNotification);
            else if(!c->slider.isMouseButtonDown() && !c->slider.hasKeyboardFocus(true)) c->slider.setValue(value,juce::dontSendNotification);
        }
        const int selected=voices.getSelectedItemIndex();
        if(selected>=0 && static_cast<std::size_t>(selected)<voicings.size()) {
            const auto& p=voicings[static_cast<std::size_t>(selected)];
            for(std::size_t i=0;i<p.values.size();++i)
                if(std::abs(p.values[i]-descriptor->parameters[i].denormalize(rack.bank.value(identity.slot,descriptor->parameters[i].bankIndex)))>.02f) {
                    voices.setSelectedId(0,juce::dontSendNotification); break;
                }
        }
        if(voices.getSelectedItemIndex()<0) {
            for(std::size_t candidate=0;candidate<voicings.size();++candidate) {
                const auto& voice=voicings[candidate];bool matches=true;
                for(std::size_t i=0;i<voice.values.size();++i)
                    if(std::abs(voice.values[i]-descriptor->parameters[i].denormalize(rack.bank.value(identity.slot,descriptor->parameters[i].bankIndex)))>.02f){matches=false;break;}
                if(matches){voices.setSelectedItemIndex(int(candidate),juce::dontSendNotification);break;}
            }
        }
        saveVoice.setEnabled(!presets.busy() && !dialogOpen);
        repaint();
    }
    void resized() override {
        auto area=getLocalBounds().reduced(10);
        auto footer=area.removeFromBottom(26);
        saveVoice.setBounds(footer.removeFromTop(24));
        title.setBounds(area.removeFromTop(26));
        auto actions=area.removeFromTop(22); const int width=actions.getWidth()/4;
        up.setBounds(actions.removeFromLeft(width).reduced(2)); down.setBounds(actions.removeFromLeft(width).reduced(2));
        reset.setBounds(actions.removeFromLeft(width).reduced(2)); remove.setBounds(actions.reduced(2));
        area.removeFromTop(4); voices.setBounds(area.removeFromTop(26));auto detail=area.removeFromTop(22);bypass.setBounds(detail.removeFromRight(90));if(identity.effectType=="echo")tap.setBounds(detail.removeFromRight(75));expand.setBounds(detail);
        for(auto& c:controls) {c->label.setVisible(false);c->slider.setVisible(false);c->mode.setVisible(false);}
        std::size_t shown=0;
        juce::Rectangle<int> row;
        for(std::size_t i=0;i<controls.size();++i) {
            if(!expanded && std::find(basic.begin(),basic.end(),i)==basic.end())continue;
            if(shown%2==0) row=area.removeFromTop(64);
            auto cell=(shown%2==0?row.removeFromLeft(row.getWidth()/2):row).reduced(3);++shown;
            auto& c=controls[i];c->label.setVisible(true);c->label.setBounds(cell.removeFromTop(18));
            if(c->parameter.choices.empty()) {c->slider.setVisible(true);c->slider.setBounds(cell);}
            else {c->mode.setVisible(true);c->mode.setBounds(cell.removeFromTop(28));}
        }
        if(identity.effectType=="cabinet") {
            cabinetChoices.setBounds(area.removeFromTop(28));loadIR.setBounds(area.removeFromTop(26)); assetStatus.setBounds(area.removeFromTop(24));
        }

    }
    void paint(juce::Graphics& g) override {
        const auto bounds=getLocalBounds().toFloat().reduced(1);
        g.setColour(juce::Colour(0xff17212d)); g.fillRoundedRectangle(bounds,12);
        g.setColour(colour.withAlpha(.65f)); g.drawRoundedRectangle(bounds,12,2);
        const auto engaged=!rack.bank.bypassed(identity.slot);
        g.setColour(engaged?colour.brighter():juce::Colour(0xff394552));
        g.fillEllipse(static_cast<float>(getWidth()/2-4),static_cast<float>(getHeight()-14),8,8);
    }
private:
    std::vector<std::size_t> basic;
    juce::TextButton expand,tap{"Tap BPM"};
    double lastTap=0;
    void refreshVoices() {
        std::string selected;
        const int old=voices.getSelectedItemIndex();
        if(old>=0 && static_cast<std::size_t>(old)<voicings.size()) selected=voicings[static_cast<std::size_t>(old)].id;
        voicings.clear(); voices.clear(juce::dontSendNotification);
        auto add=[&](const auto& candidates){for(const auto& p:candidates)
            if(p.effectType==identity.effectType && p.effectVersion==identity.effectVersion) voicings.push_back(p);};
        add(effectPresets(rack.registry()));
        std::vector<std::string> errors; add(store.list(rack.registry(),errors));
        if(!errors.empty()) presets.error=errors.front();
        for(std::size_t j=0;j<voicings.size();++j) {
            voices.addItem(juce::String::fromUTF8(voicings[j].name.c_str()),static_cast<int>(j+1));
            if(voicings[j].id==selected) voices.setSelectedId(static_cast<int>(j+1),juce::dontSendNotification);
        }
    }
    void saveVoicing() {
        if(presets.busy() || dialogOpen) return;
        std::vector<float> values;
        for(const auto& p:descriptor->parameters) values.push_back(p.denormalize(rack.bank.value(identity.slot,p.bankIndex)));
        EffectPreset voice{newInstanceId(),"",identity.effectType,identity.effectVersion,std::move(values)};
        auto* alert=new juce::AlertWindow("Save pedal voicing","Name these controls. Bypass, order and Cabinet IR stay in the full rack preset.",juce::MessageBoxIconType::NoIcon);
        alert->addTextEditor("name","My "+descriptor->displayName);
        alert->addButton("Save",1,juce::KeyPress(juce::KeyPress::returnKey));
        alert->addButton("Cancel",0,juce::KeyPress(juce::KeyPress::escapeKey)); dialogOpen=true;
        alert->enterModalState(true,juce::ModalCallbackFunction::create([safe=juce::Component::SafePointer<Card>(this),alert,voice](int result) mutable {
            if(!safe) return;
            safe->dialogOpen=false;
            if(result!=1) return;
            try {
                voice.name=alert->getTextEditorContents("name").toStdString(); safe->store.save(voice,safe->rack.registry());
                ++safe->libraryRevision; safe->refreshVoices(); safe->seenRevision=safe->libraryRevision;
                for(std::size_t j=0;j<safe->voicings.size();++j) if(safe->voicings[j].id==voice.id)
                    safe->voices.setSelectedId(static_cast<int>(j+1),juce::dontSendNotification);
            } catch(const std::exception& e) {safe->presets.error=e.what();}
        }),true);
    }
    struct Control {ParameterDescriptor parameter; juce::Label label; juce::Slider slider; juce::ComboBox mode;};
    RackEngine& rack;
    PresetSession& presets;
    RackInstance identity;
    const EffectDescriptor* descriptor=nullptr;
    juce::Colour colour;
    PedalPresetStore store;
    std::vector<EffectPreset> voicings;
    std::uint64_t& libraryRevision;
    bool expanded=false;
    std::function<void(bool)> onExpansion;
    juce::ComboBox cabinetChoices;
    std::uint64_t seenRevision=0;
    bool dialogOpen=false;
    juce::ComboBox voices;
    juce::TextButton saveVoice{"Save voicing"},loadIR{"Load IR..."};
    juce::Label assetStatus,title;
    std::unique_ptr<juce::FileChooser> chooser;
    juce::TextButton up{"Earlier"},down{"Later"},remove{"Remove"},bypass{"ENGAGED"},reset{"Reset"};
    std::vector<std::unique_ptr<Control>> controls;
};
void RackEditor::Board::paint(juce::Graphics& g) {
    g.setColour(juce::Colour(0xff718299));
    for(int i=1;i<getNumChildComponents();++i) {
        const auto a=getChildComponent(i-1)->getBounds(), b=getChildComponent(i)->getBounds();
        juce::Path wire;
        if(a.getY()==b.getY()) {
            const float y=static_cast<float>(a.getY()+52);
            wire.startNewSubPath(static_cast<float>(a.getRight()),y);
            wire.lineTo(static_cast<float>(b.getX()),y);
            g.strokePath(wire,juce::PathStrokeType(2));
            g.drawArrow(juce::Line<float>(static_cast<float>(a.getRight()),y,static_cast<float>(b.getX()),y),2,5,5);
        } else {
            const float y=static_cast<float>(a.getBottom()+12);
            wire.startNewSubPath(static_cast<float>(a.getCentreX()),static_cast<float>(a.getBottom()));
            wire.lineTo(static_cast<float>(a.getCentreX()),y);
            wire.lineTo(static_cast<float>(b.getCentreX()),y);
            wire.lineTo(static_cast<float>(b.getCentreX()),static_cast<float>(b.getY()));
            g.strokePath(wire,juce::PathStrokeType(2));
        }
    }
    if(getNumChildComponents()==0) {
        g.setColour(juce::Colour(0xff9aa8ba));
        g.drawFittedText("Your pedalboard is empty. Add an effect or choose a complete rack preset.",getLocalBounds().reduced(16),juce::Justification::centredTop,2);
    }
}
RackEditor::RackEditor(RackEngine& owner,PresetSession& session) : rack(owner),presets(session) {
    for (auto* button : {&addEffect, &bypass}) addAndMakeVisible(button);
    addAndMakeVisible(picker); addAndMakeVisible(status); addAndMakeVisible(viewport);
    viewport.setViewedComponent(&content, false);
    viewport.setScrollBarsShown(true, false);
    int id = 1;
    for (const auto& effect : rack.registry().descriptors())
        if(rack.registry().latest(effect.typeId)==&effect) { picker.addItem(effect.displayName,id++); pickerTypes.push_back(effect.typeId); }
    picker.setSelectedId(1);
    picker.setTooltip("Choose an implemented effect from the catalog. Each addition has independent controls and a stable slot.");
    addEffect.setTooltip("Add the selected effect with conservative defaults. Maximum 16 stable slots.");
    addEffect.onClick = [this] {
        const auto index = picker.getSelectedItemIndex();
        if (index >= 0) rack.add(pickerTypes[static_cast<std::size_t>(index)]);
    };
    bypass.setClickingTogglesState(true);
    bypass.setTooltip("Bypass with a 20 ms ramp. Effects keep processing; tails are hidden. Gain, protection, meters and Mute remain active.");
    bypass.onClick = [this] { rack.bank.setRackBypassed(bypass.getToggleState()); };
    poll();
}
RackEditor::~RackEditor() { viewport.setViewedComponent(nullptr, false); }
void RackEditor::poll() {
    const auto state = rack.snapshot();
    if (state.requested != displayed) {
        const int scroll=std::max(restoredScroll,viewport.getViewPositionY());
        std::string anchor;int offset=0;
        for(const auto& card:cards)if(card->getY()+card->getHeight()>scroll){anchor=card->instanceId();offset=scroll-card->getY();break;}
        cards.clear(); displayed = state.requested;
        for (std::size_t i = 0; i < displayed.order.size(); ++i) {
            auto card = std::make_unique<Card>(rack, presets, *displayed.find(displayed.order[i]), static_cast<int>(i), static_cast<int>(displayed.order.size()),voicingRevision,expanded.contains(displayed.order[i]),
                [this,id=displayed.order[i]](bool open){if(open){if(expanded.size()>=512&&!expanded.contains(id))expanded.erase(expanded.begin());expanded.insert(id);}else expanded.erase(id);const int y=viewport.getViewPositionY();layoutCards();viewport.setViewPosition(0,y);});
            content.addAndMakeVisible(*card); cards.push_back(std::move(card));
        }
        layoutCards();int target=scroll;for(const auto& card:cards)if(card->instanceId()==anchor)target=card->getY()+offset;viewport.setViewPosition(0,target);restoredScroll=0;
    }
    // Full tone controls install at activation; keep the prior visible controls
    // until the matching rack/selection commit acknowledgment is published.
    if (!state.tonePending && !state.transitioning) {
        for (auto& card : cards) card->poll();
        bypass.setToggleState(rack.bank.rackBypassed(), juce::dontSendNotification);
    }
    addEffect.setEnabled(displayed.instances.size() < rackSlots);
    const auto text = juce::String(displayed.instances.size()) + "/16 pedals | Signal flows by number | "
        + (state.error.empty() ? (state.requestedGeneration == state.committedGeneration ? "Applied" : "Preparing / transitioning...") : juce::String(state.error));
    status.setText(text, juce::dontSendNotification);
}
void RackEditor::layoutCards() {
    const int available=std::max(1,viewport.getWidth()-viewport.getScrollBarThickness());
    const int columns=std::max(1,available/250), gap=14, width=(available-gap*(columns-1))/columns;
    int y=0;
    for(std::size_t start=0;start<cards.size();start+=static_cast<std::size_t>(columns)) {
        int height=0;
        for(int column=0;column<columns && start+column<cards.size();++column)
            height=std::max(height,cards[start+column]->preferredHeight());
        for(int column=0;column<columns && start+column<cards.size();++column)
            cards[start+column]->setBounds(column*(width+gap),y,width,height);
        y+=height+24;
    }
    content.setSize(available,std::max(1,y)); content.repaint();
}
void RackEditor::restoreLocal(const Preferences& p) {expanded={p.expanded.begin(),p.expanded.end()};restoredScroll=p.boardScroll;displayed={};poll();}
void RackEditor::captureLocal(Preferences& p) const {p.expanded.assign(expanded.begin(),expanded.end());p.boardScroll=viewport.getViewPositionY();}
void RackEditor::resized() {
    auto area = getLocalBounds(); auto bar = area.removeFromTop(34);
    picker.setBounds(bar.removeFromLeft(230)); bar.removeFromLeft(6);
    addEffect.setBounds(bar.removeFromLeft(110)); bar.removeFromLeft(10);
    bypass.setBounds(bar.removeFromLeft(120)); bar.removeFromLeft(12); status.setBounds(bar);
    area.removeFromTop(6); viewport.setBounds(area); layoutCards();
}
}

