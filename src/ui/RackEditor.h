#pragma once
#include <juce_gui_basics/juce_gui_basics.h>
#include "core/RackEngine.h"
#include "state/PresetSession.h"
#include "state/Preferences.h"
#include <set>

namespace guitarrack {
class RackEditor final : public juce::Component {
public:
    RackEditor(RackEngine&,PresetSession&);
    ~RackEditor() override;
    void poll();
    void restoreLocal(const Preferences&);
    void captureLocal(Preferences&) const;
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
    std::set<std::string> expanded;
    int restoredScroll=0;
    void layoutCards();
};
}

