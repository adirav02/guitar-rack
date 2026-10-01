#include "Presets.h"
#include "core/SoundLibrary.h"
#include "core/Impulse.h"
#include <cmath>
#include <charconv>
#include <cstring>
#include <cstdlib>
#include <set>
#include <optional>
#include <stdexcept>
#include <system_error>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>

namespace guitarrack {
namespace {
[[noreturn]] void fail(const std::string& reason) { throw std::runtime_error(reason); }
// JUCE's JSON reader is used after this strict preflight (JUCE otherwise accepts
// duplicate keys/trailing commas). Depth/size/node bounds apply before construction.
class Syntax {
public:
    explicit Syntax(const std::string& input) : s(input) {}
    void check() { value(0); space(); if (p != s.size()) fail("Trailing JSON content."); }
private:
    const std::string& s; std::size_t p=0; unsigned nodes=0;
    void space() { while (p<s.size() && (s[p]==' '||s[p]=='\n'||s[p]=='\r'||s[p]=='\t')) ++p; }
    bool take(char c) { space(); if (p<s.size() && s[p]==c) { ++p; return true; } return false; }
    void expect(char c) { if (!take(c)) fail("Malformed JSON punctuation."); }
    std::string string() {
        space(); auto begin=p; if (p==s.size()||s[p++]!='"') fail("Expected JSON string.");
        bool closed=false;
        while(p<s.size()) {
            unsigned char c=s[p++]; if(c=='"') { closed=true; break; }
            if(c<32) fail("Control character in JSON string.");
            if(c=='\\') {
                if(p==s.size()) fail("Incomplete JSON escape.");
                char e=s[p++];
                if(e=='u') { unsigned code=0; for(int i=0;i<4;++i) { if(p==s.size()) fail("Incomplete Unicode escape."); char h=s[p++]; unsigned v;
                    if(h>='0'&&h<='9') v=h-'0'; else if(h>='a'&&h<='f') v=h-'a'+10; else if(h>='A'&&h<='F') v=h-'A'+10; else fail("Invalid Unicode escape."); code=code*16+v; }
                    if(code==0) fail("Null in JSON string.");
                    if(code>=0xd800 && code<=0xdbff) {
                        if(p+6>s.size()||s[p]!='\\'||s[p+1]!='u') fail("Unpaired Unicode surrogate.");
                        p+=2;
                        unsigned low=0; for(int i=0;i<4;++i) { char h=s[p++]; unsigned v;
                            if(h>='0'&&h<='9') v=h-'0'; else if(h>='a'&&h<='f') v=h-'a'+10; else if(h>='A'&&h<='F') v=h-'A'+10; else fail("Invalid surrogate escape."); low=low*16+v; }
                        if(low<0xdc00||low>0xdfff) fail("Invalid surrogate pair.");
                    } else if(code>=0xdc00&&code<=0xdfff) fail("Unpaired Unicode surrogate.");
                } else if(std::string("\"\\/bfnrt").find(e)==std::string::npos) fail("Invalid JSON escape.");
            }
        }
        if(!closed) fail("Unterminated JSON string.");
        auto decoded=juce::JSON::fromString(juce::String::fromUTF8(s.data()+begin, static_cast<int>(p-begin)));
        if(!decoded.isString()) fail("Invalid JSON string.");
        return decoded.toString().toStdString();
    }
    void value(unsigned depth) {
        if(depth>32||++nodes>4096) fail("JSON resource limit exceeded.");
        space(); if(p==s.size()) fail("Incomplete JSON.");
        if(take('{')) { std::set<std::string> keys; if(take('}')) return;
            do { if(!keys.insert(string()).second) fail("Duplicate JSON key."); expect(':'); value(depth+1); if(take('}')) return; expect(','); } while(true);
        }
        if(take('[')) { if(take(']')) return; do { value(depth+1); if(take(']')) return; expect(','); } while(true); }
        if(s[p]=='"') { string(); return; }
        for(const auto* word : {"true","false","null"}) { std::string w=word; if(s.compare(p,w.size(),w)==0) { p+=w.size(); return; } }
        const auto begin=p;
        if(s[p]=='-') ++p;
        if(p==s.size()||s[p]<'0'||s[p]>'9') fail("Invalid JSON number.");
        if(s[p]=='0') ++p; else while(p<s.size()&&s[p]>='0'&&s[p]<='9') ++p;
        auto digits=[&] { auto start=p; while(p<s.size()&&s[p]>='0'&&s[p]<='9') ++p; if(p==start) fail("Invalid JSON number."); };
        if(p<s.size()&&s[p]=='.') { ++p; digits(); }
        if(p<s.size()&&(s[p]=='e'||s[p]=='E')) { ++p; if(p<s.size()&&(s[p]=='+'||s[p]=='-')) ++p; digits(); }
        double parsed=0;
        auto converted=std::from_chars(s.data()+begin,s.data()+p,parsed);
        if(converted.ec!=std::errc{}||!std::isfinite(parsed)) fail("Non-finite or unrepresentable JSON number.");
        // JUCE parses integer tokens into int64. Reject overflow before its parser
        // can wrap a huge literal into an apparently valid small control value.
        auto token=s.substr(begin,p-begin);
        if(token.find_first_of(".eE")==std::string::npos) {
            std::int64_t integer=0;
            auto result=std::from_chars(token.data(),token.data()+token.size(),integer);
            if(result.ec!=std::errc{}) fail("JSON integer overflow.");
        }
    }
};
void keys(const juce::var& o, std::initializer_list<const char*> wanted) {
    auto* obj=o.getDynamicObject(); if(!obj) fail("Expected JSON object.");
    if(obj->getProperties().size()!=static_cast<int>(wanted.size())) fail("Missing or unknown JSON fields.");
    for(auto* k:wanted) if(!obj->hasProperty(k)) fail(std::string("Missing field: ")+k);
}
std::string str(const juce::var& v) { if(!v.isString()) fail("Expected string."); return v.toString().toStdString(); }
double number(const juce::var& v, double lo, double hi) {
    if(!v.isInt()&&!v.isInt64()&&!v.isDouble()) fail("Expected number.");
    double x=static_cast<double>(v);
    if(!std::isfinite(x)||x<lo||x>hi) fail("Number outside allowed range.");
    return x;
}
unsigned integer(const juce::var& v, unsigned lo, unsigned hi) { double x=number(v,lo,hi); if(std::floor(x)!=x) fail("Expected integer."); return static_cast<unsigned>(x); }
bool boolean(const juce::var& v) { if(!v.isBool()) fail("Expected boolean."); return static_cast<bool>(v); }
const juce::Array<juce::var>& array(const juce::var& v) { auto* a=v.getArray(); if(!a) fail("Expected array."); return *a; }
juce::var object() { return juce::var(new juce::DynamicObject()); }
void put(juce::var& o, const char* k, const juce::var& v) { o.getDynamicObject()->setProperty(k,v); }
std::filesystem::path xdg(const char* key, const char* fallback) { if(auto* p=std::getenv(key); p && std::filesystem::path(p).is_absolute()) return std::filesystem::path(p)/"GuitarRack"; return std::filesystem::path(juce::File::getSpecialLocation(juce::File::userHomeDirectory).getFullPathName().toStdString())/fallback/"GuitarRack"; }
}
juce::var parseJson(const std::string& text) {
    if(text.empty()||text.size()>maxPresetBytes) fail("Empty or oversized JSON file (maximum 128 KiB).");
    if(!validUtf8(text)) fail("Invalid UTF-8.");
    Syntax(text).check(); juce::var value;
    auto r=juce::JSON::parse(juce::String::fromUTF8(text.data(),static_cast<int>(text.size())),value);
    if(r.failed()) fail(r.getErrorMessage().toStdString());
    return value;
}
ToneState migratePreset(const juce::var& root, const EffectRegistry& registry) {
    // Dispatch point for future explicit migrations. No invented legacy format.
    keys(root,{"schema_version","preset_id","name","globals","order","instances","assets"});
    auto version=integer(root["schema_version"],0,100000);
    if(version!=1&&version!=2) fail("Unsupported preset schema version; expected 1 or 2.");
    ToneState t; t.schemaVersion=version; t.id=str(root["preset_id"]); t.name=str(root["name"]);
    if(!validUtf8(t.name)) fail("Invalid UTF-8 preset name.");
    auto g=root["globals"]; keys(g,{"input_db","output_db","rack_bypass"});
    t.controls.inputDb=static_cast<float>(number(g["input_db"],-24,24)); t.controls.outputDb=static_cast<float>(number(g["output_db"],-60,6)); t.controls.rackBypass=boolean(g["rack_bypass"]);
    std::set<std::string> assets,usedAssets;
    if(array(root["assets"]).size()>16) fail("Too many IR assets.");
    if(version==1&&!array(root["assets"]).isEmpty()) fail("IR assets require schema version 2.");
    for(const auto& asset:array(root["assets"])) {
        keys(asset,{"asset_id","relative_path","sha256","format"});
        auto id=str(asset["asset_id"]);
        if(!validAssetId(id)||str(asset["sha256"])!=id||str(asset["relative_path"])!="ir/"+id+".wav"||str(asset["format"])!="wav"||!assets.insert(id).second)
            fail("Invalid, unsafe or duplicate IR asset manifest.");
    }
    auto& instances=array(root["instances"]); auto& order=array(root["order"]);
    if(instances.size()>16||order.size()>16) fail("Preset exceeds 16 slots.");
    for(const auto& v:instances) {
        if(version==1) keys(v,{"instance_id","slot_id","effect_type","effect_version","bypassed","parameters"});
        else keys(v,{"instance_id","slot_id","effect_type","effect_version","bypassed","parameters","ir_asset"});
        RackInstance i{str(v["instance_id"]),integer(v["slot_id"],0,15),str(v["effect_type"]),integer(v["effect_version"],1,100000)};
        if(version==2) i.irAsset=str(v["ir_asset"]);
        if(!i.irAsset.empty()) { if(!assets.contains(i.irAsset)) fail("Missing required IR manifest reference."); usedAssets.insert(i.irAsset); }
        auto* d=registry.find(i.effectType,i.effectVersion); if(!d) fail("Unknown required effect or incompatible effect version.");
        auto params=v["parameters"]; auto* o=params.getDynamicObject();
        if(!o||o->getProperties().size()!=static_cast<int>(d->parameters.size())) fail("Missing/unknown effect parameter.");
        for(const auto& p:d->parameters) {
            if(!o->hasProperty(p.key.c_str())) fail("Missing required parameter: "+p.key);
            auto x=static_cast<float>(number(o->getProperty(p.key.c_str()),p.minimum,p.maximum));
            if (!p.accepts(x)) fail("Invalid effect mode: "+p.key);
            t.controls.values[i.slot][p.bankIndex]=p.normalize(x);
        }
        t.controls.bypass[i.slot]=boolean(v["bypassed"]); t.rack.instances.push_back(i);
    }
    for(const auto& id:order) t.rack.order.push_back(str(id));
    if(assets!=usedAssets) fail("Preset has unreferenced IR assets.");
    if(auto e=validateTone(t,registry); !e.empty()) fail(e);
    return t;
}
ToneState parsePreset(const std::string& s,const EffectRegistry& r) { return migratePreset(parseJson(s),r); }
std::string serializePreset(const ToneState& t,const EffectRegistry& r) {
    if(auto e=validateTone(t,r); !e.empty()) fail(e);
    if(!validUtf8(t.name)) fail("Invalid UTF-8 preset name.");
    auto root=object(), g=object(); put(root,"schema_version",static_cast<int>(t.schemaVersion)); put(root,"preset_id",juce::String::fromUTF8(t.id.c_str())); put(root,"name",juce::String::fromUTF8(t.name.c_str()));
    put(g,"input_db",t.controls.inputDb); put(g,"output_db",t.controls.outputDb); put(g,"rack_bypass",t.controls.rackBypass); put(root,"globals",g);
    juce::Array<juce::var> order, instances, assets; std::set<std::string> seenAssets;
    for(const auto& id:t.rack.order) order.add(juce::String(id));
    for(const auto& i:t.rack.instances) {
        auto v=object(), p=object(); put(v,"instance_id",juce::String(i.id)); put(v,"slot_id",static_cast<int>(i.slot)); put(v,"effect_type",juce::String(i.effectType)); put(v,"effect_version",static_cast<int>(i.effectVersion)); put(v,"bypassed",t.controls.bypass[i.slot]);
        for(const auto& d:r.find(i.effectType,i.effectVersion)->parameters) put(p,d.key.c_str(),d.denormalize(t.controls.values[i.slot][d.bankIndex]));
        put(v,"parameters",p);
        if(t.schemaVersion==2) put(v,"ir_asset",juce::String(i.irAsset));
        if(!i.irAsset.empty()&&seenAssets.insert(i.irAsset).second) {
            auto asset=object(); put(asset,"asset_id",juce::String(i.irAsset)); put(asset,"sha256",juce::String(i.irAsset));
            put(asset,"relative_path",juce::String("ir/"+i.irAsset+".wav")); put(asset,"format","wav"); assets.add(asset);
        }
        instances.add(v);
    }
    put(root,"order",order); put(root,"instances",instances); put(root,"assets",assets);
    return juce::JSON::toString(root).toStdString()+"\n";
}
namespace {
std::string readDescriptor(int descriptor, const std::filesystem::path& path) {
    struct stat info{};
    if (::fstat(descriptor, &info) != 0 || !S_ISREG(info.st_mode))
        fail("Preset/settings must be a regular file: " + path.string());
    if (info.st_size < 0 || static_cast<std::uintmax_t>(info.st_size) > maxPresetBytes)
        fail("File exceeds 128 KiB: " + path.string());
    std::string text(maxPresetBytes + 1, '\0');
    std::size_t used = 0;
    while (used < text.size()) {
        const auto count = ::read(descriptor, text.data() + used, text.size() - used);
        if (count < 0 && errno == EINTR) continue;
        if (count < 0) fail("Read failed: " + path.string() + ": " + std::strerror(errno));
        if (count == 0) break;
        used += static_cast<std::size_t>(count);
    }
    if (used > maxPresetBytes) fail("File grew beyond 128 KiB: " + path.string());
    text.resize(used);
    return text;
}
struct Descriptor {
    int value;
    ~Descriptor() { if (value >= 0) ::close(value); }
};
}
std::string readBounded(const std::filesystem::path& path) {
    // Nonblocking open prevents a selected FIFO from hanging the parser/close.
    // Type/size validation and bytes refer to this same descriptor.
    Descriptor descriptor{::open(path.c_str(), O_RDONLY | O_CLOEXEC | O_NONBLOCK)};
    if (descriptor.value < 0) fail("Cannot read: " + path.string() + ": " + std::strerror(errno));
    return readDescriptor(descriptor.value, path);
}
void atomicWrite(const std::filesystem::path& path,const std::string& text) {
    if(text.size()>maxPresetBytes) fail("Write exceeds 128 KiB.");
    auto dir=path.parent_path(); if(dir.empty()) dir=".";
    std::filesystem::create_directories(dir);
    std::string temporary=(dir/".guitarrack-XXXXXX").string(); int fd=::mkstemp(temporary.data()); if(fd<0) fail("Cannot create temporary file: "+std::string(std::strerror(errno)));
    try {
        std::size_t done=0; while(done<text.size()) { auto n=::write(fd,text.data()+done,text.size()-done); if(n<0&&errno==EINTR) continue; if(n<=0) fail("Preset write failed: "+std::string(std::strerror(errno))); done+=static_cast<std::size_t>(n); }
        if(::fsync(fd)!=0) fail("File sync failed: "+std::string(std::strerror(errno)));
        if(::close(fd)!=0) { fd=-1; fail("File close failed."); } fd=-1;
        if(::rename(temporary.c_str(),path.c_str())!=0) fail("Atomic replacement failed: "+std::string(std::strerror(errno)));
        int directoryFd=::open(dir.c_str(),O_RDONLY|O_DIRECTORY);
        if(directoryFd<0) fail("Cannot sync preset directory.");
        int result=::fsync(directoryFd); ::close(directoryFd); if(result!=0) fail("Directory sync failed; durability uncertain.");
    } catch(...) { if(fd>=0) ::close(fd); ::unlink(temporary.c_str()); throw; }
}
std::filesystem::path userDataDirectory() { return xdg("XDG_DATA_HOME",".local/share"); }
std::filesystem::path userConfigDirectory() { return xdg("XDG_CONFIG_HOME",".config"); }
void PresetStore::save(const ToneState& t,const EffectRegistry& r) const { atomicWrite(directory/(t.id+".json"),serializePreset(t,r)); }
void PresetStore::erase(const std::string& id, const EffectRegistry& registry,
                        const std::filesystem::path& recallFile, const ToneState* saveFirst) const {
    if (!validInstanceId(id)) fail("Invalid managed preset UUID.");
    for (const auto& factory : factoryPresets(registry))
        if (factory.id == id) fail("Factory presets cannot be deleted.");

    Descriptor folder{::open(directory.c_str(), O_RDONLY | O_DIRECTORY | O_CLOEXEC | O_NOFOLLOW)};
    if (folder.value < 0) fail("Cannot open user preset directory: " + directory.string()
                               + ". Check directory permissions: " + std::strerror(errno));
    const auto filename = id + ".json";
    Descriptor file{::openat(folder.value, filename.c_str(), O_RDONLY | O_CLOEXEC | O_NONBLOCK | O_NOFOLLOW)};
    struct stat original{};
    if (file.value < 0 || ::fstat(file.value, &original) != 0 || !S_ISREG(original.st_mode))
        fail("Cannot delete managed preset " + (directory / filename).string()
             + ". It must be an accessible regular file, not a link or directory.");
    const auto saved = readDescriptor(file.value, directory / filename);
    if (parsePreset(saved, registry).id != id) fail("Managed file UUID changed; refresh presets before deleting.");

    std::optional<std::string> oldRecall;
    bool recallMatches = false;
    const auto recallStatus = std::filesystem::symlink_status(recallFile);
    if (std::filesystem::exists(recallStatus)) {
        if (!std::filesystem::is_regular_file(recallStatus))
            fail("Cannot update local recall: " + recallFile.string() + ". It must be a regular file.");
        oldRecall = readBounded(recallFile);
        recallMatches = parsePreset(*oldRecall, registry).id == id;
    }
    // All potentially allocating serialization finishes before the file is removed.
    const auto cleanRecall = serializePreset(factoryPresets(registry).front(), registry);
    std::optional<std::string> edited;
    if (saveFirst) {
        if (saveFirst->id != id) fail("Save choice does not match the selected preset.");
        edited = serializePreset(*saveFirst, registry);
    }
    auto checkEntry = [&] {
        struct stat current{};
        if (::fstatat(folder.value, filename.c_str(), &current, AT_SYMLINK_NOFOLLOW) != 0
            || !S_ISREG(current.st_mode) || current.st_dev != original.st_dev || current.st_ino != original.st_ino)
            fail("Managed preset changed during deletion; refresh and try again.");
    };
    bool recallTouched = false, fileTouched = false;
    try {
        checkEntry();
        if (edited) {
            // Save is part of this transaction. Failed deletion restores the
            // original file as well as preserving the session's dirty baseline.
            fileTouched = true;
            atomicWrite(directory / filename, *edited);
            if (::fstatat(folder.value, filename.c_str(), &original, AT_SYMLINK_NOFOLLOW) != 0
                || !S_ISREG(original.st_mode)) fail("Cannot verify saved user preset; refresh and try again.");
        }
        if (recallMatches) {
            recallTouched = true;
            atomicWrite(recallFile, cleanRecall);
        }
        checkEntry();
        if (::unlinkat(folder.value, filename.c_str(), 0) != 0)
            fail("Cannot delete " + (directory / filename).string()
                 + ". Check user preset directory permissions: " + std::strerror(errno));
        fileTouched = true;
        if (::fsync(folder.value) != 0) fail("Preset directory sync failed; deletion durability is uncertain.");
    } catch (const std::exception& e) {
        std::string message = e.what();
        // Rollback never targets an export or an asset: both destinations are
        // store/local-recall paths. Keep the in-memory session unchanged on error.
        try { if (fileTouched) atomicWrite(directory / filename, saved); }
        catch (const std::exception& rollback) { message += " File rollback failed: " + std::string(rollback.what()); }
        try { if (recallTouched) atomicWrite(recallFile, *oldRecall); }
        catch (const std::exception& rollback) { message += " Recall rollback failed: " + std::string(rollback.what()); }
        fail(message);
    }
}
std::vector<ToneState> PresetStore::list(const EffectRegistry& r,std::vector<std::string>& errors) const {
    std::vector<ToneState> result; if(!std::filesystem::exists(directory)) return result;
    for(const auto& file:std::filesystem::directory_iterator(directory)) if(file.path().extension()==".json") {
        try { auto t=parsePreset(readBounded(file.path()),r); if(file.path().filename()!=t.id+".json") fail("Preset filename must match its UUID."); result.push_back(std::move(t)); }
        catch(const std::exception& e) { errors.push_back(file.path().string()+": "+e.what()); }
    }
    return result;
}
std::vector<ToneState> factoryPresets(const EffectRegistry& r) {
    std::vector<ToneState> result;
    for(int index=0;index<3;++index) {
        ToneState t; t.id="00000000-0000-4000-8000-00000000000"+std::to_string(index+1); t.name=std::array<const char*,3>{"Clean","Crunch","Lead Delay"}[index];
        if(index>0) {
            auto add=[&](const char* type,unsigned slot) { auto id="10000000-0000-4000-8000-00000000000"+std::to_string(slot+1); t.rack.instances.push_back({id,slot,type,1}); t.rack.order.push_back(id); for(const auto& p:r.find(type,1)->parameters) t.controls.values[slot][p.bankIndex]=p.normalize(p.defaultValue); };
            add("overdrive",0); add("eq",1); if(index==2) add("delay",2);
            t.controls.values[0][0]=r.find("overdrive",1)->parameters[0].normalize(index==1?9.f:12.f);
            t.controls.values[0][3]=r.find("overdrive",1)->parameters[3].normalize(index==1?35.f:50.f);
            t.controls.values[1][2]=r.find("eq",1)->parameters[2].normalize(-2.f);
            if(index==2) t.controls.values[2][2]=r.find("delay",1)->parameters[2].normalize(20.f);
        }
        result.push_back(t);
    }
    for(unsigned example=0;example<2;++example) {
        ToneState t; t.id="00000000-0000-4000-8000-00000000000"+std::to_string(example+4);
        t.name=example==0?"Driven Synthetic Cabinet":"Small Room";
        auto add=[&](const std::string& type) {
            auto slot=static_cast<unsigned>(t.rack.instances.size());
            auto id=std::to_string(example+2)+"0000000-0000-4000-8000-00000000000"+std::to_string(slot+1);
            t.rack.instances.push_back({id,slot,type,1,type=="cabinet"?syntheticIRId():""}); t.rack.order.push_back(id);
            for(const auto& p:r.find(type,1)->parameters) t.controls.values[slot][p.bankIndex]=p.normalize(p.defaultValue);
        };
        if(example==0) {
            t.schemaVersion=2; add("distortion"); add("cabinet"); add("eq"); add("delay"); add("reverb");
            t.controls.values[2][2]=r.find("eq",1)->parameters[2].normalize(-2);
            t.controls.values[3][2]=r.find("delay",1)->parameters[2].normalize(15);
            t.controls.values[4][2]=r.find("reverb",1)->parameters[2].normalize(10);
        } else { add("eq"); add("reverb"); }
        result.push_back(std::move(t));
    }
    auto extra=experimentalFactoryPresets(r);
    result.insert(result.end(),extra.begin(),extra.end());
    return result;
}
}

