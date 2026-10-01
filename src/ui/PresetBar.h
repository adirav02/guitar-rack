#pragma once
#include <juce_gui_basics/juce_gui_basics.h>
#include "state/PresetSession.h"
#include "PresetBrowser.h"
namespace guitarrack {
class PresetBar final : public juce::Component {
public:
    explicit PresetBar(RackEngine&);
    ~PresetBar() override;
    void poll();
    void restoreLocal(const Preferences&);
    void captureLocal(Preferences&) const;
    void openBrowser();
    void resized() override;
    void guard(std::function<void()> action);
    void saveAs(std::function<void()> after = {});
    PresetSession presets;
private:
    const EffectRegistry& registry;
    Preferences browserPreferences;
    std::unique_ptr<PresetBrowser> browser;
    juce::TextButton chooseButton{"Choose Preset..."};
    juce::Label currentName;
    juce::TextButton saveButton{"Save"}, saveAsButton{"Save As"}, deleteButton{"Delete"}, importButton{"Import"}, exportButton{"Export"}, cancelButton{"Cancel load"};
    juce::Label indicator, status;
    std::unique_ptr<juce::FileChooser> chooser;
    std::vector<ToneState> displayed;
    bool dialogOpen=false;

    void fileDialog(bool importing);
    void chooseDirty(std::function<void(DirtyAction)>);
    void deletePreset();
    void confirmDelete(ToneState, DirtyAction);
};
}
