#pragma once
#include <juce_gui_basics/juce_gui_basics.h>
#include "core/RackEngine.h"
#include "state/PresetSession.h"

namespace guitarrack {
class RackEditor final : public juce::Component {
public:
    RackEditor(RackEngine&,PresetSession&);
    ~RackEditor() override;
    void poll();
    void resized() override;
private:
    class Card;
    RackEngine& rack;
    PresetSession& presets;
    juce::ComboBox picker;
    std::vector<std::string> pickerTypes;
    juce::TextButton addEffect{"Add Effect"}, bypass{"Bypass rack"};
    juce::Label status;
    juce::Viewport viewport;
    class Board final : public juce::Component { public: void paint(juce::Graphics&) override; };
    Board content;
    std::vector<std::unique_ptr<Card>> cards;
    RackDefinition displayed;
    std::uint64_t voicingRevision=0;
    void layoutCards();
};
}

