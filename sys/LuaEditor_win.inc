// Windows code view. Included in LuaEditor.cpp to keep platform details private.
#if defined (_WIN32)
#include <richole.h>
#include <tom.h>
#include <map>
#include <set>
#include <string>
namespace {
bool luaAlpha(wchar_t c) {return (c>=L'a'&&c<=L'z')||(c>=L'A'&&c<=L'Z');}
bool luaDigit(wchar_t c) {return c>=L'0'&&c<=L'9';}
bool luaAlnum(wchar_t c) {return luaAlpha(c)||luaDigit(c);}
struct LuaColourSpan { long start, end; COLORREF colour; };
struct LuaView {
    LuaEditor editor;
    HWND text, gutter;
    ITextDocument *document=nullptr;
    std::wstring previous;
    std::vector<LuaColourSpan> spans;
    long oldFirst=-1;
    bool formatting=false;
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
    void refresh() {
        if(formatting||editor->running)return;
        RECT r;GetClientRect(text,&r);SetWindowPos(gutter,HWND_TOP,0,0,54,r.bottom,SWP_NOACTIVATE);
        InvalidateRect(gutter,nullptr,FALSE);
        if(!document)return;
        GETTEXTLENGTHEX length{GTL_NUMCHARS|GTL_PRECISE,1200};
        long n=(long)SendMessageW(text,EM_GETTEXTLENGTHEX,(WPARAM)&length,0);
        std::wstring source(n+1,L'\0');GETTEXTEX get{(DWORD)((n+1)*sizeof(wchar_t)),GT_RAWTEXT,1200,nullptr,nullptr};
        SendMessageW(text,EM_GETTEXTEX,(WPARAM)&get,(LPARAM)source.data());source.resize(n);
        long first=(long)SendMessageW(text,EM_GETFIRSTVISIBLELINE,0,0);
        bool edited=source!=previous;
        if(!edited&&first==oldFirst)return;
        if(edited){scan(source);previous=std::move(source);}
        oldFirst=first;
        long start=(long)SendMessageW(text,EM_LINEINDEX,first,0);
        long last=(long)SendMessageW(text,EM_LINEINDEX,first+r.bottom/8+2,0);if(last<0)last=n;
        formatting=true;
        CHARRANGE selection;POINT scroll;
        SendMessageW(text,EM_EXGETSEL,0,(LPARAM)&selection);SendMessageW(text,EM_GETSCROLLPOS,0,(LPARAM)&scroll);
        auto modified=SendMessageW(text,EM_GETMODIFY,0,0),mask=SendMessageW(text,EM_SETEVENTMASK,0,0);
        document->Undo(tomSuspend,nullptr);SendMessageW(text,WM_SETREDRAW,FALSE,0);
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
    if(message==WM_PAINT) {
        PAINTSTRUCT paint;HDC dc=BeginPaint(window,&paint);RECT r;GetClientRect(window,&r);
        FillRect(dc,&r,(HBRUSH)(COLOR_BTNFACE+1));SetBkMode(dc,TRANSPARENT);SetTextColor(dc,RGB(105,110,120));
        auto old=SelectObject(dc,(HFONT)SendMessageW(self->text,WM_GETFONT,0,0));
        long first=(long)SendMessageW(self->text,EM_GETFIRSTVISIBLELINE,0,0);
        long count=(long)SendMessageW(self->text,EM_GETLINECOUNT,0,0);
        for(long line=first;line<count;++line) {
            long index=(long)SendMessageW(self->text,EM_LINEINDEX,line,0);POINTL pos;
            SendMessageW(self->text,EM_POSFROMCHAR,(WPARAM)&pos,index);if(pos.y>r.bottom)break;
            std::wstring label=std::to_wstring(line+1);RECT row{0,pos.y,46,pos.y+24};DrawTextW(dc,label.c_str(),-1,&row,DT_RIGHT|DT_SINGLELINE|DT_NOPREFIX);
        }
        SelectObject(dc,old);EndPaint(window,&paint);return 0;
    }
    return DefSubclassProc(window,message,wp,lp);
}
LRESULT CALLBACK luaTextProc(HWND window,UINT message,WPARAM wp,LPARAM lp,UINT_PTR,DWORD_PTR data) {
    auto self=(LuaView*)data;
    if(message==WM_SIZE)self->oldFirst=-1;
    if(message==WM_TIMER&&wp==775){self->refresh();return 0;}
    if(message==WM_KEYDOWN&&(wp==VK_F5||wp==VK_F7)) {
        if(wp==VK_F5 && GetKeyState(VK_SHIFT)<0) {self->editor->stopRequested=true;return 0;}
        try {execute(self->editor,false,wp==VK_F7);} catch(MelderError){Melder_flushError();}
        return 0;
    }
    return DefSubclassProc(window,message,wp,lp);
}
}
static void createLuaView(LuaEditor editor) {
    auto self=new LuaView;self->editor=editor;self->text=editor->textWidget->d_widget->window;
    luaViews[editor]=self;
    IUnknown *unknown=nullptr;SendMessageW(self->text,EM_GETOLEINTERFACE,0,(LPARAM)&unknown);
    if(unknown){static const IID iid={0x8cc497c0,0xa1df,0x11ce,{0x80,0x98,0x00,0xaa,0x00,0x47,0xbe,0x5d}};unknown->QueryInterface(iid,(void**)&self->document);unknown->Release();}
    SendMessageW(self->text,EM_SETMARGINS,EC_LEFTMARGIN|EC_RIGHTMARGIN,MAKELPARAM(58,4));
    self->gutter=CreateWindowW(L"STATIC",L"",WS_CHILD|WS_VISIBLE,0,0,54,100,self->text,nullptr,GetModuleHandleW(nullptr),nullptr);
    SetWindowSubclass(self->gutter,luaGutter,775,(DWORD_PTR)self);SetWindowSubclass(self->text,luaTextProc,775,(DWORD_PTR)self);
    SetTimer(self->text,775,180,nullptr);
}
static void destroyLuaView(LuaEditor editor) {
    auto it=luaViews.find(editor);if(it==luaViews.end())return;auto self=it->second;
    KillTimer(self->text,775);RemoveWindowSubclass(self->text,luaTextProc,775);DestroyWindow(self->gutter);
    if(self->document)self->document->Release();delete self;luaViews.erase(it);
}
#else
static void createLuaView(LuaEditor) {}
static void destroyLuaView(LuaEditor) {}
#endif
