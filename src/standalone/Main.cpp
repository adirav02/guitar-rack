#include <juce_gui_basics/juce_gui_basics.h>
#include "ui/MainComponent.h"

namespace guitarrack {
class Application final : public juce::JUCEApplication, private juce::Timer {
public:
    const juce::String getApplicationName() override { return "GuitarRack"; }
    const juce::String getApplicationVersion() override { return "0.1.0"; }
    bool moreThanOneInstanceAllowed() override { return false; }
    void initialise(const juce::String& commandLine) override {
        window = std::make_unique<Window>();
        // Launch/close smoke check, with the exact same no-monitoring startup.
        if (commandLine.contains("--smoke-test")) startTimer(1200);
    }
    void shutdown() override { stopTimer(); window.reset(); }
    void systemRequestedQuit() override { quit(); }
    void anotherInstanceStarted(const juce::String&) override { if (window) window->toFront(true); }
private:
    void timerCallback() override { stopTimer(); quit(); }
    class Window final : public juce::DocumentWindow {
    public:
        Window() : DocumentWindow("GuitarRack", juce::Colour(0xff202b3a), allButtons) {
            setUsingNativeTitleBar(true);
            setContentOwned(new MainComponent(), true);
            setResizable(true, false); setResizeLimits(850, 950, 1600, 1400);
            centreWithSize(getWidth(), getHeight()); setVisible(true);
        }
        void closeButtonPressed() override { juce::JUCEApplication::getInstance()->systemRequestedQuit(); }
    };
    std::unique_ptr<Window> window;
};
}
START_JUCE_APPLICATION(guitarrack::Application)
