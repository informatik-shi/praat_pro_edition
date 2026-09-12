// Windows code view. Included in LuaEditor.cpp to keep platform details private.
#if defined (_WIN32)
#include <richole.h>
#include <tom.h>
#include <map>
#include <set>
#include <string>
#include "codefold/CodeFolding.h"
namespace {
bool luaAlpha(wchar_t c) {return (c>=L'a'&&c<=L'z')||(c>=L'A'&&c<=L'Z');}
bool luaDigit(wchar_t c) {return c>=L'0'&&c<=L'9';}
bool luaAlnum(wchar_t c) {return luaAlpha(c)||luaDigit(c);}
struct LuaColourSpan { long start, end; COLORREF colour; };
struct LuaView {
    LuaEditor editor;
    HWND text, gutter, host=nullptr,panel=nullptr,view=nullptr,status=nullptr,tabs[3]{},buttons[5]{};
    ITextDocument *document=nullptr;
    LuaDebugSession debug;
    struct Anchor {ITextRange *range;};
    std::vector<Anchor> anchors;
    int tab=0;
    std::wstring lastPanel,filePath;
    std::wstring previous;
    std::vector<LuaColourSpan> spans;
    long oldFirst=-1;
    unsigned long foldRevision=0;
    bool formatting=false;
    void syncBreakpoints() {
        CodeFolding_sync(editor->textWidget);
        std::wstring path=Melder_peek32toW(MelderFile_peekPath(&editor->file));
        if(!filePath.empty()&&path!=filePath){for(auto &a:anchors)a.range->Release();anchors.clear();}
        filePath=std::move(path);debug.breakpoints.clear();
        for(auto &a:anchors){long position=0;a.range->GetStart(&position);debug.breakpoints.insert(CodeFolding_lineAt(editor->textWidget,position)+1);}
    }
    void toggle(long line) {
        syncBreakpoints();
        for(auto it=anchors.begin();it!=anchors.end();++it) {
            long position=0;it->range->GetStart(&position);
            if(CodeFolding_lineAt(editor->textWidget,position)+1==line){it->range->Release();anchors.erase(it);syncBreakpoints();InvalidateRect(gutter,nullptr,FALSE);return;}
        }
        long start=CodeFolding_lineStart(editor->textWidget,line-1);if(start<0||!document)return;
        if(!CodeFolding_lineText(editor->textWidget,line-1).empty())++start;
        ITextRange *range=nullptr;if(SUCCEEDED(document->Range(start,start,&range)))anchors.push_back({range});
        syncBreakpoints();InvalidateRect(gutter,nullptr,FALSE);
    }
    void layout() {
        if(!panel)return;RECT r;GetClientRect(host,&r);const int height=210;
        SetWindowPos(panel,nullptr,0,std::max(0L,r.bottom-height),r.right,height,SWP_NOZORDER|SWP_NOACTIVATE);
        for(int i=0;i<3;++i)MoveWindow(tabs[i],4+i*130,3,128,24,TRUE);
        MoveWindow(view,4,31,std::max(1L,r.right-8),height-91,TRUE);
        for(int i=0;i<5;++i)MoveWindow(buttons[i],4+i*94,height-55,92,24,TRUE);
        MoveWindow(status,4,height-27,std::max(1L,r.right-8),23,TRUE);
    }
    void showDebug() {
        if(!panel)return;
        const wchar_t *state=debug.state==LuaDebugState::Paused?L"Paused":debug.state==LuaDebugState::Running?L"Running":debug.state==LuaDebugState::Finished?L"Finished":debug.state==LuaDebugState::Stopped?L"Stopped":debug.state==LuaDebugState::Error?L"Error":L"Ready";
        std::wstring label=std::wstring(state)+L" | Ln "+std::to_wstring(debug.line)+L" | F9 breakpoint   F5 continue   F10 over   F11 into";
        SetWindowTextW(status,label.c_str());
        std::wstring content;
        if(tab<2){auto s=Melder_8to32_e((tab==0?debug.variables:debug.callStack).c_str());content=Melder_peek32toW(s.get());if(content.empty())content=L"Pause the Lua script to inspect its real state.";}
        else {auto error=Melder_8to32_e(debug.error.c_str());content=Melder_peek32toW(error.get());content+=L"\nPraat Info:\n";const char32 *info=Melder_getInfo();if(info)content+=Melder_peek32toW(info+std::max<integer>(0,Melder_length(info)-32000));}
        if(content!=lastPanel){lastPanel=content;std::wstring crlf;for(wchar_t c:content){if(c==L'\n')crlf+=L'\r';crlf+=c;}SetWindowTextW(view,crlf.c_str());}
        for(int i=0;i<3;++i)SendMessageW(tabs[i],BM_SETSTATE,tab==i,0);
        for(int i=1;i<=3;++i)EnableWindow(buttons[i],!editor->running||debug.state==LuaDebugState::Paused);
        EnableWindow(buttons[4],editor->running);
        InvalidateRect(gutter,nullptr,FALSE);
    }
    void scan (const std::wstring &s) {
        spans.clear();
        static const std::set<std::wstring> keywords={L"and",L"break",L"do",L"else",L"elseif",L"end",L"false",L"for",L"function",L"global",L"goto",L"if",L"in",L"local",L"nil",L"not",L"or",L"repeat",L"return",L"then",L"true",L"until",L"while"};
        for(size_t i=0;i<s.size();) {
            size_t start=i;
            bool comment=s.compare(i,2,L"--")==0;
            if(comment) i+=2;
            size_t open=i;
            if(i<s.size() && s[i]==L'[') {
                size_t j=i+1;while(j<s.size()&&s[j]==L'=')++j;
                if(j<s.size()&&s[j]==L'[') {
                    std::wstring closing=L"]"+std::wstring(j-i-1,L'=')+L"]";
                    size_t end=s.find(closing,j+1);
                    i=end==std::wstring::npos?s.size():end+closing.size();
                    spans.push_back({(long)start,(long)i,comment?RGB(55,120,65):RGB(155,55,37)});continue;
                }
            }
            if(comment) {
                while(i<s.size()&&s[i]!=L'\r'&&s[i]!=L'\n')++i;
                spans.push_back({(long)start,(long)i,RGB(55,120,65)});continue;
            }
            i=open;
            if(s[i]==L'\'' || s[i]==L'"') {
                wchar_t quote=s[i++];
                while(i<s.size()) {if(s[i]==L'\\'){i=std::min(i+2,s.size());continue;} if(s[i++]==quote)break;}
                spans.push_back({(long)start,(long)i,RGB(155,55,37)});
            } else if(luaAlpha(s[i]) || s[i]==L'_') {
                while(i<s.size()&&(luaAlnum(s[i])||s[i]==L'_'))++i;
                auto word=s.substr(start,i-start);
                if(keywords.count(word))spans.push_back({(long)start,(long)i,RGB(28,69,179)});
                else if(word==L"praat" || word==L"print")spans.push_back({(long)start,(long)i,RGB(0,110,130)});
            } else if(luaDigit(s[i])) {
                while(i<s.size()&&(luaAlnum(s[i])||s[i]==L'.'))++i;
                spans.push_back({(long)start,(long)i,RGB(125,55,150)});
            } else ++i;
        }
    }
    void colour(long first,long last,COLORREF value) {
        if(last<=first)return;
        CHARRANGE range{first,last};SendMessageW(text,EM_EXSETSEL,0,(LPARAM)&range);
        CHARFORMAT2W f{};f.cbSize=sizeof(f);f.dwMask=CFM_COLOR;f.crTextColor=value;
        SendMessageW(text,EM_SETCHARFORMAT,SCF_SELECTION,(LPARAM)&f);
    }
    void refresh(bool force=false) {
        if(formatting||(editor->running&&debug.state!=LuaDebugState::Paused&&!force))return;
        syncBreakpoints();
        CodeFolding_sync(editor->textWidget);
        RECT r;GetClientRect(text,&r);SetWindowPos(gutter,HWND_TOP,0,0,72,r.bottom,SWP_NOACTIVATE);
        InvalidateRect(gutter,nullptr,FALSE);
        if(!document)return;
        GETTEXTLENGTHEX length{GTL_NUMCHARS|GTL_PRECISE,1200};
        long n=(long)SendMessageW(text,EM_GETTEXTLENGTHEX,(WPARAM)&length,0);
        std::wstring source(n+1,L'\0');GETTEXTEX get{(DWORD)((n+1)*sizeof(wchar_t)),GT_RAWTEXT,1200,nullptr,nullptr};
        SendMessageW(text,EM_GETTEXTEX,(WPARAM)&get,(LPARAM)source.data());source.resize(n);
        long first=(long)SendMessageW(text,EM_GETFIRSTVISIBLELINE,0,0);
        bool edited=source!=previous;
        auto revision=CodeFolding_revision(editor->textWidget);
        if(!edited&&first==oldFirst&&revision==foldRevision&&!force)return;
        foldRevision=revision;
        if(edited){scan(source);previous=std::move(source);}
        oldFirst=first;
        long start=(long)SendMessageW(text,EM_LINEINDEX,first,0);
        long last=(long)SendMessageW(text,EM_LINEINDEX,first+r.bottom/8+2,0);if(last<0)last=n;
        formatting=true;
        CHARRANGE selection;POINT scroll;
        SendMessageW(text,EM_EXGETSEL,0,(LPARAM)&selection);SendMessageW(text,EM_GETSCROLLPOS,0,(LPARAM)&scroll);
        auto modified=SendMessageW(text,EM_GETMODIFY,0,0),mask=SendMessageW(text,EM_SETEVENTMASK,0,0);
        document->Undo(tomSuspend,nullptr);SendMessageW(text,WM_SETREDRAW,FALSE,0);
        auto background=[&](long a,long b,COLORREF colour){if(b<=a)return;CHARRANGE range{a,b};SendMessageW(text,EM_EXSETSEL,0,(LPARAM)&range);CHARFORMAT2W f{};f.cbSize=sizeof(f);f.dwMask=CFM_BACKCOLOR;f.crBackColor=colour;SendMessageW(text,EM_SETCHARFORMAT,SCF_SELECTION,(LPARAM)&f);};
        background(start,last,RGB(255,255,255));
        if((debug.state==LuaDebugState::Paused||debug.state==LuaDebugState::Error)&&debug.line>0) {
            long a=CodeFolding_lineStart(editor->textWidget,debug.line-1);if(a>=0)background(a,a+(long)CodeFolding_lineText(editor->textWidget,debug.line-1).size(),RGB(255,239,167));
        }
        colour(start,last,RGB(32,35,40));
        for(auto &span:spans)if(span.end>start&&span.start<last)colour(std::max(start,span.start),std::min(last,span.end),span.colour);
        SendMessageW(text,EM_EXSETSEL,0,(LPARAM)&selection);SendMessageW(text,EM_SETSCROLLPOS,0,(LPARAM)&scroll);
        SendMessageW(text,EM_SETMODIFY,modified,0);SendMessageW(text,EM_SETEVENTMASK,0,mask);
        document->Undo(tomResume,nullptr);SendMessageW(text,WM_SETREDRAW,TRUE,0);
        formatting=false;InvalidateRect(text,nullptr,FALSE);
    }
};
std::map<LuaEditor,LuaView*> luaViews;
LRESULT CALLBACK luaGutter(HWND window,UINT message,WPARAM wp,LPARAM lp,UINT_PTR,DWORD_PTR data) {
    auto self=(LuaView*)data;
    if(message==WM_LBUTTONDOWN) {
        POINTL point{100,(short)HIWORD(lp)};long position=(long)SendMessageW(self->text,EM_CHARFROMPOS,0,(LPARAM)&point);
        self->toggle(CodeFolding_lineAt(self->editor->textWidget,position)+1);return 0;
    }
    if(message==WM_PAINT) {
        PAINTSTRUCT paint;HDC dc=BeginPaint(window,&paint);RECT r;GetClientRect(window,&r);
        FillRect(dc,&r,(HBRUSH)(COLOR_BTNFACE+1));SetBkMode(dc,TRANSPARENT);SetTextColor(dc,RGB(105,110,120));
        auto old=SelectObject(dc,(HFONT)SendMessageW(self->text,WM_GETFONT,0,0));
        long first=CodeFolding_firstVisible(self->editor->textWidget);
        long count=CodeFolding_lineCount(self->editor->textWidget);
        for(long line=first;line<count;++line) {
            if(CodeFolding_hiddenLine(self->editor->textWidget,line))continue;
            long index=CodeFolding_lineStart(self->editor->textWidget,line);POINTL pos;
            SendMessageW(self->text,EM_POSFROMCHAR,(WPARAM)&pos,index);if(pos.y>r.bottom)break;
            std::wstring label=std::to_wstring(line+1);RECT row{24,pos.y,64,pos.y+24};DrawTextW(dc,label.c_str(),-1,&row,DT_RIGHT|DT_SINGLELINE|DT_NOPREFIX);
            if(self->debug.breakpoints.count(line+1)){HBRUSH brush=CreateSolidBrush(RGB(190,45,50));auto prev=SelectObject(dc,brush);Ellipse(dc,3,pos.y+2,15,pos.y+14);SelectObject(dc,prev);DeleteObject(brush);}
            if(self->debug.line==line+1&&(self->debug.state==LuaDebugState::Paused||self->debug.state==LuaDebugState::Error)){MoveToEx(dc,16,pos.y+2,nullptr);LineTo(dc,22,pos.y+8);LineTo(dc,16,pos.y+14);}
        }
        SelectObject(dc,old);EndPaint(window,&paint);return 0;
    }
    return DefSubclassProc(window,message,wp,lp);
}
LRESULT CALLBACK luaTextProc(HWND window,UINT message,WPARAM wp,LPARAM lp,UINT_PTR,DWORD_PTR data) {
    auto self=(LuaView*)data;
    if(message==WM_SIZE){self->oldFirst=-1;self->layout();}
    if(message==WM_TIMER&&wp==775){self->refresh();return 0;}
    if((message==WM_KEYDOWN||message==WM_SYSKEYDOWN)&&luaDebugKey(self->editor,(unsigned long)wp))return 0;
    if(message==WM_KEYDOWN&&wp==VK_F7) {
        try {execute(self->editor,false,true);} catch(MelderError){Melder_flushError();}
        return 0;
    }
    return DefSubclassProc(window,message,wp,lp);
}
LRESULT CALLBACK luaPanelProc(HWND window,UINT message,WPARAM wp,LPARAM lp,UINT_PTR,DWORD_PTR data) {
    auto self=(LuaView*)data;
    if(message==WM_COMMAND&&HIWORD(wp)==BN_CLICKED) {
        for(int i=0;i<3;++i)if((HWND)lp==self->tabs[i]){self->tab=i;self->showDebug();return 0;}
        const LuaDebugAction actions[]={LuaDebugAction::Start,LuaDebugAction::Over,LuaDebugAction::Into,LuaDebugAction::Out,LuaDebugAction::Stop};
        for(int i=0;i<5;++i)if((HWND)lp==self->buttons[i]){try{luaDebugAction(self->editor,actions[i]);}catch(MelderError){Melder_flushError();}return 0;}
    }
    return DefSubclassProc(window,message,wp,lp);
}
}
static void createLuaView(LuaEditor editor) {
    auto self=new LuaView;self->editor=editor;self->text=editor->textWidget->d_widget->window;
    self->host=GetParent(self->text);
    luaViews[editor]=self;
    IUnknown *unknown=nullptr;SendMessageW(self->text,EM_GETOLEINTERFACE,0,(LPARAM)&unknown);
    if(unknown){static const IID iid={0x8cc497c0,0xa1df,0x11ce,{0x80,0x98,0x00,0xaa,0x00,0x47,0xbe,0x5d}};unknown->QueryInterface(iid,(void**)&self->document);unknown->Release();}
    SendMessageW(self->text,EM_SETMARGINS,EC_LEFTMARGIN|EC_RIGHTMARGIN,MAKELPARAM(58,4));
    self->gutter=CreateWindowW(L"STATIC",L"",WS_CHILD|WS_VISIBLE|SS_NOTIFY,0,0,72,100,self->text,nullptr,GetModuleHandleW(nullptr),nullptr);
    self->panel=CreateWindowW(L"STATIC",L"Lua Debugger",WS_CHILD|WS_VISIBLE,0,0,100,210,self->host,nullptr,GetModuleHandleW(nullptr),nullptr);
    SetWindowSubclass(self->panel,luaPanelProc,775,(DWORD_PTR)self);
    const wchar_t *tabTitles[]={L"Variables",L"Call Stack",L"Output / Error"};
    for(int i=0;i<3;++i)self->tabs[i]=CreateWindowW(L"BUTTON",tabTitles[i],WS_CHILD|WS_VISIBLE|WS_TABSTOP,0,0,1,1,self->panel,nullptr,GetModuleHandleW(nullptr),nullptr);
    self->view=CreateWindowW(L"EDIT",L"",WS_CHILD|WS_VISIBLE|WS_BORDER|WS_VSCROLL|WS_HSCROLL|ES_MULTILINE|ES_AUTOVSCROLL|ES_AUTOHSCROLL|ES_READONLY,0,0,1,1,self->panel,nullptr,GetModuleHandleW(nullptr),nullptr);
    const wchar_t *buttonTitles[]={L"Continue",L"Over",L"Into",L"Out",L"Stop"};
    for(int i=0;i<5;++i)self->buttons[i]=CreateWindowW(L"BUTTON",buttonTitles[i],WS_CHILD|WS_VISIBLE|WS_TABSTOP,0,0,1,1,self->panel,nullptr,GetModuleHandleW(nullptr),nullptr);
    self->status=CreateWindowW(L"STATIC",L"Ready",WS_CHILD|WS_VISIBLE,0,0,1,1,self->panel,nullptr,GetModuleHandleW(nullptr),nullptr);
    auto font=(WPARAM)GetStockObject(DEFAULT_GUI_FONT);
    for(HWND control:{self->view,self->status,self->tabs[0],self->tabs[1],self->tabs[2],self->buttons[0],self->buttons[1],self->buttons[2],self->buttons[3],self->buttons[4]})SendMessageW(control,WM_SETFONT,font,TRUE);
    SetWindowSubclass(self->gutter,luaGutter,775,(DWORD_PTR)self);SetWindowSubclass(self->text,luaTextProc,775,(DWORD_PTR)self);
    SetTimer(self->text,775,180,nullptr);
    CodeFolding_install(editor->textWidget,72,true);self->layout();self->showDebug();
}
static void destroyLuaView(LuaEditor editor) {
    CodeFolding_destroy(editor->textWidget);
    auto it=luaViews.find(editor);if(it==luaViews.end())return;auto self=it->second;
    KillTimer(self->text,775);RemoveWindowSubclass(self->text,luaTextProc,775);DestroyWindow(self->gutter);
    DestroyWindow(self->panel);for(auto &a:self->anchors)a.range->Release();
    if(self->document)self->document->Release();delete self;luaViews.erase(it);
}
static LuaDebugSession *luaDebugger(LuaEditor editor){auto it=luaViews.find(editor);return it==luaViews.end()?nullptr:&it->second->debug;}
static void luaDebugUpdate(LuaEditor editor) {
    auto it=luaViews.find(editor);if(it==luaViews.end())return;auto self=it->second;
    if((self->debug.state==LuaDebugState::Paused||self->debug.state==LuaDebugState::Error)&&self->debug.line>0) {
        CodeFolding_sync(editor->textWidget);long start=CodeFolding_lineStart(editor->textWidget,self->debug.line-1);
        if(start>=0){CHARRANGE range{start,start};SendMessageW(self->text,EM_EXSETSEL,0,(LPARAM)&range);SendMessageW(self->text,EM_SCROLLCARET,0,0);}
    }
    if(self->debug.state==LuaDebugState::Error)self->tab=2;
    self->showDebug();self->refresh(true);
}
static void luaDebugAction(LuaEditor editor,LuaDebugAction action) {
    auto it=luaViews.find(editor);if(it==luaViews.end())return;auto self=it->second;auto &debug=self->debug;
    if(action==LuaDebugAction::Toggle){CHARRANGE selection;SendMessageW(self->text,EM_EXGETSEL,0,(LPARAM)&selection);CodeFolding_sync(editor->textWidget);self->toggle(CodeFolding_lineAt(editor->textWidget,selection.cpMin)+1);return;}
    if(action==LuaDebugAction::Clear){for(auto &a:self->anchors)a.range->Release();self->anchors.clear();self->syncBreakpoints();InvalidateRect(self->gutter,nullptr,FALSE);return;}
    if(action==LuaDebugAction::Stop){editor->stopRequested=true;if(editor->running)debug.stop();return;}
    if(action==LuaDebugAction::Pause){if(editor->running)debug.pauseNext=true;return;}
    LuaDebugStep step=action==LuaDebugAction::Over?LuaDebugStep::Over:action==LuaDebugAction::Into?LuaDebugStep::Into:action==LuaDebugAction::Out?LuaDebugStep::Out:LuaDebugStep::Continue;
    if(editor->running){debug.resume(step);return;}
    self->syncBreakpoints();debug.breakOnEntry=action!=LuaDebugAction::Start;
    debug.state=LuaDebugState::Running;debug.line=0;debug.variables.clear();debug.callStack.clear();debug.error.clear();self->showDebug();self->refresh(true);
    execute(editor,false,false,true);
}
static bool luaDebugKey(LuaEditor editor,unsigned long key) {
    LuaDebugAction action;
    if(key==VK_F5)action=GetKeyState(VK_SHIFT)<0?LuaDebugAction::Stop:LuaDebugAction::Start;
    else if(key==VK_F6)action=LuaDebugAction::Pause;
    else if(key==VK_F9)action=LuaDebugAction::Toggle;
    else if(key==VK_F10)action=LuaDebugAction::Over;
    else if(key==VK_F11)action=GetKeyState(VK_SHIFT)<0?LuaDebugAction::Out:LuaDebugAction::Into;
    else return false;
    try{luaDebugAction(editor,action);}catch(MelderError){Melder_flushError();}return true;
}
static bool luaDebugDispatch(LuaEditor editor,unsigned int command) {
    auto it=luaViews.find(editor);if(it==luaViews.end())return false;HWND shell=GetAncestor(it->second->text,GA_ROOT);HMENU menu=GetMenu(shell);
    for(int i=0;i<GetMenuItemCount(menu);++i){wchar_t title[80]{};GetMenuStringW(menu,i,title,80,MF_BYPOSITION);if(std::wstring(title).find(L"Debug")==std::wstring::npos)continue;HMENU debug=GetSubMenu(menu,i);
        const LuaDebugAction actions[]={LuaDebugAction::Start,LuaDebugAction::Pause,LuaDebugAction::Stop,LuaDebugAction::Over,LuaDebugAction::Into,LuaDebugAction::Out,LuaDebugAction::Toggle,LuaDebugAction::Clear};
        for(int j=0;j<8;++j)if(GetMenuItemID(debug,j)==command){luaDebugAction(editor,actions[j]);return true;}
    }return false;
}
#else
static void createLuaView(LuaEditor) {}
static void destroyLuaView(LuaEditor) {}
static LuaDebugSession *luaDebugger(LuaEditor){return nullptr;}
static void luaDebugUpdate(LuaEditor){}
static bool luaDebugKey(LuaEditor,unsigned long){return false;}
static bool luaDebugDispatch(LuaEditor,unsigned int){return false;}
static void luaDebugAction(LuaEditor editor,LuaDebugAction action){if(action==LuaDebugAction::Stop)editor->stopRequested=true;else if(action==LuaDebugAction::Start)execute(editor,false,false);}
#endif
