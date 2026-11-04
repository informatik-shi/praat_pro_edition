// Component integration test: real editors and RichEdit, no injected desktop input.
#include "praat.h"
#include "GuiP.h"
#include "ScriptEditor.h"
#include "LuaEditor.h"
#include "CodeAssist.h"
#include "codefold/CodeFolding.h"
#include "../main/main_Praat.h"
#include <richedit.h>
#include <string>

static void pump(int ms) {
    auto end=GetTickCount64()+ms;
    do {MSG m;while(PeekMessageW(&m,nullptr,0,0,PM_REMOVE)){TranslateMessage(&m);DispatchMessageW(&m);}Sleep(10);}while(GetTickCount64()<end);
}
static HWND ownedTool(HWND editor,const wchar_t *prefix) {
    struct Search{HWND owner,found=nullptr;const wchar_t*prefix;} search{editor,nullptr,prefix};
    EnumThreadWindows(GetCurrentThreadId(),[](HWND w,LPARAM p)->BOOL {
        auto &s=*(Search*)p;wchar_t name[256]{};GetWindowTextW(w,name,256);
        if(GetWindow(w,GW_OWNER)==s.owner&&std::wstring(name).find(s.prefix)==0)s.found=w;
        return TRUE;
    },(LPARAM)&search);return search.found;
}
static std::wstring firstItem(HWND window) {
    HWND list=FindWindowExW(window,nullptr,L"LISTBOX",nullptr);Melder_require(list,U"Missing problem list");
    auto n=SendMessageW(list,LB_GETTEXTLEN,0,0);if(n<0)return L"";
    std::wstring value(n+1,L'\0');SendMessageW(list,LB_GETTEXT,0,(LPARAM)value.data());value.resize(n);return value;
}
static void checkEditor(TextEditor editor,bool lua) {
    HWND text=editor->textWidget->d_widget->window, shell=GetAncestor(text,GA_ROOT);
    ShowWindow(shell,SW_HIDE);
    GuiText_setString(editor->textWidget,lua?U"local x = math.s":U"To Spe");
    CHARRANGE end{-1,-1};SendMessageW(text,EM_EXSETSEL,0,(LPARAM)&end);pump(600);
    // Exercise the shortcut handler without injecting input into the desktop.
    BYTE savedKeys[256]{}, testKeys[256]{};GetKeyboardState(savedKeys);
    testKeys[VK_CONTROL]=0x80;SetKeyboardState(testKeys);
    SendMessageW(text,WM_KEYDOWN,VK_SPACE,0);SendMessageW(text,WM_CHAR,0,0);
    SetKeyboardState(savedKeys);
    HWND popup=ownedTool(shell,L"Completion");Melder_require(popup&&IsWindowVisible(popup),U"Missing completion popup");
    auto proposed=firstItem(popup);Melder_require(proposed==(lua?L"math.sin":L"To Spectrogram"),U"Unexpected first completion");
    SendMessageW(text,WM_KEYDOWN,VK_RETURN,0);SendMessageW(text,WM_CHAR,L'\r',0);
    auto source=GuiText_getString(editor->textWidget);
    Melder_require(str32equ(source.get(),lua?U"local x = math.sin":U"To Spectrogram"),U"Completion insertion failed");
    SendMessageW(text,EM_UNDO,0,0);source=GuiText_getString(editor->textWidget);
    Melder_require(str32equ(source.get(),lua?U"local x = math.s":U"To Spe"),U"Completion undo failed");
    GuiText_setString(editor->textWidget,lua?U"local x=1\nlocal y=)":U"x=1\nendif");pump(900);
    CodeAssist_action(editor->textWidget,1);HWND problems=ownedTool(shell,L"Code problems");
    Melder_require(problems&&firstItem(problems).find(L"Line 2:")==0,U"Missing line 2 diagnostic");
    CodeAssist_action(editor->textWidget,2);CHARRANGE selection;SendMessageW(text,EM_EXGETSEL,0,(LPARAM)&selection);
    Melder_require(selection.cpMin==(lua?10:4),U"Wrong diagnostic navigation offset");
    GuiText_setString(editor->textWidget,lua?U"local x=1\nlocal y=2":U"x=1\nif 1\nendif");pump(900);
    Melder_require(firstItem(problems)==L"No problems found by this check.",U"Stale diagnostics remain after correction");
    // Parser must not execute otherwise valid source.
    if(lua){GuiText_setString(editor->textWidget,U"error('syntax checker executed source')");pump(900);Melder_require(firstItem(problems)==L"No problems found by this check.",U"Checker executed Lua");}
    GuiText_setString(editor->textWidget,lua?U"praat.call(\"To Spectrogram\", ":U"To Spectrogram: ");
    SendMessageW(text,EM_EXSETSEL,0,(LPARAM)&end);CodeAssist_action(editor->textWidget,3);
    popup=ownedTool(shell,L"Parameter hints");Melder_require(popup&&IsWindowVisible(popup),U"Missing parameter hint");
    Melder_require(firstItem(popup)==L"To Spectrogram",U"Parameter hint resolved wrong command");
    SendMessageW(popup,WM_CLOSE,0,0);
    GuiText_setString(editor->textWidget,lua?U"if true then\nlocal x=)\nend":U"if 1\nendfor\nendif");pump(900);
    CodeFolding_action(editor->textWidget,1);
    Melder_require(CodeFolding_hiddenLine(editor->textWidget,1),U"Test block was not folded");
    HWND list=FindWindowExW(problems,nullptr,L"LISTBOX",nullptr);SendMessageW(list,LB_SETCURSEL,0,0);
    SendMessageW(problems,WM_COMMAND,MAKEWPARAM(0,LBN_DBLCLK),(LPARAM)list);
    SendMessageW(text,EM_EXGETSEL,0,(LPARAM)&selection);
    Melder_require(!CodeFolding_hiddenLine(editor->textWidget,1)&&selection.cpMin==(lua?13:5),U"Double-click failed to reveal folded diagnostic");
    editor->dirty=false;
}
int main(int,char**) {
    try {
        char a0[]="code-assist-tests",a1[]="--no-pref-files",a2[]="--no-plugins",a3[]="--utf8";char *args[]={a0,a1,a2,a3};
        praat_init(U"Praat",U"" stringize(PRAAT_VERSION_STR),PRAAT_VERSION_NUM,PRAAT_YEAR,PRAAT_MONTH,PRAAT_DAY,U"",U"",4,args);
        INCLUDE_LIBRARY(praat_uvafon_init)
        {auto editor=LuaEditor_create();checkEditor(editor.get(),true);}
        {auto editor=ScriptEditor_createFromText(nullptr,U"");checkEditor(editor.get(),false);}
        Melder_casual(U"CODE ASSIST EDITOR TESTS: PASS");return 0;
    }catch(MelderError){Melder_casual(Melder_getError());Melder_clearError();return 1;}
}
