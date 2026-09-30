#pragma once
#include <juce_gui_basics/juce_gui_basics.h>
#include "state/PresetSession.h"
namespace guitarrack {
class PresetBar final : public juce::Component {
public:
    explicit PresetBar(RackEngine&);
    void poll();
    void resized() override;
    void guard(std::function<void()> action);
    void saveAs(std::function<void()> after = {});
    PresetSession presets;
private:
    juce::ComboBox choices;
    juce::TextButton saveButton{"Save"}, saveAsButton{"Save As"}, importButton{"Import"}, exportButton{"Export"}, cancelButton{"Cancel load"};
    juce::Label indicator, status;
    std::unique_ptr<juce::FileChooser> chooser;
    std::vector<ToneState> displayed;
    bool dialogOpen=false;
    void refreshChoices();
    void fileDialog(bool importing);
};
}
