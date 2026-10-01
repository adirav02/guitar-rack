#include "Preferences.h"
#include <cmath>
#include <stdexcept>
namespace guitarrack {
int exactDeviceIndex(const juce::StringArray& devices, const std::string& name) {
    return name.empty() ? -1 : devices.indexOf(juce::String::fromUTF8(name.c_str()));
}
Preferences parsePreferences(const std::string& text) {
    auto v=parseJson(text); auto* o=v.getDynamicObject();
    if(!o) throw std::runtime_error("Invalid local preferences.");
    auto n=[&](const char* key,double lo,double hi,bool whole=true) { auto x=v[key]; if(!x.isInt()&&!x.isInt64()&&!x.isDouble()) throw std::runtime_error("Invalid preference number."); double value=static_cast<double>(x); if(!std::isfinite(value)||value<lo||value>hi||(whole&&std::floor(value)!=value)) throw std::runtime_error("Preference out of range."); return value; };
    auto s=[&](const char* key) { if(!v[key].isString()||v[key].toString().getNumBytesAsUTF8()>256) throw std::runtime_error("Invalid preference string."); return v[key].toString().toStdString(); };
    const int version=static_cast<int>(n("settings_version",1,2));
    if(o->getProperties().size()!=(version==1?11:24)) throw std::runtime_error("Unknown/missing local preference fields.");
    Preferences p; p.backend=s("backend"); p.inputDevice=s("input_device"); p.outputDevice=s("output_device"); p.input=static_cast<int>(n("input",-1,255)); p.left=static_cast<int>(n("left",-1,255)); p.right=static_cast<int>(n("right",-1,255)); p.rate=n("rate",8000,768000,false); p.buffer=static_cast<int>(n("buffer",1,65536)); p.width=static_cast<int>(n("width",version==1?850:700,version==1?1600:7680)); p.height=static_cast<int>(n("height",version==1?1030:450,version==1?1400:4320));
    if(version==1) p.height=std::min(p.height,768);
    else {
        if(!v["audio_collapsed"].isBool()) throw std::runtime_error("Invalid layout preference.");
        p.audioCollapsed=static_cast<bool>(v["audio_collapsed"]);
        p.browserX=static_cast<int>(n("browser_x",-7680,7680)); p.browserY=static_cast<int>(n("browser_y",-4320,4320));
        p.browserWidth=static_cast<int>(n("browser_width",700,7680)); p.browserHeight=static_cast<int>(n("browser_height",400,4320));
        p.boardScroll=static_cast<int>(n("board_scroll",0,100000));
        p.latencyProfile=static_cast<int>(n("latency_profile",128,4096));
        if(p.latencyProfile!=128&&p.latencyProfile!=4096)throw std::runtime_error("Unknown latency profile.");
        p.search=s("search"); p.source=s("source"); p.artist=s("artist"); p.category=s("category");
        auto list=[&](const char* key,std::size_t max) {std::vector<std::string> result; auto* a=v[key].getArray(); if(!a||static_cast<std::size_t>(a->size())>max) throw std::runtime_error("Invalid UUID list."); for(const auto& x:*a) {if(!x.isString()||!validInstanceId(x.toString().toStdString())) throw std::runtime_error("Invalid UUID preference."); auto id=x.toString().toStdString(); if(std::find(result.begin(),result.end(),id)!=result.end()) throw std::runtime_error("Duplicate UUID preference."); result.push_back(id);} return result;};
        p.favorites=list("favorites",512); p.expanded=list("expanded",512);
    }
    return p;
}
std::string serializePreferences(const Preferences& p) {
    juce::var v(new juce::DynamicObject()); auto* o=v.getDynamicObject();
    o->setProperty("settings_version",2); o->setProperty("backend",juce::String(p.backend)); o->setProperty("input_device",juce::String(p.inputDevice)); o->setProperty("output_device",juce::String(p.outputDevice)); o->setProperty("input",p.input); o->setProperty("left",p.left); o->setProperty("right",p.right); o->setProperty("rate",p.rate); o->setProperty("buffer",p.buffer); o->setProperty("width",p.width); o->setProperty("height",p.height);
    o->setProperty("audio_collapsed",p.audioCollapsed); o->setProperty("browser_x",p.browserX); o->setProperty("browser_y",p.browserY); o->setProperty("browser_width",p.browserWidth); o->setProperty("browser_height",p.browserHeight); o->setProperty("board_scroll",p.boardScroll); o->setProperty("latency_profile",p.latencyProfile);
    o->setProperty("search",juce::String(p.search)); o->setProperty("source",juce::String(p.source)); o->setProperty("artist",juce::String(p.artist)); o->setProperty("category",juce::String(p.category));
    auto list=[&](const char* key,const auto& values) {juce::Array<juce::var> a; for(const auto& x:values) a.add(juce::String(x)); o->setProperty(key,a);};
    list("favorites",p.favorites); list("expanded",p.expanded);
    auto text=juce::JSON::toString(v).toStdString()+"\n"; parsePreferences(text); return text;
}
}
