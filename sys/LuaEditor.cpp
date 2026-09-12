// Praat Custom. GPL-3.0-or-later.
#include "LuaEditor.h"
#include "LuaRuntime.h"
#include "luadebug/LuaDebugRun.h"
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
enum class LuaDebugAction {Start,Pause,Stop,Over,Into,Out,Toggle,Clear};
static LuaDebugSession *luaDebugger(LuaEditor);
static void luaDebugAction(LuaEditor,LuaDebugAction);
static void luaDebugUpdate(LuaEditor);
static bool luaDebugKey(LuaEditor,unsigned long);
static bool luaDebugDispatch(LuaEditor,unsigned int);

Thing_implement (LuaEditor, TextEditor, 0);
static std::vector<LuaEditor> editors;
static void changed (LuaEditor me, GuiTextEvent) {
    my dirty=true; my v_nameChanged();
}
void structLuaEditor :: v_createChildren () {
    textWidget=GuiText_createShown (windowForm,0,0,Machine_getMenuBarBottom(),
#if defined (_WIN32)
        -210,
#else
        0,
#endif
        GuiText_SCROLLED | GuiText_SCRIPT_IDE);
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
static void execute (LuaEditor me, bool selection, bool check,bool debugging=false) {
    if (my running) return;
    auto source=selection ? GuiText_getSelection(my textWidget) : GuiText_getString(my textWidget);
    if (!source || !source[0]) Melder_throw(U"No Lua code to run.");
    my running=true; my stopRequested=false;
    LuaDebugSession *debugger=debugging?luaDebugger(me):nullptr;
    struct Guard {
        LuaEditor editor;
#if defined (_WIN32)
        HWND shell, text;
        UINT stopCommand=0;
        std::vector<int> disabledMenus;
        std::vector<HWND> disabled;
        static BOOL CALLBACK disableWindow(HWND w,LPARAM p) {
            auto self=reinterpret_cast<Guard*>(p);
            if(w!=self->shell && IsWindowVisible(w) && IsWindowEnabled(w)) {
                self->disabled.push_back(w); EnableWindow(w,FALSE);
            }
            return TRUE;
        }
#endif
        explicit Guard(LuaEditor owner):editor(owner) {
#if defined (_WIN32)
            text=editor->textWidget->d_widget->window; shell=GetAncestor(text,GA_ROOT);
            HMENU menu=GetMenu(shell);
            for(int i=0;i<GetMenuItemCount(menu);++i) {
                wchar_t label[80]{};GetMenuStringW(menu,i,label,80,MF_BYPOSITION);
                if(std::wstring(label).find(L"Run")!=std::wstring::npos)stopCommand=GetMenuItemID(GetSubMenu(menu,i),3);
                else if(std::wstring(label).find(L"Debug")!=std::wstring::npos)continue;
                else if(!(GetMenuState(menu,i,MF_BYPOSITION)&(MF_DISABLED|MF_GRAYED))) {
                    disabledMenus.push_back(i);EnableMenuItem(menu,i,MF_BYPOSITION|MF_GRAYED);
                }
            }
            DrawMenuBar(shell);
            SendMessageW(text,EM_SETREADONLY,TRUE,0);
            EnumThreadWindows(GetCurrentThreadId(),disableWindow,(LPARAM)this);
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
            luaDebugUpdate(editor);
        }
        bool poll() {
#if defined (_WIN32)
            MSG msg; int count=0;
            while(count++<100 && PeekMessageW(&msg,nullptr,0,0,PM_REMOVE)) {
                if(msg.message==WM_QUIT) {editor->stopRequested=true;PostQuitMessage((int)msg.wParam);break;}
                if((msg.message==WM_KEYDOWN || msg.message==WM_SYSKEYDOWN) && luaDebugKey(editor,(unsigned long)msg.wParam))continue;
                if(msg.message==WM_CLOSE && msg.hwnd==shell) {luaDebugAction(editor,LuaDebugAction::Stop);continue;}
                bool ours=msg.hwnd==shell || IsChild(shell,msg.hwnd);
                if(ours && msg.message==WM_COMMAND) {
                    if(!msg.lParam&&luaDebugDispatch(editor,LOWORD(msg.wParam)))continue;
                    if(LOWORD(msg.wParam)==stopCommand&&!msg.lParam)luaDebugAction(editor,LuaDebugAction::Stop);
                    else if(msg.lParam)DispatchMessageW(&msg);
                    continue;
                }
                // Only this editor receives input; its source is read-only and all
                // menus except Run and Debug are disabled. Run callbacks reject re-entry.
                if(ours) {TranslateMessage(&msg);DispatchMessageW(&msg);}
                else if(msg.message==WM_PAINT || msg.message==WM_NCPAINT || msg.message==WM_ERASEBKGND)DispatchMessageW(&msg);
            }
#endif
            return !editor->stopRequested;
        }
    } guard(me);
    if(debugger) {
        struct CallbackGuard {LuaDebugSession *session;~CallbackGuard(){session->paused={};}} callbackGuard{debugger};
        debugger->paused=[&](LuaDebugSession &session) {
            luaDebugUpdate(me);
            while(session.state==LuaDebugState::Paused) {
                if(!guard.poll()){session.stop();break;}
#if defined (_WIN32)
                MsgWaitForMultipleObjects(0,nullptr,FALSE,20,QS_ALLINPUT);
#endif
            }
            luaDebugUpdate(me);
        };
        try {Lua_runDebug(source.get(),&my file,[&](){bool go=guard.poll();if(!go)debugger->stop();return go;},*debugger);}
        catch(MelderError){luaDebugUpdate(me);throw;}
        luaDebugUpdate(me);
    } else Lua_run(source.get(),&my file,check,[&guard](){return guard.poll();});
    if(check) Melder_information(U"Lua syntax OK.");
}
static void run(LuaEditor me,EDITOR_ARGS) {luaDebugAction(me,LuaDebugAction::Start);}
static void runPlain(LuaEditor me,EDITOR_ARGS) {execute(me,false,false);}
static void runSelection(LuaEditor me,EDITOR_ARGS) {execute(me,true,false);}
static void check(LuaEditor me,EDITOR_ARGS) {execute(me,false,true);}
static void stop(LuaEditor me,EDITOR_ARGS) {luaDebugAction(me,LuaDebugAction::Stop);}
static void debugPause(LuaEditor me,EDITOR_ARGS) {luaDebugAction(me,LuaDebugAction::Pause);}
static void debugOver(LuaEditor me,EDITOR_ARGS) {luaDebugAction(me,LuaDebugAction::Over);}
static void debugInto(LuaEditor me,EDITOR_ARGS) {luaDebugAction(me,LuaDebugAction::Into);}
static void debugOut(LuaEditor me,EDITOR_ARGS) {luaDebugAction(me,LuaDebugAction::Out);}
static void debugToggle(LuaEditor me,EDITOR_ARGS) {luaDebugAction(me,LuaDebugAction::Toggle);}
static void debugClear(LuaEditor me,EDITOR_ARGS) {luaDebugAction(me,LuaDebugAction::Clear);}
void structLuaEditor :: v_createMenus () {
    LuaEditor_Parent::v_createMenus();
    Editor_addMenu(this,U"Run",0);
    Editor_addCommand(this,U"Run",U"Run Lua / Continue",GuiMenu_F5,run);
    Editor_addCommand(this,U"Run",U"Run selection",'T',runSelection);
    Editor_addCommand(this,U"Run",U"Check syntax",GuiMenu_F7,check);
    Editor_addCommand(this,U"Run",U"Stop",GuiMenu_SHIFT | GuiMenu_F5,stop);
    Editor_addCommand(this,U"Run",U"Run without debugger",'R',runPlain);
#if defined (_WIN32)
    Editor_addMenu(this,U"Debug",0);
    Editor_addCommand(this,U"Debug",U"Start / Continue",0,run);
    Editor_addCommand(this,U"Debug",U"Pause",GuiMenu_F6,debugPause);
    Editor_addCommand(this,U"Debug",U"Stop",0,stop);
    Editor_addCommand(this,U"Debug",U"Step Over",GuiMenu_F10,debugOver);
    Editor_addCommand(this,U"Debug",U"Step Into",GuiMenu_F11,debugInto);
    Editor_addCommand(this,U"Debug",U"Step Out",GuiMenu_SHIFT|GuiMenu_F11,debugOut);
    Editor_addCommand(this,U"Debug",U"Toggle Breakpoint",GuiMenu_F9,debugToggle);
    Editor_addCommand(this,U"Debug",U"Remove All Breakpoints",0,debugClear);
#endif
}
autoLuaEditor LuaEditor_create () {
    autoLuaEditor me=Thing_new(LuaEditor);
    TextEditor_init(me.get(),U"-- Lua 5.5 + Praat API\nlocal ids = praat.call(\"Create Sound from formula\",\n    \"tone\", 1, 0, 1, 44100, \"0.2*sin(2*pi*440*x)\")\npraat.select(ids)\nprint(\"Duration:\", praat.call(\"Get total duration\"))\n");
    editors.push_back(me.get());
    my v_nameChanged();
    return me;
}
#include "LuaEditor_win.inc"
