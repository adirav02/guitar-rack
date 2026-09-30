#include "RackEditor.h"

namespace guitarrack {
class RackEditor::Card final : public juce::Component {
public:
    Card(RackEngine& owner, const RackInstance& instance, int position, int count)
        : rack(owner), identity(instance) {
        const auto* descriptor = rack.registry().find(identity.effectType, identity.effectVersion);
        title.setText(juce::String(position + 1) + ". " + descriptor->displayName + " | Slot " + juce::String(identity.slot), juce::dontSendNotification);
        title.setTooltip("Persistent instance: " + identity.id + "\nMoving changes order only; its slot and controls stay fixed.");
        addAndMakeVisible(title);
        for (auto* button : {&up, &down, &remove, &bypass, &reset}) addAndMakeVisible(button);
        up.setEnabled(position > 0); down.setEnabled(position + 1 < count);
        up.setTooltip("Move this instance earlier in the processing order.");
        down.setTooltip("Move this instance later in the processing order.");
        remove.setTooltip("Remove with a 20 ms crossfade. Stateful tails may shorten during structural edits.");
        bypass.setClickingTogglesState(true);
        bypass.setTooltip("Crossfade to dry over 20 ms. The effect keeps processing; delay tails are hidden while bypassed.");
        up.onClick = [this] { rack.move(identity.id, -1); };
        down.onClick = [this] { rack.move(identity.id, 1); };
        remove.onClick = [this] { rack.remove(identity.id); };
        bypass.onClick = [this] { rack.setBypassed(identity.id, bypass.getToggleState()); };
        reset.setTooltip("Reset this effect's controls to conservative defaults. Does not clear delay history or change bypass.");
        reset.onClick = [this] {
            for (auto& control : controls) control->slider.setValue(control->parameter.defaultValue);
        };
        for (const auto& p : descriptor->parameters) {
            auto control = std::make_unique<Control>();
            control->parameter = p;
            auto& slider = control->slider;
            control->label.setText(p.label, juce::dontSendNotification);
            addAndMakeVisible(control->label);
            slider.setRange(p.minimum, p.maximum, 0.1);
            slider.setSliderStyle(juce::Slider::LinearHorizontal);
            slider.setTextBoxStyle(juce::Slider::TextBoxRight, false, 104, 26);
            slider.setTextValueSuffix(" " + p.units);
            slider.setDoubleClickReturnValue(true, p.defaultValue);
            slider.setValue(p.denormalize(rack.bank.value(identity.slot, p.bankIndex)), juce::dontSendNotification);
            slider.setTooltip(juce::String(p.label) + ": " + juce::String(p.minimum) + " to " + juce::String(p.maximum) + " " + p.units
                + ". Smooth changes; numeric entry; double-click to reset to " + juce::String(p.defaultValue) + " " + p.units + ".");
            slider.onValueChange = [this, item = control.get()] {
                rack.setParameter(identity.id, item->parameter.bankIndex, static_cast<float>(item->slider.getValue()));
            };
            addAndMakeVisible(slider); controls.push_back(std::move(control));
        }
        poll();
    }
    int preferredHeight() const { return 50 + static_cast<int>(controls.size()) * 34; }
    void poll() {
        bypass.setToggleState(rack.bank.bypassed(identity.slot), juce::dontSendNotification);
        for (auto& control : controls) {
            auto& slider = control->slider;
            if (!slider.isMouseButtonDown() && !slider.hasKeyboardFocus(true))
                slider.setValue(control->parameter.denormalize(rack.bank.value(identity.slot, control->parameter.bankIndex)), juce::dontSendNotification);
        }
    }
    void resized() override {
        auto area = getLocalBounds().reduced(8);
        auto top = area.removeFromTop(30);
        remove.setBounds(top.removeFromRight(76)); top.removeFromRight(6);
        down.setBounds(top.removeFromRight(64)); top.removeFromRight(6);
        up.setBounds(top.removeFromRight(54)); top.removeFromRight(6);
        reset.setBounds(top.removeFromRight(64)); top.removeFromRight(6);
        bypass.setBounds(top.removeFromRight(88)); title.setBounds(top);
        area.removeFromTop(4);
        for (auto& control : controls) {
            auto row = area.removeFromTop(34);
            control->label.setBounds(row.removeFromLeft(80)); control->slider.setBounds(row);
        }
    }
    void paint(juce::Graphics& g) override {
        g.setColour(juce::Colour(0xff17212d)); g.fillRoundedRectangle(getLocalBounds().toFloat(), 6);
    }
private:
    struct Control { ParameterDescriptor parameter; juce::Label label; juce::Slider slider; };
    RackEngine& rack;
    RackInstance identity;
    juce::Label title;
    juce::TextButton up{"Up"}, down{"Down"}, remove{"Remove"}, bypass{"Bypass"}, reset{"Reset"};
    std::vector<std::unique_ptr<Control>> controls;
};
RackEditor::RackEditor(RackEngine& owner) : rack(owner) {
    for (auto* button : {&addEffect, &bypass}) addAndMakeVisible(button);
    addAndMakeVisible(picker); addAndMakeVisible(status); addAndMakeVisible(viewport);
    viewport.setViewedComponent(&content, false);
    viewport.setScrollBarsShown(true, false);
    int id = 1;
    for (const auto& effect : rack.registry().descriptors()) picker.addItem(effect.displayName, id++);
    picker.setSelectedId(1);
    picker.setTooltip("Choose Gain, Overdrive, EQ or Delay. Each addition has independent controls and a stable slot.");
    addEffect.setTooltip("Add the selected effect with conservative defaults. Maximum 16 stable slots.");
    addEffect.onClick = [this] {
        const auto index = picker.getSelectedItemIndex();
        if (index >= 0) rack.add(rack.registry().descriptors()[static_cast<std::size_t>(index)].typeId);
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
        cards.clear(); displayed = state.requested;
        for (std::size_t i = 0; i < displayed.order.size(); ++i) {
            auto card = std::make_unique<Card>(rack, *displayed.find(displayed.order[i]), static_cast<int>(i), static_cast<int>(displayed.order.size()));
            content.addAndMakeVisible(*card); cards.push_back(std::move(card));
        }
        layoutCards();
    }
    for (auto& card : cards) card->poll();
    bypass.setToggleState(rack.bank.rackBypassed(), juce::dontSendNotification);
    addEffect.setEnabled(displayed.instances.size() < rackSlots);
    const auto text = juce::String(displayed.instances.size()) + "/16 slots | "
        + (state.error.empty() ? (state.requestedGeneration == state.committedGeneration ? "Applied" : "Preparing / transitioning...") : juce::String(state.error));
    status.setText(text, juce::dontSendNotification);
}
void RackEditor::layoutCards() {
    int height = 0;
    const int width = std::max(1, viewport.getWidth() - viewport.getScrollBarThickness());
    for (auto& card : cards) { card->setBounds(0, height, width, card->preferredHeight()); height += card->preferredHeight() + 6; }
    content.setSize(width, std::max(1, height));
}
void RackEditor::resized() {
    auto area = getLocalBounds(); auto bar = area.removeFromTop(34);
    picker.setBounds(bar.removeFromLeft(130)); bar.removeFromLeft(6);
    addEffect.setBounds(bar.removeFromLeft(110)); bar.removeFromLeft(10);
    bypass.setBounds(bar.removeFromLeft(120)); bar.removeFromLeft(12); status.setBounds(bar);
    area.removeFromTop(6); viewport.setBounds(area); layoutCards();
}
}
