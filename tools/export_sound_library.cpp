// Run off audio after building guitarrack_state. Emits tracked fixtures and the
// actual registry mappings used by tools/update_sound_schemas.py.
#include "state/Presets.h"
#include "core/SoundLibrary.h"
#include "core/Impulse.h"
#include <fstream>
#include <iomanip>
#include <iostream>

using namespace guitarrack;
int main(int argc,char** argv) {
    if(argc!=2) {std::cerr<<"Usage: export_sound_library <repository-root>\n";return 1;}
    const auto root=std::filesystem::path(argv[1]);
    const auto registry=EffectRegistry::builtIns();
    const auto bytes=syntheticIRBytes();
    std::ofstream asset(root/"assets/ir/synthetic-filter.wav",std::ios::binary);
    asset.write(reinterpret_cast<const char*>(bytes.data()),static_cast<std::streamsize>(bytes.size()));
    if(!asset) return 1;
    auto directory=root/"assets/presets/experimental";std::filesystem::create_directories(directory);
    for(const auto& tone:experimentalFactoryPresets(registry)) {
        std::ofstream file(directory/(tone.id+".json"));file<<serializePreset(tone,registry);
        if(!file) return 1;
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
