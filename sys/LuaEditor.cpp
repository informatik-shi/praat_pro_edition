// Praat Custom. GPL-3.0-or-later.
#include "LuaEditor.h"
#include "LuaRuntime.h"
#include "EditorM.h"
#include "machine.h"
#include <algorithm>
#include <vector>
#if defined (_WIN32)
#include "GuiP.h"
#include <richedit.h>
#endif
static void createLuaView (LuaEditor);
static void destroyLuaView (LuaEditor);

Thing_implement (LuaEditor, TextEditor, 0);
static std::vector<LuaEditor> editors;
static void changed (LuaEditor me, GuiTextEvent) {
    my dirty=true; my v_nameChanged();
}
void structLuaEditor :: v_createChildren () {
    textWidget=GuiText_createShown (windowForm,0,0,Machine_getMenuBarBottom(),0,GuiText_SCROLLED | GuiText_SCRIPT_IDE);
    GuiText_setChangedCallback(textWidget,changed,this);
    createLuaView(this);
}
void structLuaEditor :: v_nameChanged () {
    GuiShell_setTitle (windowForm, Melder_cat (U"Lua Editor — ",
        MelderFile_isNull(&file) ? U"untitled.lua" : MelderFile_peekPath(&file), dirty ? U" *" : U""));
}
void structLuaEditor :: v_goAway () {
    if (running) { stopRequested=true; return; }
    LuaEditor_Parent::v_goAway();
}
void structLuaEditor :: v9_destroy () noexcept {
    destroyLuaView(this);
    editors.erase(std::remove(editors.begin(),editors.end(),this),editors.end());
    LuaEditor_Parent::v9_destroy();
}
bool LuaEditors_dirty () {
    for (auto editor:editors) if(editor->dirty || editor->running) return true;
    return false;
}
static void execute (LuaEditor me, bool selection, bool check) {
    if (my running) return;
    auto source=selection ? GuiText_getSelection(my textWidget) : GuiText_getString(my textWidget);
    if (!source || !source[0]) Melder_throw(U"No Lua code to run.");
    my running=true; my stopRequested=false;
    struct Guard {
        LuaEditor editor;
#if defined (_WIN32)
        HWND shell, text;
        UINT stopCommand=0;
        std::vector<int> disabledMenus;
        std::vector<HWND> disabled;
#endif
        explicit Guard(LuaEditor owner):editor(owner) {
#if defined (_WIN32)
            text=editor->textWidget->d_widget->window; shell=GetAncestor(text,GA_ROOT);
            HMENU menu=GetMenu(shell);
            for(int i=0;i<GetMenuItemCount(menu);++i) {
                wchar_t label[80]{};GetMenuStringW(menu,i,label,80,MF_BYPOSITION);
                if(std::wstring(label).find(L"Run")!=std::wstring::npos)stopCommand=GetMenuItemID(GetSubMenu(menu,i),3);
                else if(!(GetMenuState(menu,i,MF_BYPOSITION)&(MF_DISABLED|MF_GRAYED))) {
                    disabledMenus.push_back(i);EnableMenuItem(menu,i,MF_BYPOSITION|MF_GRAYED);
                }
            }
            DrawMenuBar(shell);
            SendMessageW(text,EM_SETREADONLY,TRUE,0);
            EnumThreadWindows(GetCurrentThreadId(),[](HWND w,LPARAM p)->BOOL {
                auto self=reinterpret_cast<Guard*>(p);
                if(w!=self->shell && IsWindowVisible(w) && IsWindowEnabled(w)) {
                    self->disabled.push_back(w); EnableWindow(w,FALSE);
                } return TRUE;
            },(LPARAM)this);
#endif
        }
        ~Guard() {
#if defined (_WIN32)
            SendMessageW(text,EM_SETREADONLY,FALSE,0);
            for(int i:disabledMenus)EnableMenuItem(GetMenu(shell),i,MF_BYPOSITION|MF_ENABLED);
            DrawMenuBar(shell);
            for(HWND w:disabled) if(IsWindow(w))EnableWindow(w,TRUE);
#endif
            editor->running=false;
        }
        bool poll() {
#if defined (_WIN32)
            MSG msg; int count=0;
            while(count++<100 && PeekMessageW(&msg,nullptr,0,0,PM_REMOVE)) {
                if(msg.message==WM_QUIT) {editor->stopRequested=true;PostQuitMessage((int)msg.wParam);break;}
                if(msg.message==WM_KEYDOWN && msg.wParam==VK_F5 && GetKeyState(VK_SHIFT)<0) {editor->stopRequested=true;continue;}
                if(msg.message==WM_CLOSE && msg.hwnd==shell) {editor->stopRequested=true;continue;}
                bool ours=msg.hwnd==shell || IsChild(shell,msg.hwnd);
                if(ours && msg.message==WM_COMMAND) {
                    if(LOWORD(msg.wParam)==stopCommand)editor->stopRequested=true;
                    continue;
                }
                // Only this editor receives input; its source is read-only and all
                // menus except Run are disabled. Run callbacks reject re-entry.
                if(ours) {TranslateMessage(&msg);DispatchMessageW(&msg);}
                else if(msg.message==WM_PAINT || msg.message==WM_NCPAINT || msg.message==WM_ERASEBKGND)DispatchMessageW(&msg);
            }
#endif
            return !editor->stopRequested;
        }
    } guard(me);
    Lua_run(source.get(),&my file,check,[&guard](){return guard.poll();});
    if(check) Melder_information(U"Lua syntax OK.");
}
static void run(LuaEditor me,EDITOR_ARGS) {execute(me,false,false);}
static void runSelection(LuaEditor me,EDITOR_ARGS) {execute(me,true,false);}
static void check(LuaEditor me,EDITOR_ARGS) {execute(me,false,true);}
static void stop(LuaEditor me,EDITOR_ARGS) {my stopRequested=true;}
void structLuaEditor :: v_createMenus () {
    LuaEditor_Parent::v_createMenus();
    Editor_addMenu(this,U"Run",0);
    Editor_addCommand(this,U"Run",U"Run Lua",GuiMenu_F5,run);
    Editor_addCommand(this,U"Run",U"Run selection",'T',runSelection);
    Editor_addCommand(this,U"Run",U"Check syntax",GuiMenu_F7,check);
    Editor_addCommand(this,U"Run",U"Stop",GuiMenu_SHIFT | GuiMenu_F5,stop);
}
autoLuaEditor LuaEditor_create () {
    autoLuaEditor me=Thing_new(LuaEditor);
    TextEditor_init(me.get(),U"-- Lua 5.5 + Praat API\nlocal ids = praat.call(\"Create Sound from formula\",\n    \"tone\", 1, 0, 1, 44100, \"0.2*sin(2*pi*440*x)\")\npraat.select(ids)\nprint(\"Duration:\", praat.call(\"Get total duration\"))\n");
    editors.push_back(me.get());
    my v_nameChanged();
    return me;
}
#include "LuaEditor_win.inc"
