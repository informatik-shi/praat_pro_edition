// Praat Custom Script IDE. GPL-3.0-or-later.
#include "WinScriptEditor.h"
#include "ScriptEditor.h"
#include "ScriptDebugger.h"
#include "ScriptSyntax.h"
#if defined (_WIN32)
#include "GuiP.h"
#include <richedit.h>
#include <richole.h>
#include <tom.h>
#include <map>
#include <fstream>
#include <filesystem>
#include <chrono>

namespace {
struct EditorTheme {
    COLORREF text=RGB(32,35,40), background=RGB(255,255,255), gutter=RGB(242,244,247),
        number=RGB(103,111,123), breakpoint=RGB(190,45,50), execution=RGB(255,239,167);
    COLORREF colour (ScriptToken token) const {
        switch(token) {
            case ScriptToken::Keyword: return RGB(28,69,179);
            case ScriptToken::Form: return RGB(115,55,150);
            case ScriptToken::Comment: return RGB(58,120,69);
            case ScriptToken::String: return RGB(155,55,37);
            case ScriptToken::Number: return RGB(126,57,150);
            case ScriptToken::StringVariable: return RGB(147,78,28);
            case ScriptToken::Array: return RGB(12,111,122);
            case ScriptToken::Function: return RGB(0,99,130);
            case ScriptToken::Procedure: return RGB(139,40,146);
            case ScriptToken::Command: return RGB(18,93,128);
            default: return text;
        }
    }
};
std::wstring wide (const std::u32string &s) { return Melder_peek32toW(s.c_str()); }
std::wstring nativeText (HWND window) {
    int n=GetWindowTextLengthW(window);
    std::wstring result(n+1,L'\0'); GetWindowTextW(window,result.data(),n+1); result.resize(n);return result;
}
struct Anchor { ITextRange *range=nullptr; long line=0; };
struct WinIDE final : ScriptDebugger {
    ScriptEditor editor;
    HWND text=nullptr, shell=nullptr, host=nullptr, gutter=nullptr, panel=nullptr, view=nullptr, watchInput=nullptr, status=nullptr;
    HWND tabs[4] {}, add=nullptr, remove=nullptr;
    HFONT font=nullptr;
    ITextDocument *document=nullptr;
    EditorTheme theme;
    std::vector<Anchor> anchors;
    std::vector<std::u32string> watches;
    std::vector<HWND> disabledWindows;
    std::wstring filePath;
    int tab=0;
    bool formatting=false, launching=false;
    long shownLine=0, oldFirst=-1, oldCaret=-1, oldCount=-1;
    std::map<long,std::pair<std::wstring,bool>> coloured;
    ULONGLONG lastPoll=0;
    explicit WinIDE (ScriptEditor owner):editor(owner) {}
    ~WinIDE () override {
        save();
        for(auto &a:anchors) if(a.range) a.range->Release();
        if(document) document->Release();
        if(font) DeleteObject(font);
    }
    long lineStart (long line) { return (long)SendMessageW(text,EM_LINEINDEX,line-1,0); }
    long caretLine () {
        CHARRANGE s; SendMessageW(text,EM_EXGETSEL,0,(LPARAM)&s);
        return (long)SendMessageW(text,EM_EXLINEFROMCHAR,0,s.cpMin)+1;
    }
    std::wstring lineText (long line) {
        long start=lineStart(line); if(start<0)return L"";
        long length=(long)SendMessageW(text,EM_LINELENGTH,start,0);
        std::wstring result(std::min<long>(length,65534)+2,L'\0');
        *reinterpret_cast<WORD*>(result.data())=(WORD)(result.size()-1);
        auto n=SendMessageW(text,EM_GETLINE,line-1,(LPARAM)result.data());
        result.resize(n);return result;
    }
    void go (long line) {
        long start=lineStart(line);if(start<0)return;
        CHARRANGE selection {start,start};SendMessageW(text,EM_EXSETSEL,0,(LPARAM)&selection);
        SendMessageW(text,EM_SCROLLCARET,0,0); coloured.clear(); refresh();
    }
    void syncAnchors () {
        breakpoints.clear();
        for(auto &a:anchors) {
            long position=0;
            if(a.range&&SUCCEEDED(a.range->GetStart(&position))) a.line=(long)SendMessageW(text,EM_EXLINEFROMCHAR,0,position)+1;
            breakpoints.insert(a.line);
        }
    }
    void toggle (long line) {
        syncAnchors();
        for(auto it=anchors.begin();it!=anchors.end();++it) if(it->line==line) {
            if(it->range)it->range->Release();anchors.erase(it);syncAnchors();save();InvalidateRect(gutter,nullptr,FALSE);return;
        }
        Anchor a; a.line=line;
        long start=lineStart(line);
        if(start<0)return;
        // Anchor inside the line so inserted preceding newlines follow the original code.
        if(!lineText(line).empty()) ++start;
        if(document)document->Range(start,start,&a.range);
        anchors.push_back(a);syncAnchors();save();InvalidateRect(gutter,nullptr,FALSE);
    }
    std::filesystem::path storage () {
        wchar_t root[32768]; DWORD n=GetEnvironmentVariableW(L"LOCALAPPDATA",root,32768);
        if(!n||n>=32768||filePath.empty())return {};
        unsigned long long hash=1469598103934665603ULL;
        for(wchar_t c:filePath) {hash^=towlower(c);hash*=1099511628211ULL;}
        return std::filesystem::path(root)/L"PraatCustom"/L"breakpoints"/(std::to_wstring(hash)+L".txt");
    }
    void save () noexcept {
        try {
            auto path=storage();if(path.empty())return;
            std::filesystem::create_directories(path.parent_path());
            std::ofstream out(path);for(auto &a:anchors)out<<a.line<<'\n';
        } catch(...) {} // optional persistence must never break editing or teardown
    }
    void fileChanged () {
        std::wstring path=Melder_peek32toW(MelderFile_peekPath(&editor->file));
        if(path==filePath)return;
        save();filePath=path;
        if(!active()){current=nullptr;shownLine=0;error.clear();state=ScriptDebugState::Idle;showPanel();}
        for(auto &a:anchors)if(a.range)a.range->Release();anchors.clear();breakpoints.clear();
        try {
            std::ifstream in(storage());long line;
            std::vector<long> restored;while(in>>line)if(line>0&&lineStart(line)>=0)restored.push_back(line);
            for(auto n:restored)toggle(n);
        }catch(...){}
        coloured.clear();
    }
    void layout () {
        RECT r;GetClientRect(host,&r);
        int height=220;
        SetWindowPos(panel,nullptr,0,r.bottom-height,r.right,height,SWP_NOZORDER|SWP_NOACTIVATE);
        RECT tr;GetClientRect(text,&tr);
        SetWindowPos(gutter,HWND_TOP,0,0,64,tr.bottom,SWP_NOACTIVATE);
        for(int i=0;i<4;++i)MoveWindow(tabs[i],4+i*100,3,98,24,TRUE);
        MoveWindow(view,4,31,std::max(1L,r.right-8),height-89,TRUE);
        MoveWindow(watchInput,4,height-54,std::max(1L,r.right-202),24,TRUE);
        MoveWindow(add,std::max(1L,r.right-194),height-54,92,24,TRUE);
        MoveWindow(remove,std::max(1L,r.right-100),height-54,96,24,TRUE);
        MoveWindow(status,4,height-27,std::max(1L,r.right-8),23,TRUE);
    }
    void showPanel () {
        std::u32string result;
        if(tab==0)result=ScriptDebugger_variables(current);
        if(tab==1) {
            result=U"Expression\tValue\n";
            for(auto &w:watches)result+=w+U"\t"+(state==ScriptDebugState::Paused?ScriptDebugger_watch(current,w):U"<pause to evaluate>")+U"\n";
        }
        if(tab==2)result=ScriptDebugger_stack(current);
        if(tab==3) {
            if(state==ScriptDebugState::Error) {
                result=U"Runtime error\nFile: "; result+=MelderFile_peekPath(&editor->file);
                result+=U"\nLine: "+std::u32string(Melder_integer(currentLine))+U"\nProcedure: ";
                result+=current&&current->callDepth?current->procedureStackNames[current->callDepth]:U"main";
                result+=U"\n\n"+error;
            } else {
                result=U"Debugger: "+std::u32string(state==ScriptDebugState::Finished?U"Finished":state==ScriptDebugState::Paused?U"Paused":state==ScriptDebugState::Running?U"Running":U"Idle");
                result+=U"\nPraat Info (last 32000 characters):\n";
                const char32 *info=Melder_getInfo();
                if(info){integer length=Melder_length(info);result+=info+std::max<integer>(0,length-32000);}
            }
        }
        std::wstring s=wide(result), crlf;
        for(auto c:s){if(c==L'\n')crlf+=L'\r';crlf+=c;}
        SetWindowTextW(view,crlf.c_str());
        for(int i=0;i<4;++i)SendMessageW(tabs[i],BM_SETSTATE,tab==i,0);
    }
    void changed () override {
        if(state==ScriptDebugState::Paused||state==ScriptDebugState::Error){shownLine=(long)currentLine;go(shownLine);}
        if(state==ScriptDebugState::Error)tab=3;
        if(state==ScriptDebugState::Finished)shownLine=0;
        showPanel();refresh();
    }
    void command (ScriptIDEAction action) {
        if(action==ScriptIDEAction::Toggle){toggle(caretLine());return;}
        if(action==ScriptIDEAction::Clear){for(auto &a:anchors)if(a.range)a.range->Release();anchors.clear();syncAnchors();save();InvalidateRect(gutter,nullptr,FALSE);return;}
        if(action==ScriptIDEAction::List){syncAnchors();std::wstring s=L"Breakpoints (source lines):\r\n";for(auto n:breakpoints)s+=std::to_wstring(n)+L"\r\n";SetWindowTextW(view,s.c_str());return;}
        if(action==ScriptIDEAction::Stop){stop();if(!current){finish();lock(false);}return;}
        if(state==ScriptDebugState::Paused) {
            resume(action==ScriptIDEAction::Over?ScriptDebugStep::Over:action==ScriptIDEAction::Into?ScriptDebugStep::Into:action==ScriptIDEAction::Out?ScriptDebugStep::Out:ScriptDebugStep::Continue);
            return;
        }
        if(action!=ScriptIDEAction::Start||active()||launching)return;
        fileChanged();syncAnchors();start();launching=true;
        ScriptEditor_runForDebugger(editor);
        launching=false;
    }
    bool key (WPARAM key) {
        const bool shift=GetKeyState(VK_SHIFT)<0;
        if(key==VK_F9)command(ScriptIDEAction::Toggle);
        else if(key==VK_F5)command(shift?ScriptIDEAction::Stop:ScriptIDEAction::Start);
        else if(key==VK_F10)command(ScriptIDEAction::Over);
        else if(key==VK_F11)command(shift?ScriptIDEAction::Out:ScriptIDEAction::Into);
        else return false;
        return true;
    }
    void pump (bool wait) {
        MSG msg;
        if(wait)MsgWaitForMultipleObjects(0,nullptr,FALSE,30,QS_ALLINPUT);
        int count=0;
        while(count++<100&&PeekMessageW(&msg,nullptr,0,0,PM_REMOVE)) {
            if(msg.message==WM_QUIT){stop();PostQuitMessage((int)msg.wParam);break;}
            bool ours=msg.hwnd==shell||IsChild(shell,msg.hwnd);
            if(ours&&(msg.message==WM_KEYDOWN||msg.message==WM_SYSKEYDOWN)&&key(msg.wParam))continue;
            // No unrelated app commands/re-entrant scripts while suspended. Paint is safe.
            if(ours) {TranslateMessage(&msg);DispatchMessageW(&msg);}
            else if(msg.message==WM_PAINT||msg.message==WM_NCPAINT||msg.message==WM_ERASEBKGND)DispatchMessageW(&msg);
        }
    }
    void lock (bool locked) {
        SendMessageW(text,EM_SETREADONLY,locked,0);
        editor->textWidget->d_editable=!locked;
        HMENU menu=GetMenu(shell);
        for(int i=0;i<GetMenuItemCount(menu);++i) {
            wchar_t title[64] {};GetMenuStringW(menu,i,title,64,MF_BYPOSITION);
            if(std::wstring(title).find(L"Debug")==std::wstring::npos)
                EnableMenuItem(menu,i,MF_BYPOSITION|(locked?MF_GRAYED:MF_ENABLED));
        }
        DrawMenuBar(shell);
        if(locked&&disabledWindows.empty()) {
            EnumThreadWindows(GetCurrentThreadId(),[](HWND w,LPARAM p)->BOOL {
                auto self=reinterpret_cast<WinIDE*>(p);
                if(w!=self->shell&&IsWindowEnabled(w)&&IsWindowVisible(w)){self->disabledWindows.push_back(w);EnableWindow(w,FALSE);}return TRUE;
            },(LPARAM)this);
        }
        if(!locked){for(HWND w:disabledWindows)if(IsWindow(w))EnableWindow(w,TRUE);disabledWindows.clear();}
    }
    void poll () override {
        ULONGLONG now=GetTickCount64();
        if(now-lastPoll>=30){lastPoll=now;pump(false);}
    }
    void paused () override {
        lock(true);
        coloured.clear();refresh();
        RedrawWindow(text,nullptr,nullptr,RDW_INVALIDATE|RDW_ALLCHILDREN|RDW_UPDATENOW);
        while(state==ScriptDebugState::Paused)pump(true);
    }
    void formatRange (long first,long last,COLORREF foreground,COLORREF background) {
        if(last<=first)return;
        CHARRANGE range {first,last};SendMessageW(text,EM_EXSETSEL,0,(LPARAM)&range);
        CHARFORMAT2W format {};format.cbSize=sizeof(format);format.dwMask=CFM_COLOR|CFM_BACKCOLOR;
        format.crTextColor=foreground;format.crBackColor=background;
        SendMessageW(text,EM_SETCHARFORMAT,SCF_SELECTION,(LPARAM)&format);
    }
    void refresh () {
        if(formatting||!IsWindow(text))return;
        fileChanged();syncAnchors();
        CHARRANGE selection;SendMessageW(text,EM_EXGETSEL,0,(LPARAM)&selection);
        long line=(long)SendMessageW(text,EM_EXLINEFROMCHAR,0,selection.cpMin)+1;
        long column=selection.cpMin-lineStart(line)+1;
        const wchar_t *stateName=state==ScriptDebugState::Paused?L"Paused":state==ScriptDebugState::Running?L"Running":state==ScriptDebugState::Error?L"Error":state==ScriptDebugState::Finished?L"Finished":L"Idle";
        std::wstring label=L"Ln "+std::to_wstring(line)+L", Col "+std::to_wstring(column)+L"    |    "+stateName+L"    |    F9 breakpoint  F5 run/continue  F10 over  F11 into";
        SetWindowTextW(status,label.c_str());
        HMENU menu=GetMenu(shell);
        for(int i=0;i<GetMenuItemCount(menu);++i) {
            wchar_t title[64] {};GetMenuStringW(menu,i,title,64,MF_BYPOSITION);
            if(std::wstring(title).find(L"Debug")!=std::wstring::npos) {
                HMENU debug=GetSubMenu(menu,i);
                EnableMenuItem(debug,0,MF_BYPOSITION|((state==ScriptDebugState::Running||state==ScriptDebugState::Stopping)?MF_GRAYED:MF_ENABLED));
                EnableMenuItem(debug,1,MF_BYPOSITION|(active()?MF_ENABLED:MF_GRAYED));
                for(int index=2;index<=4;++index)EnableMenuItem(debug,index,MF_BYPOSITION|(state==ScriptDebugState::Paused?MF_ENABLED:MF_GRAYED));
            }
        }
        RECT r;GetClientRect(text,&r);
        // RichEdit may scroll child windows with its document; the gutter stays pinned.
        SetWindowPos(gutter,HWND_TOP,0,0,64,r.bottom,SWP_NOACTIVATE);
        long first=(long)SendMessageW(text,EM_GETFIRSTVISIBLELINE,0,0)+1;
        long count=(long)SendMessageW(text,EM_GETLINECOUNT,0,0);
        HDC dc=GetDC(text);HFONT old=(HFONT)SelectObject(dc,(HFONT)SendMessageW(text,WM_GETFONT,0,0));TEXTMETRICW tm;GetTextMetricsW(dc,&tm);SelectObject(dc,old);ReleaseDC(text,dc);
        long last=std::min(count,first+r.bottom/std::max(1L,tm.tmHeight)+2);
        bool redraw=first!=oldFirst||selection.cpMin!=oldCaret||count!=oldCount;
        oldFirst=first;oldCaret=selection.cpMin;oldCount=count;
        if(!document){InvalidateRect(gutter,nullptr,FALSE);return;}
        bool needsFormat=false;
        for(long n=first;n<=last;++n) {
            auto found=coloured.find(n);
            if(found==coloured.end()||found->second!=std::make_pair(lineText(n),n==shownLine)){needsFormat=true;break;}
        }
        if(!needsFormat){if(redraw)InvalidateRect(gutter,nullptr,FALSE);return;}
        formatting=true;
        POINT scroll;SendMessageW(text,EM_GETSCROLLPOS,0,(LPARAM)&scroll);
        BOOL modified=(BOOL)SendMessageW(text,EM_GETMODIFY,0,0);
        auto mask=SendMessageW(text,EM_SETEVENTMASK,0,0);
        document->Undo(tomSuspend,nullptr);SendMessageW(text,WM_SETREDRAW,FALSE,0);
        for(long n=first;n<=last;++n) {
            auto source=lineText(n);bool currentLineHighlight=(n==shownLine);
            auto found=coloured.find(n);
            if(found!=coloured.end()&&found->second==std::make_pair(source,currentLineHighlight))continue;
            long start=lineStart(n);
            auto bg=currentLineHighlight?theme.execution:theme.background;
            formatRange(start,start+(long)source.size(),theme.text,bg);
            for(auto &span:ScriptSyntax_scan(source))formatRange(start+(long)span.start,start+(long)(span.start+span.length),theme.colour(span.token),bg);
            coloured[n]={source,currentLineHighlight};redraw=true;
        }
        if(coloured.size()>256) {auto keep=std::move(coloured);coloured.clear();for(long n=first;n<=last;++n)coloured[n]=std::move(keep[n]);}
        SendMessageW(text,EM_EXSETSEL,0,(LPARAM)&selection);
        SendMessageW(text,EM_SETSCROLLPOS,0,(LPARAM)&scroll);
        SendMessageW(text,EM_SETMODIFY,modified,0);
        SendMessageW(text,EM_SETEVENTMASK,0,mask);
        document->Undo(tomResume,nullptr);SendMessageW(text,WM_SETREDRAW,TRUE,0);
        formatting=false;
        if(redraw)RedrawWindow(text,nullptr,nullptr,RDW_INVALIDATE|RDW_ALLCHILDREN);
    }
};
std::map<ScriptEditor,WinIDE*> editors;
LRESULT CALLBACK gutterProc(HWND window,UINT message,WPARAM wp,LPARAM lp,UINT_PTR,DWORD_PTR data) {
    auto self=reinterpret_cast<WinIDE*>(data);
    if(message==WM_LBUTTONDOWN) {
        POINTL point {70,(short)HIWORD(lp)};
        long position=(long)SendMessageW(self->text,EM_CHARFROMPOS,0,(LPARAM)&point);
        self->toggle((long)SendMessageW(self->text,EM_EXLINEFROMCHAR,0,position)+1);return 0;
    }
    if(message==WM_PAINT) {
        PAINTSTRUCT ps;HDC dc=BeginPaint(window,&ps);RECT r;GetClientRect(window,&r);
        HBRUSH bg=CreateSolidBrush(self->theme.gutter);FillRect(dc,&r,bg);DeleteObject(bg);
        SetBkMode(dc,TRANSPARENT);SetTextColor(dc,self->theme.number);
        auto old=SelectObject(dc,(HFONT)SendMessageW(self->text,WM_GETFONT,0,0));
        long first=(long)SendMessageW(self->text,EM_GETFIRSTVISIBLELINE,0,0)+1;
        long count=(long)SendMessageW(self->text,EM_GETLINECOUNT,0,0);
        for(long line=first;line<=count;++line) {
            POINTL point;SendMessageW(self->text,EM_POSFROMCHAR,(WPARAM)&point,self->lineStart(line));
            if(point.y>r.bottom)break;
            std::wstring label=std::to_wstring(line);
            TextOutW(dc,22,point.y,label.c_str(),(int)label.size());
            if(self->breakpoints.count(line)) {HBRUSH b=CreateSolidBrush(self->theme.breakpoint);auto prev=SelectObject(dc,b);Ellipse(dc,3,point.y+2,15,point.y+14);SelectObject(dc,prev);DeleteObject(b);}
            if(line==self->shownLine){POINT p[3]={{16,point.y+2},{22,point.y+8},{16,point.y+14}};MoveToEx(dc,p[0].x,p[0].y,nullptr);LineTo(dc,p[1].x,p[1].y);LineTo(dc,p[2].x,p[2].y);}
        }
        SelectObject(dc,old);EndPaint(window,&ps);return 0;
    }
    return DefSubclassProc(window,message,wp,lp);
}
LRESULT CALLBACK panelProc(HWND window,UINT message,WPARAM wp,LPARAM lp,UINT_PTR,DWORD_PTR data) {
    auto self=reinterpret_cast<WinIDE*>(data);
    if(message==WM_COMMAND) {
        HWND control=(HWND)lp;
        for(int i=0;i<4;++i)if(control==self->tabs[i]){self->tab=i;self->showPanel();return 0;}
        if(control==self->add) {
            auto value=nativeText(self->watchInput);
            if(!value.empty()&&self->watches.size()<100)self->watches.emplace_back(Melder_peekWto32(value.c_str()));
            self->tab=1;self->showPanel();return 0;
        }
        if(control==self->remove){if(!self->watches.empty())self->watches.pop_back();self->tab=1;self->showPanel();return 0;}
        return 0;
    }
    return DefSubclassProc(window,message,wp,lp);
}
LRESULT CALLBACK textProc(HWND window,UINT message,WPARAM wp,LPARAM lp,UINT_PTR,DWORD_PTR data) {
    auto self=reinterpret_cast<WinIDE*>(data);
    if(message==WM_TIMER&&wp==771){self->refresh();return 0;}
    if((message==WM_KEYDOWN||message==WM_SYSKEYDOWN)&&self->key(wp))return 0;
    if(message==WM_PASTE){return DefSubclassProc(window,EM_PASTESPECIAL,CF_UNICODETEXT,0);}
    auto result=DefSubclassProc(window,message,wp,lp);
    if(message==WM_SIZE){self->layout();self->coloured.clear();}
    return result;
}
LRESULT CALLBACK shellProc(HWND window,UINT message,WPARAM wp,LPARAM lp,UINT_PTR,DWORD_PTR data) {
    auto self=reinterpret_cast<WinIDE*>(data);
    if(message==WM_CLOSE&&self->active()){self->stop();return 0;}
    auto result=DefSubclassProc(window,message,wp,lp);
    if(message==WM_SIZE)self->layout();
    return result;
}
}

void WinScriptEditor_create (ScriptEditor editor) {
    auto self=new WinIDE(editor);editors[editor]=self;
    self->text=editor->textWidget->d_widget->window;
    self->host=GetParent(self->text);
    self->shell=GetAncestor(self->text,GA_ROOT);
    self->font=CreateFontW(14,0,0,0,FW_NORMAL,FALSE,FALSE,FALSE,DEFAULT_CHARSET,0,0,0,FIXED_PITCH,L"Consolas");
    SendMessageW(self->text,EM_SETMARGINS,EC_LEFTMARGIN|EC_RIGHTMARGIN,MAKELPARAM(68,4));
    IUnknown *unknown=nullptr;
    SendMessageW(self->text,EM_GETOLEINTERFACE,0,(LPARAM)&unknown);
    if(unknown) {
        static const IID iid={0x8cc497c0,0xa1df,0x11ce,{0x80,0x98,0x00,0xaa,0x00,0x47,0xbe,0x5d}};
        unknown->QueryInterface(iid,(void**)&self->document);unknown->Release();
    }
    self->gutter=CreateWindowW(L"STATIC",L"",WS_CHILD|WS_VISIBLE|SS_NOTIFY,0,0,64,100,self->text,nullptr,GetModuleHandleW(nullptr),nullptr);
    SetWindowSubclass(self->gutter,gutterProc,1,(DWORD_PTR)self);
    self->panel=CreateWindowW(L"STATIC",L"Debugger",WS_CHILD|WS_VISIBLE,0,0,100,220,self->host,nullptr,GetModuleHandleW(nullptr),nullptr);
    SetWindowSubclass(self->panel,panelProc,1,(DWORD_PTR)self);
    const wchar_t *titles[]={L"Variables",L"Watch",L"Call Stack",L"Output / Error"};
    for(int i=0;i<4;++i)self->tabs[i]=CreateWindowW(L"BUTTON",titles[i],WS_CHILD|WS_VISIBLE|WS_TABSTOP,0,0,1,1,self->panel,nullptr,GetModuleHandleW(nullptr),nullptr);
    self->view=CreateWindowW(L"EDIT",L"",WS_CHILD|WS_VISIBLE|WS_BORDER|WS_VSCROLL|WS_HSCROLL|ES_MULTILINE|ES_AUTOVSCROLL|ES_AUTOHSCROLL|ES_READONLY,0,0,1,1,self->panel,nullptr,GetModuleHandleW(nullptr),nullptr);
    self->watchInput=CreateWindowW(L"EDIT",L"",WS_CHILD|WS_VISIBLE|WS_BORDER|WS_TABSTOP|ES_AUTOHSCROLL,0,0,1,1,self->panel,nullptr,GetModuleHandleW(nullptr),nullptr);
    self->add=CreateWindowW(L"BUTTON",L"Add Watch",WS_CHILD|WS_VISIBLE|WS_TABSTOP,0,0,1,1,self->panel,nullptr,GetModuleHandleW(nullptr),nullptr);
    self->remove=CreateWindowW(L"BUTTON",L"Remove last",WS_CHILD|WS_VISIBLE|WS_TABSTOP,0,0,1,1,self->panel,nullptr,GetModuleHandleW(nullptr),nullptr);
    self->status=CreateWindowW(L"STATIC",L"Ready",WS_CHILD|WS_VISIBLE,0,0,1,1,self->panel,nullptr,GetModuleHandleW(nullptr),nullptr);
    for(HWND h:{self->view,self->watchInput,self->status,self->add,self->remove,self->tabs[0],self->tabs[1],self->tabs[2],self->tabs[3]})SendMessageW(h,WM_SETFONT,(WPARAM)self->font,TRUE);
    SetWindowSubclass(self->text,textProc,1,(DWORD_PTR)self);
    SetWindowSubclass(self->shell,shellProc,771,(DWORD_PTR)self);
    SetTimer(self->text,771,100,nullptr);self->layout();self->showPanel();
}
void WinScriptEditor_destroy (ScriptEditor editor) {
    auto it=editors.find(editor);if(it==editors.end())return;auto self=it->second;
    self->lock(false);KillTimer(self->text,771);
    RemoveWindowSubclass(self->shell,shellProc,771);RemoveWindowSubclass(self->text,textProc,1);
    DestroyWindow(self->gutter);DestroyWindow(self->panel);
    editors.erase(it);delete self;
}
void WinScriptEditor_action (ScriptEditor editor,ScriptIDEAction action) {auto it=editors.find(editor);if(it!=editors.end())it->second->command(action);}
bool WinScriptEditor_active (ScriptEditor editor) {auto it=editors.find(editor);return it!=editors.end()&&it->second->active();}
ScriptDebugger *WinScriptEditor_debugger (ScriptEditor editor) {auto it=editors.find(editor);return it!=editors.end()&&it->second->active()?it->second:nullptr;}
void WinScriptEditor_finished (ScriptEditor editor) {auto it=editors.find(editor);if(it!=editors.end()){it->second->finish();it->second->lock(false);}}
void WinScriptEditor_begin (ScriptEditor editor) {auto it=editors.find(editor);if(it!=editors.end()&&it->second->active())it->second->lock(true);}
void WinScriptEditor_reset (ScriptEditor editor) {auto it=editors.find(editor);if(it!=editors.end()){it->second->current=nullptr;it->second->shownLine=0;it->second->state=ScriptDebugState::Idle;it->second->showPanel();}}
#else
void WinScriptEditor_create(structScriptEditor*){}
void WinScriptEditor_destroy(structScriptEditor*){}
void WinScriptEditor_action(structScriptEditor*,ScriptIDEAction){}
bool WinScriptEditor_active(structScriptEditor*){return false;}
ScriptDebugger *WinScriptEditor_debugger(structScriptEditor*){return nullptr;}
void WinScriptEditor_finished(structScriptEditor*){}
void WinScriptEditor_begin(structScriptEditor*){}
void WinScriptEditor_reset(structScriptEditor*){}
#endif
