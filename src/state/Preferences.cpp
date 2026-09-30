#include "Preferences.h"
#include <cmath>
#include <stdexcept>
namespace guitarrack {
int exactDeviceIndex(const juce::StringArray& devices, const std::string& name) {
    return name.empty() ? -1 : devices.indexOf(juce::String::fromUTF8(name.c_str()));
}
Preferences parsePreferences(const std::string& text) {
    auto v=parseJson(text); auto* o=v.getDynamicObject();
    if(!o||o->getProperties().size()!=11) throw std::runtime_error("Invalid local preferences.");
    auto n=[&](const char* key,double lo,double hi,bool whole=true) { auto x=v[key]; if(!x.isInt()&&!x.isInt64()&&!x.isDouble()) throw std::runtime_error("Invalid preference number."); double value=static_cast<double>(x); if(!std::isfinite(value)||value<lo||value>hi||(whole&&std::floor(value)!=value)) throw std::runtime_error("Preference out of range."); return value; };
    auto s=[&](const char* key) { if(!v[key].isString()||v[key].toString().getNumBytesAsUTF8()>256) throw std::runtime_error("Invalid preference string."); return v[key].toString().toStdString(); };
    if(n("settings_version",1,1)!=1) throw std::runtime_error("Unsupported settings version.");
    Preferences p; p.backend=s("backend"); p.inputDevice=s("input_device"); p.outputDevice=s("output_device"); p.input=static_cast<int>(n("input",-1,255)); p.left=static_cast<int>(n("left",-1,255)); p.right=static_cast<int>(n("right",-1,255)); p.rate=n("rate",8000,768000,false); p.buffer=static_cast<int>(n("buffer",1,65536)); p.width=static_cast<int>(n("width",850,1600)); p.height=static_cast<int>(n("height",1030,1400)); return p;
}
std::string serializePreferences(const Preferences& p) {
    juce::var v(new juce::DynamicObject()); auto* o=v.getDynamicObject();
    o->setProperty("settings_version",1); o->setProperty("backend",juce::String(p.backend)); o->setProperty("input_device",juce::String(p.inputDevice)); o->setProperty("output_device",juce::String(p.outputDevice)); o->setProperty("input",p.input); o->setProperty("left",p.left); o->setProperty("right",p.right); o->setProperty("rate",p.rate); o->setProperty("buffer",p.buffer); o->setProperty("width",p.width); o->setProperty("height",p.height);
    auto text=juce::JSON::toString(v).toStdString()+"\n"; parsePreferences(text); return text;
}
}
