// Praat Pro Edition. GPL-3.0-or-later. Included only by TextGridEditor.cpp.
#if defined(_WIN32)
#include "GuiP.h"
#include "Model.h"
#include <commctrl.h>
#include <filesystem>
#include <fstream>
#include <memory>

namespace {
using AnnotationKeys::Key;
struct AnnotationAction {AnnotationKeys::Action description;EditorCommand command;};
Thing_define(AnnotationHotkeys,Thing) {
    TextGridEditor editor=nullptr;
    std::vector<AnnotationAction> actions;
    std::vector<size_t> visible;
    std::string selected;
    GuiDialog dialog=nullptr;
    GuiList list=nullptr;
    GuiText search=nullptr,shortcut=nullptr;
    GuiLabel selection=nullptr,defaultLabel=nullptr,status=nullptr;
    bool updating=false;
};
Thing_implement(AnnotationHotkeys,Thing,0);
std::map<TextGridEditor,autoAnnotationHotkeys> annotationWindows;
AnnotationKeys::Document annotationDocument;
std::string annotationStatus;
bool annotationLoaded=false;
constexpr UINT_PTR annotationSubclassId=8301;

std::string annotationUTF8(conststring32 s){return Melder_peek32to8(s);}
std::filesystem::path annotationPath(){
    wchar_t buffer[32768];DWORD n=GetEnvironmentVariableW(L"PRAAT_TEXTGRID_SHORTCUTS_INI",buffer,32768);
    if(n&&n<32768)return std::filesystem::absolute(buffer);
    n=GetModuleFileNameW(nullptr,buffer,32768);
    if(!n||n>=32768)throw std::runtime_error("Cannot locate the Praat executable.");
    return std::filesystem::path(buffer).parent_path()/L"praat-textgrid-shortcuts.ini";
}
AnnotationKeys::Document annotationRead(){
    auto file=annotationPath();std::ifstream input(file);
    if(input)return AnnotationKeys::Document::read(input);
    if(std::filesystem::exists(file))throw std::runtime_error("Cannot read praat-textgrid-shortcuts.ini.");
    AnnotationKeys::Document result;
    result.lines={"; Praat Pro Edition: TextGrid / Sound annotation shortcuts (UTF-8)",
        "; Missing commands retain their Praat defaults. None disables a menu shortcut.",
        "; Configure in TextGrid > Keyboard shortcuts, or edit and Reload.","[TextGridEditor.Shortcuts]"};
    return result;
}
void annotationWrite(const AnnotationKeys::Document &document){
    auto file=annotationPath(),temporary=file;
    temporary+=L"."+std::to_wstring(GetCurrentProcessId())+L"."+std::to_wstring(GetTickCount64())+L".tmp";
    try {
        {std::ofstream output(temporary,std::ios::binary);if(!output)throw std::runtime_error("Cannot save shortcut INI beside the executable. Check write access or set PRAAT_TEXTGRID_SHORTCUTS_INI.");
            document.write(output);output.flush();if(!output)throw std::runtime_error("Cannot finish writing shortcut INI.");}
        if(!MoveFileExW(temporary.c_str(),file.c_str(),MOVEFILE_REPLACE_EXISTING|MOVEFILE_WRITE_THROUGH))
            throw std::runtime_error("Cannot replace shortcut INI. Existing settings have been kept.");
    }catch(...){std::error_code ignored;std::filesystem::remove(temporary,ignored);throw;}
}
std::vector<AnnotationKeys::Action> annotationCatalogue(AnnotationHotkeys me){
    std::vector<AnnotationKeys::Action> result;for(auto &a:my actions)result.push_back(a.description);return result;
}
AnnotationAction *annotationFind(AnnotationHotkeys me,const std::string &id){
    for(auto &a:my actions)if(a.description.id==id)return &a;return nullptr;
}
Key annotationActual(const AnnotationAction &action){
    auto w=action.command->itemWidget->d_widget;
    return {(int)w->motiff.pushButton.acceleratorChar,w->motiff.pushButton.acceleratorModifiers};
}
void annotationRefresh(AnnotationHotkeys me);
void annotationApply(AnnotationHotkeys me){
    auto shell=my editor->windowForm->d_widget->shell;
    // Rebuild masks, including unchanged menu items. Leaving a removed Enter/Tab bit
    // behind would redirect text entry to a command that no longer exists.
    for(auto &mask:shell->motiff.shell.lowAccelerators)mask=0;
    for(auto &a:my actions){
        Key key=annotationDocument.effective(a.description);
        auto widget=a.command->itemWidget->d_widget;
        widget->motiff.pushButton.acceleratorChar=(unsigned char)key.code;
        widget->motiff.pushButton.acceleratorModifiers=key.modifiers;
        MENUITEMINFOW preserved{};preserved.cbSize=sizeof preserved;preserved.fMask=MIIM_STATE;
        GetMenuItemInfoW(widget->nat.entry.handle,widget->nat.entry.id,FALSE,&preserved);
        _GuiWinMenuItem_setText(widget);
        SetMenuItemInfoW(widget->nat.entry.handle,widget->nat.entry.id,FALSE,&preserved);
        if(key.code>0&&key.code<32)shell->motiff.shell.lowAccelerators[key.modifiers]|=uint32(1)<<key.code;
    }
    DrawMenuBar(shell->window);
}
void annotationCommit(AnnotationKeys::Document next,bool save){
    for(auto &entry:annotationWindows)next.validate(annotationCatalogue(entry.second.get()));
    if(save)annotationWrite(next);
    annotationDocument=std::move(next);annotationStatus=save?"Saved. Applied to all open TextGrid editors.":"Reloaded from INI.";
    for(auto &entry:annotationWindows){annotationApply(entry.second.get());annotationRefresh(entry.second.get());}
}
void annotationAssign(AnnotationHotkeys me,const std::string &id,const std::string &combination,bool useDefault=false){
    auto action=annotationFind(me,id);
    if(!action)throw std::runtime_error("Unknown command. Use its Menu/Command identifier from Keyboard shortcuts.");
    auto next=annotationRead(); // also preserves external edits and comments
    next.change(id,useDefault?action->description.defaultKey:AnnotationKeys::parse(combination),useDefault);
    annotationCommit(std::move(next),true);
}
void annotationSelection(AnnotationHotkeys me,GuiList_SelectionChangedEvent){
    if(my updating)return;
    auto positions=GuiList_getSelectedPositions(my list);if(positions.size!=1)return;
    auto &action=my actions[my visible[(size_t)positions[1]-1]];
    my selected=action.description.id;
    GuiLabel_setText(my selection,Melder_peek8to32_u(my selected.c_str()));
    GuiLabel_setText(my defaultLabel,Melder_peek8to32_u(("Praat default: "+AnnotationKeys::format(action.description.defaultKey)).c_str()));
    GuiText_setString(my shortcut,Melder_peek8to32_u(AnnotationKeys::format(annotationActual(action)).c_str()));
}
void annotationRefresh(AnnotationHotkeys me){
    if(!my dialog)return;
    my updating=true;
    auto text=GuiText_getString(my search);auto filter=AnnotationKeys::lower(annotationUTF8(text.get()));
    GuiList_deleteAllItems(my list);my visible.clear();integer selected=0;
    for(size_t i=0;i<my actions.size();++i){auto &a=my actions[i];
        if(AnnotationKeys::lower(a.description.id).find(filter)==std::string::npos)continue;
        my visible.push_back(i);
        std::string row=a.description.id+"    ["+AnnotationKeys::format(annotationActual(a))+"]";
        GuiList_insertItem(my list,Melder_peek8to32_u(row.c_str()),0);
        if(a.description.id==my selected)selected=(integer)my visible.size();
    }
    if(!selected&&!my visible.empty())selected=1;
    if(selected)GuiList_selectItem(my list,selected);
    my updating=false;
    if(selected)annotationSelection(me,nullptr);
    else {my selected.clear();GuiLabel_setText(my selection,U"No matching command");GuiLabel_setText(my defaultLabel,U"");GuiText_setString(my shortcut,U"");}
    GuiLabel_setText(my status,Melder_peek8to32_u(annotationStatus.c_str()));
}
void annotationSearch(AnnotationHotkeys me,GuiTextEvent){annotationRefresh(me);}
void annotationClose(AnnotationHotkeys me){GuiThing_hide(my dialog);}
void annotationCloseButton(AnnotationHotkeys me,GuiButtonEvent){annotationClose(me);}
void annotationError(AnnotationHotkeys me,const std::exception &e){GuiLabel_setText(my status,Melder_peek8to32_u(e.what()));}
void annotationAssignButton(AnnotationHotkeys me,GuiButtonEvent){
    try{auto key=GuiText_getString(my shortcut);annotationAssign(me,my selected,annotationUTF8(key.get()));}
    catch(const std::exception &e){annotationError(me,e);}
}
void annotationDisableButton(AnnotationHotkeys me,GuiButtonEvent){
    try{annotationAssign(me,my selected,"None");}catch(const std::exception &e){annotationError(me,e);}
}
void annotationDefaultButton(AnnotationHotkeys me,GuiButtonEvent){
    try{annotationAssign(me,my selected,"",true);}catch(const std::exception &e){annotationError(me,e);}
}
void annotationReloadButton(AnnotationHotkeys me,GuiButtonEvent){
    try{annotationCommit(annotationRead(),false);}catch(const std::exception &e){annotationError(me,e);}
}
void annotationShow(AnnotationHotkeys me){
    if(!my dialog){
        my dialog=GuiDialog_create(my editor->windowForm,100,100,850,560,U"TextGrid annotation: keyboard shortcuts",annotationClose,me,GuiDialog_Modality::MODELESS);
        auto form=my dialog;
        GuiLabel_createShown(form,12,838,10,46,Melder_cat(U"INI: ",Melder_peekWto32(annotationPath().c_str())),0);
        GuiLabel_createShown(form,12,65,54,78,U"Filter:",0);
        my search=GuiText_createShown(form,72,838,52,78,0);
        GuiText_setChangedCallback(my search,annotationSearch,me);
        my list=GuiList_createShown(form,12,838,86,334,false,U"Command and current shortcut");
        GuiList_setSelectionChangedCallback(my list,annotationSelection,me);
        my selection=GuiLabel_createShown(form,12,838,342,366,U"",0);
        my defaultLabel=GuiLabel_createShown(form,12,838,370,394,U"",0);
        GuiLabel_createShown(form,12,125,400,424,U"New shortcut:",0);
        my shortcut=GuiText_createShown(form,130,430,398,424,0);
        GuiButton_createShown(form,445,540,398,426,U"Assign",annotationAssignButton,me,0);
        GuiButton_createShown(form,550,645,398,426,U"Disable",annotationDisableButton,me,0);
        GuiButton_createShown(form,655,760,398,426,U"Default",annotationDefaultButton,me,0);
        my status=GuiLabel_createShown(form,12,838,434,480,U"",0);
        GuiLabel_createShown(form,12,838,484,508,U"Type F4, Ctrl+Enter, Ctrl+Shift+B, Alt+Right, etc. Letters need Ctrl or Alt.",0);
        GuiButton_createShown(form,12,170,518,546,U"Reload INI",annotationReloadButton,me,0);
        GuiButton_createShown(form,740,838,518,546,U"Close",annotationCloseButton,me,0);
    }
    annotationRefresh(me);GuiThing_show(my dialog);
}
AnnotationHotkeys annotationState(TextGridEditor me){
    auto found=annotationWindows.find(me);Melder_require(found!=annotationWindows.end(),U"Shortcut settings are not available yet.");return found->second.get();
}
void menu_cb_annotationKeys(TextGridEditor me,EDITOR_ARGS){annotationShow(annotationState(me));}
void menu_cb_setAnnotationKey(TextGridEditor me,EDITOR_ARGS){
    EDITOR_FORM(U"Set TextGrid keyboard shortcut",nullptr)
        SENTENCE(action,U"Command (Menu/Command)",U"Boundary/Add on selected tier")
        SENTENCE(shortcut,U"Shortcut",U"Ctrl+Enter")
    EDITOR_OK
    EDITOR_DO
        try{annotationAssign(annotationState(me),annotationUTF8(action),annotationUTF8(shortcut));}
        catch(const std::exception &e){Melder_throw(Melder_peek8to32_u(e.what()));}
    EDITOR_END
}
void menu_cb_getAnnotationKey(TextGridEditor me,EDITOR_ARGS){
    EDITOR_FORM(U"Get TextGrid keyboard shortcut",nullptr)
        SENTENCE(action,U"Command (Menu/Command)",U"Boundary/Add on selected tier")
    EDITOR_OK
    EDITOR_DO
        auto a=annotationFind(annotationState(me),annotationUTF8(action));Melder_require(a,U"Unknown annotation command.");
        conststring32 result=Melder_peek8to32_u(AnnotationKeys::format(annotationActual(*a)).c_str());
        QUERY_EDITOR_FOR_STRING_END
    EDITOR_END
}
void menu_cb_reloadAnnotationKeys(TextGridEditor,EDITOR_ARGS){
    try{annotationCommit(annotationRead(),false);}catch(const std::exception &e){Melder_throw(Melder_peek8to32_u(e.what()));}
}
LRESULT CALLBACK annotationWindowProc(HWND window,UINT message,WPARAM w,LPARAM l,UINT_PTR,DWORD_PTR reference){
    auto me=reinterpret_cast<AnnotationHotkeys>(reference);
    if(message==WM_NCDESTROY){
        if(my dialog){auto shell=my dialog->d_xmShell;my dialog=nullptr;GuiObject_destroy(shell);}
        RemovePropW(window,L"Praat.AnnotationShortcuts");RemoveWindowSubclass(window,annotationWindowProc,annotationSubclassId);
        annotationWindows.erase(my editor);
    }
    return DefSubclassProc(window,message,w,l);
}
void TextGridHotkeys_install(TextGridEditor editor){
    auto me=Thing_new(AnnotationHotkeys);my editor=editor;my selected="Boundary/Add on selected tier";
    for(integer i=1;i<=editor->menus.size;++i){auto menu=editor->menus.at[i];
        for(integer j=1;j<=menu->commands.size;++j){auto command=menu->commands.at[j];
            if(!command->itemWidget||!command->commandCallback||!command->itemWidget->d_widget)continue;
            auto widget=command->itemWidget->d_widget;
            my actions.push_back({{annotationUTF8(menu->menuTitle.get())+"/"+annotationUTF8(command->itemTitle.get()),
                {(int)widget->motiff.pushButton.acceleratorChar,widget->motiff.pushButton.acceleratorModifiers}},command});
        }
    }
    std::sort(my actions.begin(),my actions.end(),[](const AnnotationAction &a,const AnnotationAction &b){return a.description.id<b.description.id;});
    annotationWindows.emplace(editor,std::move(me));
    auto state=annotationState(editor);
    auto window=editor->windowForm->d_widget->shell->window;
    if(!SetWindowSubclass(window,annotationWindowProc,annotationSubclassId,reinterpret_cast<DWORD_PTR>(state))){
        annotationWindows.erase(editor);Melder_throw(U"Cannot attach annotation shortcut settings to this window.");
    }
    SetPropW(window,L"Praat.AnnotationShortcuts",reinterpret_cast<HANDLE>(1));
    if(!annotationLoaded){annotationLoaded=true;
        try{annotationCommit(annotationRead(),false);}catch(const std::exception &e){annotationStatus=std::string("INI not loaded; using defaults. ")+e.what();}
    }else{try{annotationDocument.validate(annotationCatalogue(state));annotationApply(state);}catch(const std::exception &e){annotationStatus=std::string("New editor keeps defaults: ")+e.what();}}
    Editor_addCommand(editor,U"TextGrid",U"-- annotation shortcuts --",0,nullptr);
    Editor_addCommand(editor,U"TextGrid",U"Keyboard shortcuts...",0,menu_cb_annotationKeys);
    Editor_addCommand(editor,U"TextGrid",U"Set keyboard shortcut...",GuiMenu_HIDDEN,menu_cb_setAnnotationKey);
    Editor_addCommand(editor,U"TextGrid",U"Get keyboard shortcut...",GuiMenu_HIDDEN,menu_cb_getAnnotationKey);
    Editor_addCommand(editor,U"TextGrid",U"Reload keyboard shortcuts",0,menu_cb_reloadAnnotationKeys);
}
}
#else
static void TextGridHotkeys_install(TextGridEditor){}
#endif
