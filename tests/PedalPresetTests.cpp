#include "state/PedalPresets.h"
#include <iostream>
#include <limits>

using namespace guitarrack;
int main() {
    int failures=0;
    auto check=[&](bool ok,const char* label){if(!ok){++failures;std::cerr<<"FAIL: "<<label<<'\n';}};
    auto rejects=[&](auto f,const char* label){bool caught=false;try{f();}catch(const std::exception&){caught=true;}check(caught,label);};
    const auto registry=EffectRegistry::builtIns();
    const auto folder=std::filesystem::temp_directory_path()/("GuitarRack-pedals-"+newInstanceId());
    PedalPresetStore store(folder);
    auto pedals=effectPresets(registry);
    for(const auto& p:pedals) {
        auto roundtrip=parsePedalPreset(serializePedalPreset(p,registry),registry);
        check(roundtrip.id==p.id&&roundtrip.name==p.name&&roundtrip.effectVersion==p.effectVersion&&roundtrip.values==p.values,"factory pedal physical controls round trip exactly");
    }
    auto personal=pedals.front();personal.id=newInstanceId();personal.name="צליל אישי 🎸";store.save(personal,registry);
    std::vector<std::string> errors;auto loaded=store.list(registry,errors);
    check(errors.empty()&&loaded.size()==1&&loaded.front().name==personal.name&&loaded.front().values==personal.values,"Unicode personal pedal survives library reload");
    rejects([&]{store.save(pedals.front(),registry);},"factory pedal identity protected");
    auto invalid=personal;invalid.id="../escape";rejects([&]{store.save(invalid,registry);},"path traversal identity rejected");
    invalid=personal;invalid.values.clear();rejects([&]{store.save(invalid,registry);},"missing control rejected");
    invalid=personal;invalid.values[0]=std::numeric_limits<float>::infinity();rejects([&]{store.save(invalid,registry);},"nonfinite control rejected");
    auto text=serializePedalPreset(personal,registry);
    const auto at=text.find("\"schema_version\": 1");
    check(at!=std::string::npos,"schema serialized");
    if(at!=std::string::npos) {text.replace(at,19,"\"schema_version\": 9");rejects([&]{parsePedalPreset(text,registry);},"future pedal schema rejected");}
    atomicWrite(folder/"broken.json","{}");errors.clear();loaded=store.list(registry,errors);
    check(loaded.size()==1&&!errors.empty(),"corrupt personal entry does not hide valid voicings");
    std::filesystem::remove_all(folder);
    if(!failures)std::cout<<"PASS: "<<pedals.size()<<" pedal round trips, Unicode persistence, invalid values/identity/schema and corrupt entry isolation\n";
    return failures?1:0;
}
