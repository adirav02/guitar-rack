#include "PresetBar.h"
namespace guitarrack {
PresetBar::PresetBar(RackEngine& rack) : presets(rack,userDataDirectory()/"presets") {
    for(auto* c: std::initializer_list<juce::Component*>{&choices,&saveButton,&saveAsButton,&importButton,&exportButton,&cancelButton,&indicator,&status}) addAndMakeVisible(c);
    choices.setTextWhenNothingSelected("Clean"); refreshChoices();
    choices.onChange=[this] { int i=choices.getSelectedItemIndex(); if(i<0) return; auto tone=displayed[static_cast<std::size_t>(i)]; guard([safe=juce::Component::SafePointer<PresetBar>(this),tone] { if(safe) safe->presets.select(tone); }); };
    saveButton.onClick=[this] { if(presets.selectedIsFactory()) saveAs(); else presets.save(); };
    saveAsButton.onClick=[this] { saveAs(); };
    importButton.onClick=[this] { guard([safe=juce::Component::SafePointer<PresetBar>(this)] { if(safe) safe->fileDialog(true); }); };
    exportButton.onClick=[this] { fileDialog(false); };
    cancelButton.onClick=[this] { if(!presets.cancelLoad()) presets.error="Preset already accepted; its transition will complete."; };
    try { auto path=userDataDirectory()/"last-tone.json"; if(std::filesystem::exists(path)) presets.importFile(path); }
    catch(const std::exception& e) { presets.error=e.what(); }
}
void PresetBar::refreshChoices() {
    displayed=presets.library; choices.clear(juce::dontSendNotification);
    for(std::size_t i=0;i<displayed.size();++i) choices.addItem(juce::String::fromUTF8(displayed[i].name.c_str()),static_cast<int>(i+1));
}
void PresetBar::poll() {
    presets.poll();
    if(displayed!=presets.library) refreshChoices();
    auto selected=presets.selected();
    int index=-1; if(selected) for(std::size_t i=0;i<displayed.size();++i) if(displayed[i].id==selected->id) index=static_cast<int>(i);
    choices.setSelectedItemIndex(index,juce::dontSendNotification);
    if(index<0) choices.setText(selected?juce::String::fromUTF8(selected->name.c_str()):"Clean",juce::dontSendNotification);
    const bool busy=presets.busy();
    indicator.setText(busy?(presets.loading()?"Loading preset...":"Applying edits..."):presets.dirty()?"* Unsaved changes":"Saved / unchanged",juce::dontSendNotification);
    status.setText(juce::String::fromUTF8(presets.error.c_str()),juce::dontSendNotification);
    status.setTooltip(juce::String::fromUTF8(presets.error.c_str()));
    saveButton.setEnabled(!busy&&!dialogOpen); saveAsButton.setEnabled(!busy&&!dialogOpen); exportButton.setEnabled(!busy&&!dialogOpen);
    importButton.setEnabled(!dialogOpen); choices.setEnabled(!dialogOpen); cancelButton.setEnabled(presets.loading()&&!dialogOpen);
}
void PresetBar::guard(std::function<void()> action) {
    if(dialogOpen) return;
    if(presets.busy()) { presets.error="Wait for loading/edits to finish, or cancel the pending load."; return; }
    if(!presets.dirty()) { action(); return; }
    dialogOpen=true;
    auto* alert=new juce::AlertWindow("Unsaved preset changes","Save your custom rack before continuing?",juce::MessageBoxIconType::QuestionIcon);
    alert->addButton("Save",1); alert->addButton("Discard",2); alert->addButton("Cancel",0,juce::KeyPress(juce::KeyPress::escapeKey));
    alert->enterModalState(true,juce::ModalCallbackFunction::create([safe=juce::Component::SafePointer<PresetBar>(this),action=std::move(action)](int result) mutable {
        if(!safe) return;
        safe->dialogOpen=false;
        if(result==1) { if(safe->presets.selectedIsFactory()) safe->saveAs(std::move(action)); else if(safe->presets.resolveDirty(DirtyAction::save)) action(); }
        else if(result==2&&safe->presets.resolveDirty(DirtyAction::discard)) action();
    }),true);
}
void PresetBar::saveAs(std::function<void()> after) {
    if(dialogOpen||presets.busy()) return;
    dialogOpen=true;
    auto* alert=new juce::AlertWindow("Save As","Name your preset (UTF-8 names supported).",juce::MessageBoxIconType::NoIcon);
    alert->addTextEditor("name",presets.selected()?juce::String::fromUTF8(presets.selected()->name.c_str()):"My rack");
    alert->addButton("Save",1,juce::KeyPress(juce::KeyPress::returnKey)); alert->addButton("Cancel",0,juce::KeyPress(juce::KeyPress::escapeKey));
    alert->enterModalState(true,juce::ModalCallbackFunction::create([safe=juce::Component::SafePointer<PresetBar>(this),alert,after=std::move(after)](int result) mutable {
        if(!safe) return;
        safe->dialogOpen=false;
        if(result==1&&safe->presets.saveAs(alert->getTextEditorContents("name").toStdString())&&after) after();
    }),true);
}
void PresetBar::fileDialog(bool importing) {
    if(dialogOpen||presets.busy()) return;
    dialogOpen=true;
    chooser=std::make_unique<juce::FileChooser>(importing?"Import portable preset":"Export portable preset",juce::File{},"*.json");
    int flags=importing?(juce::FileBrowserComponent::openMode|juce::FileBrowserComponent::canSelectFiles):(juce::FileBrowserComponent::saveMode|juce::FileBrowserComponent::canSelectFiles|juce::FileBrowserComponent::warnAboutOverwriting);
    chooser->launchAsync(flags,[safe=juce::Component::SafePointer<PresetBar>(this),importing](const juce::FileChooser& dialog) {
        if(!safe) return;
        safe->dialogOpen=false; auto file=dialog.getResult(); if(file==juce::File{}) return;
        if(importing) safe->presets.importFile(file.getFullPathName().toStdString()); else safe->presets.exportFile(file.getFullPathName().toStdString());
    });
}
void PresetBar::resized() {
    auto area=getLocalBounds(); auto row=area.removeFromTop(30);
    choices.setBounds(row.removeFromLeft(200));
    for(auto* button:{&saveButton,&saveAsButton,&importButton,&exportButton}) { row.removeFromLeft(5); button->setBounds(row.removeFromLeft(74)); }
    row.removeFromLeft(5); cancelButton.setBounds(row.removeFromLeft(100)); indicator.setBounds(row);
    status.setBounds(area);
}
}
