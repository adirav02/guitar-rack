#include "PresetBrowser.h"
#include "StudioTheme.h"
#include "core/SoundCatalog.h"
#include "core/Impulse.h"
#include "core/CabinetAssets.h"
namespace guitarrack {
namespace {
juce::String u(const std::string& s) {return juce::String::fromUTF8(s.c_str());}
bool hasId(const std::vector<std::string>& v,const std::string& id) {return std::find(v.begin(),v.end(),id)!=v.end();}
}
PresetBrowser::PresetBrowser(PresetSession& s,const EffectRegistry& r,std::function<void(ToneState)> loadAction,std::function<void()> importing,std::function<void()> exporting,std::function<void()> deleting)
    :DocumentWindow("Choose Preset",juce::Colour(0xff202b3a),closeButton),session(s),registry(r),onLoad(std::move(loadAction)) {
    setUsingNativeTitleBar(true); setContentNonOwned(&content,false); setResizable(true,false); setResizeLimits(700,400,7680,4320);
    for(auto* c:std::initializer_list<juce::Component*>{&search,&source,&artist,&category,&list,&description,&loadButton,&favorite,&cancel,&importButton,&exportButton,&deleteButton,&status}) content.addAndMakeVisible(c);
    for(auto* label:{&heading,&subtitle,&sourceLabel,&artistLabel,&categoryLabel,&resultsLabel,&detailLabel}) {
        content.addAndMakeVisible(label);label->setColour(juce::Label::textColourId,studio::muted);
    }
    heading.setText("Find your next sound",juce::dontSendNotification);
    heading.setFont(juce::FontOptions(25.0f,juce::Font::bold));heading.setColour(juce::Label::textColourId,studio::text);
    subtitle.setText("Explore the collection. Make it yours.",juce::dontSendNotification);
    sourceLabel.setText("COLLECTION",juce::dontSendNotification);
    artistLabel.setText("ARTIST / STYLE",juce::dontSendNotification);
    categoryLabel.setText("CHARACTER",juce::dontSendNotification);
    detailLabel.setText("SOUND NOTES & SIGNAL CHAIN",juce::dontSendNotification);
    for(auto* label:{&sourceLabel,&artistLabel,&categoryLabel,&resultsLabel,&detailLabel}) label->setFont(juce::FontOptions(12.0f,juce::Font::bold));
    studio::primary(loadButton);
    deleteButton.setColour(juce::TextButton::textColourOffId,studio::danger);
    search.setFont(juce::FontOptions(16.0f));search.setIndents(12,8);
    search.setTextToShowWhenEmpty("Search sounds, artists, tags...",studio::muted);
    search.onTextChange=[this]{filter();}; search.onReturnKey=[this]{load();}; search.onEscapeKey=[this]{closeButtonPressed();};
    source.addItemList({"All","Factory","User","Favorites"},1);
    artist.addItemList({"All","Radiohead","Muse","Metallica","Rock","Metal","Original"},1);
    category.addItemList({"All","Clean","Crunch","Distortion","Fuzz","High Gain","Ambient","Experimental"},1);
    for(auto* combo:{&source,&artist,&category}) {combo->setSelectedId(1,juce::dontSendNotification); combo->onChange=[this]{filter();};}
    source.setTooltip("Factory/User source, or your UUID-based favorites."); artist.setTooltip("Artist inspiration or style."); category.setTooltip("Sound gain/character category.");
    description.setMultiLine(true); description.setReadOnly(true); description.setScrollbarsShown(true);
    description.onEscapeKey=[this]{closeButtonPressed();};
    description.setFont(juce::FontOptions(14.0f));description.setIndents(14,12);
    list.setColour(juce::ListBox::backgroundColourId,juce::Colours::transparentBlack);
    list.setRowHeight(68); list.setMultipleSelectionEnabled(false);
    loadButton.onClick=[this]{load();};
    favorite.onClick=[this]{if(highlighted.empty())return; if(hasId(favorites,highlighted)) std::erase(favorites,highlighted); else if(favorites.size()<512) favorites.push_back(highlighted); filter();};
    cancel.onClick=[this]{if(!session.cancelLoad())session.error="Preset already accepted; its transition will complete.";};
    importButton.onClick=std::move(importing); exportButton.onClick=std::move(exporting); deleteButton.onClick=std::move(deleting);
    for(const auto& t:factoryPresets(registry)) factoryIds.push_back(t.id);
    setBounds(100,80,1000,640); poll();
}
PresetBrowser::~PresetBrowser() {clearContentComponent();}
void PresetBrowser::closeButtonPressed() {setVisible(false);}
bool PresetBrowser::keyPressed(const juce::KeyPress& key) {if(key==juce::KeyPress::escapeKey){closeButtonPressed();return true;}return DocumentWindow::keyPressed(key);}
void PresetBrowser::restore(const Preferences& p) {
    favorites=p.favorites; search.setText(u(p.search),false);
    for(auto [combo,value]:{std::pair{&source,p.source},std::pair{&artist,p.artist},std::pair{&category,p.category}}) {
        int index=0; for(int i=0;i<combo->getNumItems();++i)if(combo->getItemText(i)==u(value))index=i;
        combo->setSelectedItemIndex(index,juce::dontSendNotification);
    }
    setBounds(p.browserX,p.browserY,p.browserWidth,p.browserHeight);
    if(auto* display=juce::Desktop::getInstance().getDisplays().getPrimaryDisplay())
        setBounds(getBounds().withSizeKeepingCentre(std::min(getWidth(),display->userBounds.toNearestInt().getWidth()),std::min(getHeight(),display->userBounds.toNearestInt().getHeight())).constrainedWithin(display->userBounds.toNearestInt()));
    filter();
}
void PresetBrowser::capture(Preferences& p) const {
    p.favorites=favorites;p.search=search.getText().toStdString();p.source=source.getText().toStdString();p.artist=artist.getText().toStdString();p.category=category.getText().toStdString();
    p.browserX=getX();p.browserY=getY();p.browserWidth=getWidth();p.browserHeight=getHeight();
}
void PresetBrowser::poll() {
    if(catalog!=session.library){catalog=session.library;filter();}
    status.setText(session.error.empty()?(session.busy()?(session.loading()?"Loading / preparing / transitioning...":"Applying rack edits..."):"Select to inspect. Load, double-click or Enter applies the sound."):u(session.error),juce::dontSendNotification);
    status.setTooltip(u(session.error));loadButton.setEnabled(!session.busy()&&!highlighted.empty());cancel.setEnabled(session.loading());
    deleteButton.setEnabled(session.canDelete());exportButton.setEnabled(!session.busy());
}
int PresetBrowser::getNumRows(){return static_cast<int>(filtered.size());}
void PresetBrowser::paintListBoxItem(int row,juce::Graphics& g,int width,int height,bool selected) {
    if(row<0||row>=getNumRows())return;
    const auto& tone=filtered[static_cast<std::size_t>(row)];
    const auto* metadata=soundMetadata(tone.id);
    auto bounds=juce::Rectangle<float>(3,3,float(width-6),float(height-6));
    g.setColour(selected?juce::Colour(0xff29463e):studio::panel);g.fillRoundedRectangle(bounds,8);
    if(selected) {g.setColour(studio::accent);g.drawRoundedRectangle(bounds.reduced(.5f),8,1.5f);}
    g.setColour(selected?studio::accent:studio::border);g.fillRoundedRectangle(12,16,3,float(height-32),1.5f);
    const bool isFavorite=hasId(favorites,tone.id);
    g.setColour(studio::text);g.setFont(juce::FontOptions(15.0f,juce::Font::bold));
    g.drawText((isFavorite?"★ ":"")+u(tone.name),24,10,width-38,23,juce::Justification::centredLeft);
    const auto detail=metadata?u(metadata->artist)+"  /  "+u(metadata->gain):juce::String("User sound");
    g.setColour(selected?studio::accent:studio::muted);g.setFont(12.0f);
    g.drawText(detail+"  ·  "+juce::String(int(tone.rack.order.size()))+" pedals",24,36,width-38,20,juce::Justification::centredLeft);
}
void PresetBrowser::filter() {
    filtered.clear(); const auto query=search.getText().trim();
    for(const auto& t:catalog) {
        const bool factory=hasId(factoryIds,t.id); const auto* m=soundMetadata(t.id);
        const auto src=source.getText(); if((src=="Factory"&&!factory)||(src=="User"&&factory)||(src=="Favorites"&&!hasId(favorites,t.id)))continue;
        juce::String haystack=u(t.name);if(m) {haystack+=" "+u(m->artist)+" "+u(m->style)+" "+u(m->character);for(const auto& tag:m->tags)haystack+=" "+u(tag);}
        bool matches=true;for(const auto& word:juce::StringArray::fromTokens(query,false))if(!haystack.containsIgnoreCase(word))matches=false;
        if(!matches)continue;
        if(artist.getSelectedId()>1&&(!m||(!u(m->artist).containsIgnoreCase(artist.getText())&&!u(m->style).containsIgnoreCase(artist.getText()))))continue;
        if(category.getSelectedId()>1&&(!m||u(m->gain)!=category.getText()))continue;
        filtered.push_back(t);
    }
    resultsLabel.setText(juce::String(int(filtered.size()))+" SOUNDS",juce::dontSendNotification);
    content.repaint();
    list.updateContent(); int row=-1;for(int i=0;i<getNumRows();++i)if(filtered[i].id==highlighted)row=i;
    list.selectRow(row);if(row<0){highlighted.clear();details();}
}
void PresetBrowser::selectedRowsChanged(int row) {highlighted=row>=0&&row<getNumRows()?filtered[row].id:"";details();}
void PresetBrowser::details() {
    auto at=std::find_if(filtered.begin(),filtered.end(),[this](const auto& t){return t.id==highlighted;});
    if(at==filtered.end()){description.setText("Select a sound to inspect its recipe. Navigation does not change audio.");return;}
    juce::String text=u(at->name)+"\n\n";
    if(const auto* m=soundMetadata(at->id)) text+=u(m->character)+"\n\nInspiration / style: "+u(m->artist)+" / "+u(m->style)+"\nGain: "+u(m->gain)+"\nTuning: "+u(m->tuning)+"\nPickups: "+u(m->pickups)+"\nInput: "+u(m->input)+"\nAmp family: "+u(m->amp)+"\nCabinet: "+u(m->cabinet)+"\n\n"+u(m->notes)+"\nEvidence: "+u(m->evidence)+"\nListening: "+u(m->listening)+"\n";
    else text+="User sound. No catalog listening/research assessment.\n";
    text+="\nOrdered chain:\n";int index=1;
    for(const auto& id:at->rack.order) {
        const auto& i=*at->rack.find(id);const auto* d=registry.find(i.effectType,i.effectVersion);
        text+=juce::String(index++)+". "+u(d->displayName)+" v"+juce::String(i.effectVersion)+(at->controls.bypass[i.slot]?" [bypassed]":"")+"\n";
        if(!i.irAsset.empty()) {
            const bool embedded=!builtinIRBytes(i.irAsset).empty();const bool stored=std::filesystem::is_regular_file(session.assets.directory/(i.irAsset+".wav"));
            text+="   IR "+u(i.irAsset.substr(0,12))+": "+(embedded?"embedded original synthetic":stored?"stored; checksum verified on Load":"missing; Load will retain current sound")+"\n";
        }
    }
    description.setText(text);favorite.setButtonText(hasId(favorites,highlighted)?"Unfavorite":"Favorite");
}
void PresetBrowser::load() {
    auto at=std::find_if(filtered.begin(),filtered.end(),[this](const auto& t){return t.id==highlighted;});
    if(at!=filtered.end())onLoad(*at);
}
void PresetBrowser::listBoxItemDoubleClicked(int row,const juce::MouseEvent&){selectedRowsChanged(row);load();}
void PresetBrowser::returnKeyPressed(int row){selectedRowsChanged(row);load();}
void PresetBrowser::Content::paint(juce::Graphics& g) {
    g.fillAll(studio::background);
    g.setColour(studio::accent);g.fillRoundedRectangle(20,23,3,28,1.5f);
    if(browser.filtered.empty()) {
        g.setColour(studio::muted);g.setFont(15.0f);
        g.drawFittedText("No sounds found.\nTry another search or filter.",browser.list.getBounds().reduced(18),juce::Justification::centred,3);
    }
}
void PresetBrowser::Content::resized() {
    auto a=getLocalBounds().reduced(20,16);
    auto header=a.removeFromTop(36);browser.heading.setBounds(header.withTrimmedLeft(12));
    browser.subtitle.setBounds(a.removeFromTop(22));a.removeFromTop(12);
    browser.search.setBounds(a.removeFromTop(38));a.removeFromTop(12);
    auto labels=a.removeFromTop(20);auto filters=a.removeFromTop(32);int third=(filters.getWidth()-20)/3;
    for(auto pair:{std::pair{&browser.sourceLabel,&browser.source},std::pair{&browser.artistLabel,&browser.artist},std::pair{&browser.categoryLabel,&browser.category}}) {
        pair.first->setBounds(labels.removeFromLeft(third));labels.removeFromLeft(10);
        pair.second->setBounds(filters.removeFromLeft(third));filters.removeFromLeft(10);
    }
    auto footer=a.removeFromBottom(36);
    browser.loadButton.setBounds(footer.removeFromLeft(86));footer.removeFromLeft(6);
    browser.favorite.setBounds(footer.removeFromLeft(90));footer.removeFromLeft(6);
    browser.cancel.setBounds(footer.removeFromLeft(100));footer.removeFromLeft(12);
    browser.deleteButton.setBounds(footer.removeFromRight(145));footer.removeFromRight(6);
    browser.exportButton.setBounds(footer.removeFromRight(78));footer.removeFromRight(6);
    browser.importButton.setBounds(footer.removeFromRight(78));
    browser.status.setBounds(a.removeFromBottom(40));a.removeFromTop(12);
    auto captions=a.removeFromTop(24);int half=a.getWidth()*45/100;
    browser.resultsLabel.setBounds(captions.removeFromLeft(half));captions.removeFromLeft(12);browser.detailLabel.setBounds(captions);
    browser.list.setBounds(a.removeFromLeft(half));a.removeFromLeft(12);browser.description.setBounds(a);
}
}
