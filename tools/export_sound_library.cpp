// Run off audio after building guitarrack_state. Emits tracked fixtures and the
// actual registry mappings used by tools/update_sound_schemas.py.
#include "state/Presets.h"
#include "core/SoundLibrary.h"
#include "core/Impulse.h"
#include "core/SoundCatalog.h"
#include "state/Assets.h"
#include <fstream>
#include <algorithm>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string_view>

using namespace guitarrack;
namespace {
int exportSongPack(const std::filesystem::path& directory, const EffectRegistry& registry) {
    std::filesystem::create_directories(directory);
    AssetStore assets(directory/"ir");
    std::ostringstream instructions;
    instructions << "# Factory song chains\n\n"
        "These complete rack presets include their original synthetic cabinet assets.\n"
        "Use the application built from feat/radiohead-muse-song-tones or later;\n"
        "older builds may lack required effects/versions.\n\n"
        "The updated app already includes these as factory presets: open Choose\n"
        "Preset, set Source/Artist/Category to All, search Song templates or Muse\n"
        "expansion or Alternative research, select a chain and press Load.\n\n"
        "To use a file: unzip this pack, press Import in GuitarRack, choose one\n"
        ".grbundle file and resolve any unsaved edits. Use Save As to keep your\n"
        "adjusted chain. The ZIP itself is not an application preset.\n\n"
        "All chains fit Legacy 128. Tune the instrument physically; no automatic\n"
        "pitch shifting is inserted. Acoustic Source sounds need an acoustic\n"
        "input; Electric Adaptation sounds are labeled approximations.\n"
        "Album-tone listening and hardware CPU acceptance remain pending.\n\n";
    unsigned count=0;
    for(const auto& tone:factoryPresets(registry)) {
        const auto* metadata=soundMetadata(tone.id);
        if(!metadata || std::find(metadata->tags.begin(),metadata->tags.end(),"Song templates")==metadata->tags.end()) continue;
        auto name=tone.name;
        for(auto& character:name) if(character=='/'||character=='\\') character='-';
        const auto destination=directory/(name+".grbundle");
        // Match the persisted representation; normalization can round by one float ULP.
        const auto expected=parsePreset(serializePreset(tone,registry),registry);
        if(std::filesystem::exists(destination)) {
            if(assets.importPreset(destination,registry)!=expected)
                throw std::runtime_error("Refusing to overwrite an edited bundle: "+destination.string());
        } else {
            assets.exportBundle(destination,tone,registry);
        }
        if(assets.importPreset(destination,registry)!=expected)
            throw std::runtime_error("Bundle import differs from factory: "+tone.name);
        instructions << "- " << name << ".grbundle\n";
        ++count;
    }
    if(count==0) throw std::runtime_error("No Song templates found in the compiled factory catalog.");
    std::ofstream readme(directory/"README.md"); readme << instructions.str();
    if(!readme) throw std::runtime_error("Cannot write song pack instructions.");
    std::cout << "Exported and import-verified " << count << " complete song bundles to " << directory << "\n";
    return 0;
}
}
int main(int argc,char** argv) {
    if(argc==3 && std::string_view(argv[1])=="--song-pack") {
        try {return exportSongPack(argv[2],EffectRegistry::builtIns());}
        catch(const std::exception& error) {std::cerr << error.what() << "\n";return 1;}
    }
    if(argc!=1&&argc!=2) {std::cerr<<"Usage: export_sound_library [repository-root] | --song-pack <directory>\n";return 1;}
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
