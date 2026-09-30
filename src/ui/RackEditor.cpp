#include "RackEditor.h"

namespace guitarrack {
class RackEditor::Card final : public juce::Component {
public:
    Card(RackEngine& owner, const RackInstance& instance, int position, int count)
        : rack(owner), identity(instance) {
        const auto* descriptor = rack.registry().find(identity.effectType, identity.effectVersion);
        const auto& p = descriptor->parameters.front();
        title.setText(juce::String(position + 1) + ". " + descriptor->displayName + " | Slot " + juce::String(identity.slot), juce::dontSendNotification);
        title.setTooltip("Persistent instance: " + identity.id + "\nMoving changes order only; its slot and controls stay fixed.");
        addAndMakeVisible(title);
        for (auto* button : {&up, &down, &remove, &bypass}) addAndMakeVisible(button);
        up.setEnabled(position > 0); down.setEnabled(position + 1 < count);
        up.setTooltip("Move this instance earlier in the processing order.");
        down.setTooltip("Move this instance later in the processing order.");
        remove.setTooltip("Remove this instance with a 20 ms structural crossfade.");
        bypass.setClickingTogglesState(true);
        bypass.setToggleState(rack.bank.bypassed(identity.slot), juce::dontSendNotification);
        bypass.setTooltip("Crossfade this effect to dry over 20 ms; monitoring remains active.");
        up.onClick = [this] { rack.move(identity.id, -1); };
        down.onClick = [this] { rack.move(identity.id, 1); };
        remove.onClick = [this] { rack.remove(identity.id); };
        bypass.onClick = [this] { rack.setBypassed(identity.id, bypass.getToggleState()); };
        level.setRange(p.minimum, p.maximum, 0.1);
        level.setSliderStyle(juce::Slider::LinearHorizontal);
        level.setTextBoxStyle(juce::Slider::TextBoxRight, false, 88, 26);
        level.setTextValueSuffix(" " + p.units);
        level.setDoubleClickReturnValue(true, p.defaultValue);
        level.setValue(p.denormalize(rack.bank.value(identity.slot, p.bankIndex)), juce::dontSendNotification);
        level.setTooltip("Gain Level: -60 to +24 dB, neutral 0 dB, smoothed over 20 ms. Double-click to reset.");
        level.onValueChange = [this, index = p.bankIndex] { rack.setParameter(identity.id, index, static_cast<float>(level.getValue())); };
        addAndMakeVisible(level);
    }
    void resized() override {
        auto area = getLocalBounds().reduced(8);
        auto top = area.removeFromTop(30);
        remove.setBounds(top.removeFromRight(76)); top.removeFromRight(6);
        down.setBounds(top.removeFromRight(64)); top.removeFromRight(6);
        up.setBounds(top.removeFromRight(54)); top.removeFromRight(6);
        bypass.setBounds(top.removeFromRight(88)); title.setBounds(top);
        level.setBounds(area.removeFromTop(34));
    }
    void paint(juce::Graphics& g) override {
        g.setColour(juce::Colour(0xff17212d)); g.fillRoundedRectangle(getLocalBounds().toFloat(), 6);
    }
private:
    RackEngine& rack;
    RackInstance identity;
    juce::Label title;
    juce::TextButton up{"Up"}, down{"Down"}, remove{"Remove"}, bypass{"Bypass"};
    juce::Slider level;
};
RackEditor::RackEditor(RackEngine& owner) : rack(owner) {
    for (auto* button : {&addGain, &bypass}) addAndMakeVisible(button);
    addAndMakeVisible(status); addAndMakeVisible(viewport);
    viewport.setViewedComponent(&content, false);
    viewport.setScrollBarsShown(true, false);
    addGain.setTooltip("Add an independent neutral Gain instance. Maximum 16 stable slots.");
    addGain.onClick = [this] { rack.add(); };
    bypass.setClickingTogglesState(true);
    bypass.setTooltip("Bypass all effects with a 20 ms ramp. Input/output gain, protection, meters and Mute remain active.");
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
    addGain.setEnabled(displayed.instances.size() < rackSlots);
    const auto text = juce::String(displayed.instances.size()) + "/16 slots | "
        + (state.error.empty() ? (state.requestedGeneration == state.committedGeneration ? "Applied" : "Preparing / transitioning...") : juce::String(state.error));
    status.setText(text, juce::dontSendNotification);
}
void RackEditor::layoutCards() {
    content.setSize(std::max(1, viewport.getWidth() - viewport.getScrollBarThickness()), std::max(1, static_cast<int>(cards.size()) * 84));
    for (std::size_t i = 0; i < cards.size(); ++i) cards[i]->setBounds(0, static_cast<int>(i) * 84, content.getWidth(), 78);
}
void RackEditor::resized() {
    auto area = getLocalBounds(); auto bar = area.removeFromTop(34);
    addGain.setBounds(bar.removeFromLeft(110)); bar.removeFromLeft(10);
    bypass.setBounds(bar.removeFromLeft(120)); bar.removeFromLeft(12); status.setBounds(bar);
    area.removeFromTop(6); viewport.setBounds(area); layoutCards();
}
}
