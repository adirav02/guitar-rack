#pragma once
#include <juce_gui_basics/juce_gui_basics.h>
#include <cmath>

namespace guitarrack {
// Presentation only. One application-owned look and feel also styles detached
// preset windows, popup menus and alerts; it outlives all application windows.
namespace studio {
inline const juce::Colour background{0xff10141c}, panel{0xff1b2230}, inset{0xff121923};
inline const juce::Colour border{0xff3b485c}, text{0xfff1f3f8}, muted{0xffadb9cc};
inline const juce::Colour accent{0xff9ce8c3}, amber{0xffffcb80}, danger{0xffff9a9a};
inline void primary(juce::TextButton& button) {
    button.setColour(juce::TextButton::buttonColourId, accent);
    button.setColour(juce::TextButton::textColourOffId, background);
}
inline void surface(juce::Graphics& g, juce::Rectangle<float> bounds, float radius=12.0f) {
    g.setColour(panel); g.fillRoundedRectangle(bounds, radius);
    g.setColour(border.withAlpha(.6f)); g.drawRoundedRectangle(bounds.reduced(.5f), radius, 1.0f);
}
}

class StudioTheme final : public juce::LookAndFeel_V4 {
public:
    StudioTheme() {
        using namespace studio;
        setColour(juce::ResizableWindow::backgroundColourId, background);
        setColour(juce::Label::textColourId, text);
        setColour(juce::TextButton::buttonColourId, panel.brighter(.13f));
        setColour(juce::TextButton::buttonOnColourId, juce::Colour(0xff305d4c));
        setColour(juce::TextButton::textColourOffId, text);
        setColour(juce::TextButton::textColourOnId, text);
        setColour(juce::ComboBox::backgroundColourId, inset);
        setColour(juce::ComboBox::outlineColourId, border);
        setColour(juce::ComboBox::textColourId, text);
        setColour(juce::ComboBox::arrowColourId, muted);
        setColour(juce::ComboBox::focusedOutlineColourId, accent);
        setColour(juce::PopupMenu::backgroundColourId, panel);
        setColour(juce::PopupMenu::textColourId, text);
        setColour(juce::PopupMenu::highlightedBackgroundColourId, juce::Colour(0xff305d4c));
        setColour(juce::PopupMenu::highlightedTextColourId, text);
        setColour(juce::PopupMenu::headerTextColourId, accent);
        setColour(juce::TextEditor::backgroundColourId, inset);
        setColour(juce::TextEditor::textColourId, text);
        setColour(juce::TextEditor::outlineColourId, border);
        setColour(juce::TextEditor::focusedOutlineColourId, accent);
        setColour(juce::TextEditor::highlightColourId, juce::Colour(0xff356753));
        setColour(juce::TextEditor::highlightedTextColourId, text);
        setColour(juce::CaretComponent::caretColourId, accent);
        setColour(juce::ListBox::backgroundColourId, background);
        setColour(juce::ListBox::outlineColourId, border);
        setColour(juce::Slider::rotarySliderFillColourId, accent);
        setColour(juce::Slider::rotarySliderOutlineColourId, border);
        setColour(juce::Slider::trackColourId, accent);
        setColour(juce::Slider::backgroundColourId, border);
        setColour(juce::Slider::thumbColourId, text);
        setColour(juce::Slider::textBoxTextColourId, text);
        setColour(juce::Slider::textBoxBackgroundColourId, inset);
        setColour(juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
        setColour(juce::Slider::textBoxHighlightColourId, juce::Colour(0xff356753));
        setColour(juce::ScrollBar::thumbColourId, border.brighter(.35f));
        setColour(juce::ScrollBar::backgroundColourId, juce::Colours::transparentBlack);
        setColour(juce::AlertWindow::backgroundColourId, panel);
        setColour(juce::AlertWindow::textColourId, text);
        setColour(juce::AlertWindow::outlineColourId, border);
        setColour(juce::TooltipWindow::backgroundColourId, panel);
        setColour(juce::TooltipWindow::textColourId, text);
        setColour(juce::TooltipWindow::outlineColourId, border);
    }
    juce::Font getTextButtonFont(juce::TextButton&, int height) override {
        return juce::FontOptions(juce::jmin(14.0f, float(height)*.6f), juce::Font::bold);
    }
    void drawButtonBackground(juce::Graphics& g, juce::Button& b, const juce::Colour& colour, bool hover, bool down) override {
        auto bounds=b.getLocalBounds().toFloat().reduced(1.0f);
        auto fill=colour;
        if (hover && b.isEnabled()) fill=fill.brighter(.12f);
        if (down) fill=fill.darker(.15f);
        if (!b.isEnabled()) fill=fill.interpolatedWith(studio::background,.65f);
        g.setColour(fill); g.fillRoundedRectangle(bounds,7.0f);
        g.setColour(b.hasKeyboardFocus(true)?studio::accent:studio::border.withAlpha(b.isEnabled()?.65f:.25f));
        g.drawRoundedRectangle(bounds,7.0f,b.hasKeyboardFocus(true)?2.0f:1.0f);
    }
    void drawComboBox(juce::Graphics& g, int width, int height, bool down, int, int, int, int, juce::ComboBox& box) override {
        const auto bounds=juce::Rectangle<float>(0,0,float(width),float(height)).reduced(.5f);
        g.setColour(box.findColour(juce::ComboBox::backgroundColourId)); g.fillRoundedRectangle(bounds,6);
        g.setColour(box.hasKeyboardFocus(true)?studio::accent:studio::border.withAlpha(box.isEnabled()?1.0f:.35f));
        g.drawRoundedRectangle(bounds,6,box.hasKeyboardFocus(true)?2.0f:1.0f);
        juce::Path arrow; const float x=float(width)-17, y=float(height)*.5f;
        arrow.startNewSubPath(x-4,y-2); arrow.lineTo(x,y+2); arrow.lineTo(x+4,y-2);
        g.setColour(studio::muted.withAlpha(box.isEnabled()?(down?1.0f:.8f):.3f));
        g.strokePath(arrow,juce::PathStrokeType(1.5f));
    }
    void drawRotarySlider(juce::Graphics& g,int x,int y,int width,int height,float position,float start,float end,juce::Slider& slider) override {
        const auto bounds=juce::Rectangle<float>(float(x),float(y),float(width),float(height)).reduced(5);
        const auto centre=bounds.getCentre(); const float radius=juce::jmin(bounds.getWidth(),bounds.getHeight())*.5f;
        if(radius<=1) return;
        const float angle=start+position*(end-start);
        auto colour=slider.findColour(juce::Slider::rotarySliderFillColourId);
        if(!slider.isEnabled()) colour=studio::muted.withAlpha(.35f);
        juce::Path track, fill;
        track.addCentredArc(centre.x,centre.y,radius,radius,0,start,end,true);
        fill.addCentredArc(centre.x,centre.y,radius,radius,0,start,angle,true);
        const juce::PathStrokeType stroke(3.0f,juce::PathStrokeType::curved,juce::PathStrokeType::rounded);
        g.setColour(studio::border); g.strokePath(track,stroke);
        g.setColour(colour); g.strokePath(fill,stroke);
        const auto knob=juce::Rectangle<float>(radius*1.5f,radius*1.5f).withCentre(centre);
        g.setColour(juce::Colours::black.withAlpha(.4f)); g.fillEllipse(knob.translated(0,2));
        g.setGradientFill(juce::ColourGradient(juce::Colour(0xff4b5667),knob.getTopLeft(),juce::Colour(0xff202937),knob.getBottomRight(),false));
        g.fillEllipse(knob);
        g.setColour(slider.hasKeyboardFocus(true)?studio::accent:studio::border.brighter(.3f)); g.drawEllipse(knob,1.0f);
        const auto tip=centre+juce::Point<float>(std::sin(angle),-std::cos(angle))*(radius*.59f);
        const auto base=centre+juce::Point<float>(std::sin(angle),-std::cos(angle))*(radius*.28f);
        g.setColour(studio::text); g.drawLine({base,tip},2.5f);
    }
};
}
