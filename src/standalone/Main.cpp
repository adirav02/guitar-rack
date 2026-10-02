#include <juce_gui_basics/juce_gui_basics.h>
#include "ui/MainComponent.h"
#include "ui/StudioTheme.h"

namespace guitarrack {
class Application final : public juce::JUCEApplication, private juce::Timer {
public:
    const juce::String getApplicationName() override { return "GuitarRack"; }
    const juce::String getApplicationVersion() override { return "0.1.0"; }
    bool moreThanOneInstanceAllowed() override { return false; }
    void initialise(const juce::String& commandLine) override {
        juce::LookAndFeel::setDefaultLookAndFeel(&theme);
        window = std::make_unique<Window>();
        // Launch/close smoke check, with the exact same no-monitoring startup.
        if (commandLine.contains("--smoke-test")) startTimer(1200);
    }
    void shutdown() override { stopTimer(); window.reset(); juce::LookAndFeel::setDefaultLookAndFeel(nullptr); }
    void systemRequestedQuit() override { if (window) static_cast<MainComponent*>(window->getContentComponent())->requestClose([this] { quit(); }); else quit(); }
    void anotherInstanceStarted(const juce::String&) override { if (window) window->toFront(true); }
private:
    void timerCallback() override { stopTimer(); quit(); }
    class Window final : public juce::DocumentWindow {
    public:
        Window() : DocumentWindow("GuitarRack", juce::Colour(0xff202b3a), allButtons) {
            setUsingNativeTitleBar(true);
            setContentOwned(new MainComponent(), true);
            setResizable(true, false); setResizeLimits(700, 450, 7680, 4320);
            centreWithSize(getWidth(), getHeight());
            if(auto* display=juce::Desktop::getInstance().getDisplays().getPrimaryDisplay()) {
                auto available=display->userBounds.toNearestInt();
                setBounds(getBounds().withSizeKeepingCentre(std::min(getWidth(),available.getWidth()),std::min(getHeight(),available.getHeight())).constrainedWithin(available));
            }
            setVisible(true);
        }
        void closeButtonPressed() override { juce::JUCEApplication::getInstance()->systemRequestedQuit(); }
    };
    StudioTheme theme;
    std::unique_ptr<Window> window;
};
}
START_JUCE_APPLICATION(guitarrack::Application)
