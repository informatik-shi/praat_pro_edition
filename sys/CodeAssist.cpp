// Shared Windows editor assistance. No script is executed by this module.
// GPL-3.0-or-later.
#include "codefold/AssistModel.h"
#include "CodeAssist.h"
#include "Theme.h"
#if defined (_WIN32)
#include "GuiP.h"
#include "praatP.h"
#include "codefold/CodeFolding.h"
#include "../external/lua-5.5.1/src/lua.h"
#include "../external/lua-5.5.1/src/lauxlib.h"
#include <richedit.h>
#include <future>
#include <map>
#include <memory>

namespace {
constexpr UINT_PTR assistId=784;
std::wstring sourceText(HWND text) {
    GETTEXTLENGTHEX length{GTL_NUMCHARS|GTL_PRECISE,1200};
    long n=(long)SendMessageW(text,EM_GETTEXTLENGTHEX,(WPARAM)&length,0);
    std::wstring s(n+1,L'\0');GETTEXTEX get{(DWORD)((n+1)*sizeof(wchar_t)),GT_RAWTEXT,1200,nullptr,nullptr};
    auto count=SendMessageW(text,EM_GETTEXTEX,(WPARAM)&get,(LPARAM)s.data());s.resize((size_t)count);return s;
}
std::vector<CodeAssist::Problem> analyse(std::wstring source,bool lua) {
    if(!lua)return CodeAssist::checkPraat(source);
    // Independent Lua state: no libraries, Praat globals, require, or execution.
    auto state=std::unique_ptr<lua_State,decltype(&lua_close)>(luaL_newstate(),lua_close);
    if(!state)return {{1,L"Cannot allocate Lua syntax checker"}};
    int count=WideCharToMultiByte(CP_UTF8,0,source.data(),(int)source.size(),nullptr,0,nullptr,nullptr);
    std::string utf8(count,'\0');WideCharToMultiByte(CP_UTF8,0,source.data(),(int)source.size(),utf8.data(),count,nullptr,nullptr);
    if(luaL_loadbufferx(state.get(),utf8.data(),utf8.size(),"editor","t")==LUA_OK)return {};
    const char *raw=lua_tostring(state.get(),-1);std::string error=raw?raw:"Lua syntax check failed";
    size_t line=1;
    for(size_t i=0;i+2<error.size();++i)if(error[i]==':'&&isdigit((unsigned char)error[i+1])) {
        char *end=nullptr;auto value=strtoul(error.c_str()+i+1,&end,10);
        if(end&&*end==':'){line=value;break;}
    }
    int size=MultiByteToWideChar(CP_UTF8,0,error.data(),(int)error.size(),nullptr,0);
    std::wstring message(size,L'\0');MultiByteToWideChar(CP_UTF8,0,error.data(),(int)error.size(),message.data(),size);
    return {{line,message}};
}
std::vector<CodeAssist::Entry> catalogue() {
    std::vector<CodeAssist::Entry> result;
    std::map<std::wstring,std::set<std::wstring>> selections;
    auto add=[&](Praat_Command c){
        if(!c||!c->title||!c->callback||c->script||c->noApi)return;
        std::wstring title=Melder_peek32toW(c->title.get());
        if(title.size()>=3&&title.substr(title.size()-3)==L"...")title.resize(title.size()-3);
        if(title.empty()||title[0]==L'-')return;
        std::wstring help;
        if(c->class1)help=Melder_peek32toW(c->class1->className);
        if(c->class2){help+=L" + ";help+=Melder_peek32toW(c->class2->className);}
        if(c->class3){help+=L" + ";help+=Melder_peek32toW(c->class3->className);}
        if(c->class4){help+=L" + ";help+=Melder_peek32toW(c->class4->className);}
        selections[title].insert(help.empty()?L"no selection required":help);
    };
    for(integer i=1;i<=praat_getNumberOfMenuCommands();++i)add(praat_getMenuCommand(i));
    for(integer i=1;i<=praat_getNumberOfActions();++i)add(praat_getAction(i));
    for(auto &entry:selections) {
        std::wstring help=L"Available for: ";bool first=true;
        for(auto &variant:entry.second){if(!first)help+=L" OR ";help+=variant;first=false;}
        if(entry.first==L"To Spectrogram")help+=L"\nSound parameters: windowLength, maximumFrequency, timeStep, frequencyStep, windowShape";
        else if(entry.first==L"Create Sound from formula")help+=L"\nParameters: name, channels, startTime, endTime, samplingFrequency, formula";
        else if(entry.first==L"Read from file")help+=L"\nParameter: fileName";
        result.push_back({entry.first,help});
    }
    return result;
}
struct Assist;
LRESULT CALLBACK assistProc(HWND,UINT,WPARAM,LPARAM,UINT_PTR,DWORD_PTR);
LRESULT CALLBACK windowProc(HWND,UINT,WPARAM,LPARAM,UINT_PTR,DWORD_PTR);
struct Assist {
    GuiText widget;HWND text, popup=nullptr, list=nullptr, hint=nullptr, problemsWindow=nullptr, problemsList=nullptr, summary=nullptr;
    bool lua, busy=false, swallow=false, helpOnly=false;
    std::wstring observed, submitted, checked;
    ULONGLONG changedAt=0;
    std::future<std::vector<CodeAssist::Problem>> job;
    std::vector<CodeAssist::Problem> problems;
    std::vector<CodeAssist::Entry> entries;
    CodeAssist::Context context;
    Assist(GuiText w,bool l):widget(w),text(w->d_widget->window),lua(l){}
    ~Assist(){if(popup)DestroyWindow(popup);if(problemsWindow)DestroyWindow(problemsWindow);}
    HWND tool(const wchar_t *title,int width,int height) {
        RECT owner;GetWindowRect(GetAncestor(text,GA_ROOT),&owner);
        MONITORINFO monitor{sizeof(monitor)};GetMonitorInfoW(MonitorFromWindow(text,MONITOR_DEFAULTTONEAREST),&monitor);
        int x=std::max(monitor.rcWork.left,std::min(owner.left+24,monitor.rcWork.right-width));
        int y=std::max(monitor.rcWork.top,std::min(owner.top+80,monitor.rcWork.bottom-height));
        HWND w=CreateWindowExW(WS_EX_TOOLWINDOW,L"STATIC",title,WS_POPUP|WS_CAPTION|WS_SYSMENU|WS_THICKFRAME,
            x,y,width,height,GetAncestor(text,GA_ROOT),nullptr,GetModuleHandleW(nullptr),nullptr);
        SetWindowSubclass(w,windowProc,assistId,(DWORD_PTR)this);return w;
    }
    void closeCompletion(){if(popup)ShowWindow(popup,SW_HIDE);}
    bool visible(){return popup&&IsWindowVisible(popup);}
    void layout() {
        RECT r;
        if(popup){GetClientRect(popup,&r);MoveWindow(list,0,0,r.right,std::max(1L,r.bottom-48),TRUE);MoveWindow(hint,4,std::max(0L,r.bottom-46),std::max(1L,r.right-8),44,TRUE);}
        if(problemsWindow){GetClientRect(problemsWindow,&r);MoveWindow(summary,4,4,std::max(1L,r.right-8),42,TRUE);MoveWindow(problemsList,4,48,std::max(1L,r.right-8),std::max(1L,r.bottom-52),TRUE);}
    }
    void selectedHint() {
        auto index=SendMessageW(list,LB_GETCURSEL,0,0);
        if(index>=0&&(size_t)index<entries.size())SetWindowTextW(hint,entries[index].help.c_str());
    }
    void complete(bool parameters=false) {
        if(SendMessageW(text,EM_GETOPTIONS,0,0)&ECO_READONLY)return;
        CHARRANGE range;SendMessageW(text,EM_EXGETSEL,0,(LPARAM)&range);if(range.cpMin!=range.cpMax)return;
        auto source=sourceText(text);context=CodeAssist::context(source,range.cpMin,lua);helpOnly=parameters;
        if(parameters) {
            size_t caret=range.cpMin;auto code=CodeAssist::mask(source,lua);
            if(lua) {
                int depth=0;size_t open=caret;
                while(open>0){--open;if(code[open]==L')')++depth;else if(code[open]==L'('){if(depth==0)break;--depth;}}
                if(open>=code.size()||code[open]!=L'('){closeCompletion();return;}
                size_t end=open;while(end>0&&(code[end-1]==L' '||code[end-1]==L'\t'))--end;
                context=CodeAssist::context(source,end,true);
                if(context.prefix==L"praat.call") {
                    size_t p=open+1;while(p<source.size()&&(source[p]==L' '||source[p]==L'\t'||source[p]==L'\r'||source[p]==L'\n'))++p;
                    if(p<source.size()&&(source[p]==L'"'||source[p]==L'\'')) {
                        auto endQuote=source.find(source[p],p+1);
                        if(endQuote!=source.npos&&endQuote<caret){context.commands=true;context.prefix=source.substr(p+1,endQuote-p-1);}
                    }
                }
            } else {
                auto begin=source.find_last_of(L"\r\n",caret?caret-1:0);begin=begin==source.npos?0:begin+1;
                auto colon=source.find(L':',begin);if(colon==source.npos||colon>=caret){closeCompletion();return;}
                context.commands=true;context.prefix=CodeAssist::trim(source.substr(begin,colon-begin));
            }
        }
        entries=CodeAssist::complete(source,context,lua,catalogue());
        if(parameters)entries.erase(std::remove_if(entries.begin(),entries.end(),[&](const CodeAssist::Entry&e){return e.name!=context.prefix;}),entries.end());
        else entries.erase(std::remove_if(entries.begin(),entries.end(),[&](const CodeAssist::Entry&e){return e.name==context.prefix;}),entries.end());
        if(entries.empty()){closeCompletion();return;}
        if(!popup) {
            popup=tool(L"Completion — Enter / Tab to insert, Esc to close",540,245);
            // Keep keyboard focus in the source editor.
            SetWindowLongPtrW(popup,GWL_EXSTYLE,GetWindowLongPtrW(popup,GWL_EXSTYLE)|WS_EX_NOACTIVATE);
            list=CreateWindowW(L"LISTBOX",L"",WS_CHILD|WS_VISIBLE|WS_VSCROLL|LBS_NOTIFY|(Theme_enabled()?LBS_OWNERDRAWFIXED|LBS_HASSTRINGS:0),0,0,1,1,popup,nullptr,GetModuleHandleW(nullptr),nullptr);
            hint=CreateWindowW(L"STATIC",L"",WS_CHILD|WS_VISIBLE,0,0,1,1,popup,nullptr,GetModuleHandleW(nullptr),nullptr);
            for(HWND h:{list,hint})SendMessageW(h,WM_SETFONT,(WPARAM)GetStockObject(DEFAULT_GUI_FONT),TRUE);
        }
        SendMessageW(list,LB_RESETCONTENT,0,0);for(auto &e:entries)SendMessageW(list,LB_ADDSTRING,0,(LPARAM)e.name.c_str());
        SetWindowTextW(popup,parameters?L"Parameter hints — Esc to close":L"Completion — Enter / Tab to insert, Esc to close");
        SendMessageW(list,LB_SETCURSEL,0,0);selectedHint();layout();
        POINTL caret{};SendMessageW(text,EM_POSFROMCHAR,(WPARAM)&caret,range.cpMin);POINT point{caret.x,caret.y+24};ClientToScreen(text,&point);
        MONITORINFO mi{sizeof(mi)};GetMonitorInfoW(MonitorFromWindow(text,MONITOR_DEFAULTTONEAREST),&mi);
        point.x=std::max(mi.rcWork.left,std::min(point.x,mi.rcWork.right-540));point.y=std::max(mi.rcWork.top,std::min(point.y,mi.rcWork.bottom-245));
        SetWindowPos(popup,HWND_TOP,point.x,point.y,540,245,SWP_NOACTIVATE|SWP_SHOWWINDOW);
    }
    void accept() {
        if(helpOnly){closeCompletion();return;}
        auto index=SendMessageW(list,LB_GETCURSEL,0,0);if(index<0||(size_t)index>=entries.size())return;
        if(SendMessageW(text,EM_GETOPTIONS,0,0)&ECO_READONLY){closeCompletion();return;}
        auto source=sourceText(text);CHARRANGE caret;SendMessageW(text,EM_EXGETSEL,0,(LPARAM)&caret);
        if(caret.cpMin!=(long)context.end||caret.cpMax!=caret.cpMin||source.substr(context.start,context.end-context.start)!=context.prefix){closeCompletion();return;}
        size_t end=context.end;
        if(!context.commands)while(end<source.size()&&(CodeAssist::identifier(source[end])||(lua&&source[end]==L':')))++end;
        CHARRANGE range{(LONG)context.start,(LONG)end};SendMessageW(text,EM_EXSETSEL,0,(LPARAM)&range);
        SendMessageW(text,EM_REPLACESEL,TRUE,(LPARAM)entries[index].name.c_str());closeCompletion();SetFocus(text);
    }
    void showProblems() {
        if(!problemsWindow) {
            problemsWindow=tool(L"Code problems — double-click to go to source",700,280);
            summary=CreateWindowW(L"STATIC",L"",WS_CHILD|WS_VISIBLE,0,0,1,1,problemsWindow,nullptr,GetModuleHandleW(nullptr),nullptr);
            problemsList=CreateWindowW(L"LISTBOX",L"",WS_CHILD|WS_VISIBLE|WS_VSCROLL|WS_HSCROLL|LBS_NOTIFY|WS_TABSTOP|(Theme_enabled()?LBS_OWNERDRAWFIXED|LBS_HASSTRINGS:0),0,0,1,1,problemsWindow,nullptr,GetModuleHandleW(nullptr),nullptr);
            for(HWND h:{summary,problemsList})SendMessageW(h,WM_SETFONT,(WPARAM)GetStockObject(DEFAULT_GUI_FONT),TRUE);
        }
        updateProblems();layout();ShowWindow(problemsWindow,SW_SHOWNOACTIVATE);
    }
    void updateProblems() {
        if(!problemsWindow)return;
        bool stale=checked!=observed;
        SetWindowTextW(summary,stale?L"Checking current source...":lua?L"Lua syntax only; runtime checks use the debugger. Double-click an error or press F4 in the editor.":L"Praat block structure only (includes not expanded); expressions and commands are checked at runtime.");
        SendMessageW(problemsList,LB_RESETCONTENT,0,0);
        if(stale)return;
        if(problems.empty())SendMessageW(problemsList,LB_ADDSTRING,0,(LPARAM)L"No problems found by this check.");
        for(auto &p:problems){auto label=L"Line "+std::to_wstring(p.line)+L": "+p.message;SendMessageW(problemsList,LB_ADDSTRING,0,(LPARAM)label.c_str());}
    }
    void go(size_t index) {
        if(checked!=sourceText(text)||index>=problems.size())return;
        CodeFolding_action(widget,2);CodeFolding_sync(widget);
        long start=CodeFolding_lineStart(widget,(long)problems[index].line-1);if(start<0)return;
        CHARRANGE range{start,start};SendMessageW(text,EM_EXSETSEL,0,(LPARAM)&range);SendMessageW(text,EM_SCROLLCARET,0,0);
        SetForegroundWindow(GetAncestor(text,GA_ROOT));SetFocus(text);
    }
    void next() {
        if(checked!=sourceText(text)||problems.empty()){showProblems();return;}
        CHARRANGE range;SendMessageW(text,EM_EXGETSEL,0,(LPARAM)&range);CodeFolding_sync(widget);
        long line=CodeFolding_lineAt(widget,range.cpMin)+1;
        for(size_t i=0;i<problems.size();++i)if(problems[i].line>(size_t)line){go(i);return;}go(0);
    }
    void tick() {
        if(SendMessageW(text,EM_GETOPTIONS,0,0)&ECO_READONLY){closeCompletion();return;}
        auto source=sourceText(text);
        if(source!=observed){observed=std::move(source);changedAt=GetTickCount64();updateProblems();InvalidateRect(text,nullptr,FALSE);}
        if(job.valid()&&job.wait_for(std::chrono::milliseconds(0))==std::future_status::ready) {
            try {auto result=job.get();if(submitted==observed){problems=std::move(result);checked=submitted;updateProblems();}}
            catch(...){checked=submitted;problems={{1,L"Syntax checker could not finish"}};updateProblems();}
            busy=false;InvalidateRect(text,nullptr,FALSE);
        }
        if(!busy&&checked!=observed&&GetTickCount64()-changedAt>=400) {
            if(observed.size()>1000000){checked=observed;problems={{1,L"Live checking disabled for documents over 1,000,000 characters"}};updateProblems();return;}
            submitted=observed;
            try {job=std::async(std::launch::async,analyse,submitted,lua);busy=true;}
            catch(...){checked=submitted;problems={{1,L"Cannot start syntax checker"}};updateProblems();}
        }
    }
    void paintProblems() {
        if(problems.empty()||checked!=observed||checked!=sourceText(text))return;
        HDC dc=GetDC(text);RECT rect;GetClientRect(text,&rect);
        auto font=SelectObject(dc,(HFONT)SendMessageW(text,WM_GETFONT,0,0));TEXTMETRICW metrics{};GetTextMetricsW(dc,&metrics);
        auto pen=CreatePen(PS_SOLID,1,Theme_nativeColour("editor.error",RGB(210,35,35)));auto oldPen=SelectObject(dc,pen);
        for(auto &p:problems) {
            long line=(long)p.line-1;
            if(CodeFolding_hiddenLine(widget,line))continue;
            long start=CodeFolding_lineStart(widget,line);if(start<0)continue;
            POINTL pos{};SendMessageW(text,EM_POSFROMCHAR,(WPARAM)&pos,start);
            long y=pos.y+metrics.tmHeight-1;
            if(y<0||y>=rect.bottom)continue;
            auto content=CodeFolding_lineText(widget,line);SIZE size{};GetTextExtentPoint32W(dc,content.c_str(),(int)content.size(),&size);
            long end=std::min(rect.right-4,pos.x+std::max(24L,size.cx));
            MoveToEx(dc,std::max(74L,pos.x),y,nullptr);
            for(long x=std::max(74L,pos.x);x<end;x+=4){LineTo(dc,x+2,y+2);LineTo(dc,x+4,y);}
        }
        SelectObject(dc,oldPen);DeleteObject(pen);SelectObject(dc,font);ReleaseDC(text,dc);
    }
};
std::map<GuiText,std::unique_ptr<Assist>> assistants;
LRESULT CALLBACK windowProc(HWND w,UINT message,WPARAM wp,LPARAM lp,UINT_PTR,DWORD_PTR data) {
    auto self=(Assist*)data;
    if(message==WM_CLOSE){ShowWindow(w,SW_HIDE);return 0;}
    if(message==WM_SIZE)self->layout();
    if(message==WM_MOUSEACTIVATE&&w==self->popup)return MA_NOACTIVATE;
    if(message==WM_COMMAND) {
        if((HWND)lp==self->list){if(HIWORD(wp)==LBN_DBLCLK)self->accept();else {self->selectedHint();SetFocus(self->text);}return 0;}
        if((HWND)lp==self->problemsList&&HIWORD(wp)==LBN_DBLCLK){auto i=SendMessageW(self->problemsList,LB_GETCURSEL,0,0);if(i>=0)self->go((size_t)i);return 0;}
    }
    return DefSubclassProc(w,message,wp,lp);
}
LRESULT CALLBACK assistProc(HWND w,UINT message,WPARAM wp,LPARAM lp,UINT_PTR,DWORD_PTR data) {
    auto self=(Assist*)data;
    if(message==WM_TIMER&&wp==assistId){self->tick();return 0;}
    if(message==WM_KEYDOWN) {
        self->swallow=false;
        if(wp==VK_SPACE&&GetKeyState(VK_CONTROL)<0){self->complete(GetKeyState(VK_SHIFT)<0);self->swallow=true;return 0;}
        if(wp==VK_F4){self->next();return 0;}
        if(self->visible()) {
            if((wp==VK_RETURN||wp==VK_TAB)&&!self->helpOnly){self->accept();self->swallow=true;return 0;}
            if(wp==VK_ESCAPE){self->closeCompletion();self->swallow=true;return 0;}
            if(wp==VK_UP||wp==VK_DOWN){auto i=SendMessageW(self->list,LB_GETCURSEL,0,0);i=std::max<LRESULT>(0,std::min<LRESULT>((LRESULT)self->entries.size()-1,i+(wp==VK_UP?-1:1)));SendMessageW(self->list,LB_SETCURSEL,i,0);self->selectedHint();return 0;}
            self->closeCompletion();
        }
    }
    if(message==WM_CHAR&&self->swallow){self->swallow=false;return 0;}
    if((message==WM_KILLFOCUS&&(HWND)wp!=self->list)||message==WM_LBUTTONDOWN||message==WM_VSCROLL)self->closeCompletion();
    auto result=DefSubclassProc(w,message,wp,lp);
    if(message==WM_PAINT)self->paintProblems();
    if(message==WM_CHAR&&wp==L'.'&&self->lua)self->complete();
    if(message==WM_CHAR&&((wp==L'('&&self->lua)||wp==L','||(!self->lua&&wp==L':')))self->complete(true);
    return result;
}
}
void CodeAssist_install(GuiText text,bool lua) {
    auto self=std::make_unique<Assist>(text,lua);SetWindowSubclass(self->text,assistProc,assistId,(DWORD_PTR)self.get());SetTimer(self->text,assistId,200,nullptr);assistants[text]=std::move(self);
}
void CodeAssist_destroy(GuiText text) {
    auto it=assistants.find(text);if(it==assistants.end())return;
    KillTimer(it->second->text,assistId);RemoveWindowSubclass(it->second->text,assistProc,assistId);assistants.erase(it);
}
void CodeAssist_action(GuiText text,int action) {
    auto it=assistants.find(text);if(it==assistants.end())return;auto &self=*it->second;
    if(action==0)self.complete();else if(action==1){self.tick();self.showProblems();}else if(action==3)self.complete(true);else self.next();
}
#else
void CodeAssist_install(GuiText,bool){}
void CodeAssist_destroy(GuiText){}
void CodeAssist_action(GuiText,int){}
#endif
