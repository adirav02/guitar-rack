#pragma once
#include <juce_gui_basics/juce_gui_basics.h>
#include "state/PresetSession.h"
#include "state/Preferences.h"
namespace guitarrack {
// Message-thread window, owned by PresetBar for its entire lifetime. Close hides;
// no self deletion, modal loop, DSP preparation or automatic audition.
class PresetBrowser final : public juce::DocumentWindow, private juce::ListBoxModel {
public:
    PresetBrowser(PresetSession&,const EffectRegistry&,std::function<void(ToneState)>,std::function<void()>,std::function<void()>,std::function<void()>);
    ~PresetBrowser() override;
    void closeButtonPressed() override;
    bool keyPressed(const juce::KeyPress&) override;
    void restore(const Preferences&);
    void capture(Preferences&) const;
    void poll();
private:
    class Content final : public juce::Component {
    public:
        explicit Content(PresetBrowser& b):browser(b) {}
        void resized() override;
        void paint(juce::Graphics& g) override {g.fillAll(juce::Colour(0xff202b3a));}
        PresetBrowser& browser;
    } content{*this};
    int getNumRows() override;
    void paintListBoxItem(int,juce::Graphics&,int,int,bool) override;
    void selectedRowsChanged(int) override;
    void listBoxItemDoubleClicked(int,const juce::MouseEvent&) override;
    void returnKeyPressed(int) override;
    void filter();
    void load();
    void details();
    PresetSession& session;
    const EffectRegistry& registry;
    std::function<void(ToneState)> onLoad;
    juce::TextEditor search,description;
    juce::ComboBox source,artist,category;
    juce::ListBox list{"Sounds",this};
    juce::TextButton loadButton{"Load"},favorite{"Favorite"},cancel{"Cancel load"},importButton{"Import"},exportButton{"Export"},deleteButton{"Delete user preset"};
    juce::Label status;
    std::vector<ToneState> catalog,filtered;
    std::vector<std::string> favorites,factoryIds;
    std::string highlighted;
};
}
