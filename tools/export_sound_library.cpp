// Run off audio after building guitarrack_state. Emits tracked fixtures and the
// actual registry mappings used by tools/update_sound_schemas.py.
#include "state/Presets.h"
#include "core/SoundLibrary.h"
#include "core/Impulse.h"
#include <fstream>
#include <algorithm>
#include <iomanip>
#include <iostream>

using namespace guitarrack;
int main(int argc,char** argv) {
    if(argc!=1&&argc!=2) {std::cerr<<"Usage: export_sound_library <repository-root>\n";return 1;}
    const auto root=argc==2?std::filesystem::path(argv[1]):std::filesystem::path{};
    const auto registry=EffectRegistry::builtIns();
    if(argc==2) {
    const auto bytes=syntheticIRBytes();
    std::ofstream asset(root/"assets/ir/synthetic-filter.wav",std::ios::binary);
    asset.write(reinterpret_cast<const char*>(bytes.data()),static_cast<std::streamsize>(bytes.size()));
    if(!asset) return 1;
    auto directory=root/"assets/presets/experimental";std::filesystem::create_directories(directory);
    for(const auto& tone:experimentalFactoryPresets(registry)) {
        const auto destination=directory/(tone.id+".json");
        if(tone.id.starts_with("50000000-")&&std::filesystem::exists(destination)) {
            if(parsePreset(readBounded(destination),registry)!=tone) {std::cerr<<"Refusing to change frozen legacy factory: "<<tone.id<<"\n";return 1;}
            continue;
        }
        auto text=serializePreset(tone,registry);
        text.erase(std::remove(text.begin(),text.end(),'\r'),text.end());
        std::ofstream file(destination);file<<text;
        if(!file) return 1;
    }
    }
    std::cout<<"[";
    bool firstEffect=true;
    for(const auto& d:registry.descriptors()) {
        if(!firstEffect)std::cout<<",";firstEffect=false;
        std::cout<<"{\"type\":"<<std::quoted(d.typeId)<<",\"version\":"<<d.version<<",\"parameters\":[";
        bool first=true;
        for(const auto& p:d.parameters) {
            if(!first)std::cout<<",";first=false;
            std::cout<<"{\"key\":"<<std::quoted(p.key)<<",\"index\":"<<p.bankIndex<<",\"units\":"<<std::quoted(p.units)
                     <<",\"minimum\":"<<p.minimum<<",\"maximum\":"<<p.maximum<<",\"default\":"<<p.defaultValue<<",\"choices\":[";
            for(std::size_t j=0;j<p.choices.size();++j){if(j)std::cout<<",";std::cout<<std::quoted(p.choices[j]);}
            std::cout<<"]}";
        }
        std::cout<<"]}";
    }
    std::cout<<"]\n";
}
