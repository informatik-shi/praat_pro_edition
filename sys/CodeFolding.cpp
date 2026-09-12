// Praat Custom. GPL-3.0-or-later.
#include "codefold/CodeFolding.h"
#if defined (_WIN32)
#include "GuiP.h"
#include <richedit.h>
#include <richole.h>
#include <tom.h>
#include "codefold/FoldModel.h"
#include <map>
namespace {
struct View;
LRESULT CALLBACK textProc(HWND,UINT,WPARAM,LPARAM,UINT_PTR,DWORD_PTR);
LRESULT CALLBACK laneProc(HWND,UINT,WPARAM,LPARAM,UINT_PTR,DWORD_PTR);
std::map<GuiText,View*> views;
struct View {
    HWND text,lane;
    ITextDocument *document=nullptr;
    CodeFold::Model model;
    int left;
    bool lua,busy=false;
    unsigned long revision=0;
    std::vector<std::pair<RECT,size_t>> hits;
    std::vector<bool> hidden;
    void indexHidden() {
        std::vector<int> change(model.lines.size()+1,0);
        for(auto &b:model.blocks)if(b.collapsed){++change[b.first+1];--change[b.last];}
        hidden.resize(model.lines.size());int depth=0;
        for(size_t i=0;i<hidden.size();++i){depth+=change[i];hidden[i]=depth>0;}
    }
    bool isHidden(size_t line)const{return line<hidden.size()&&hidden[line];}
    std::wstring read() {
        GETTEXTLENGTHEX length{GTL_NUMCHARS|GTL_PRECISE,1200};
        long n=(long)SendMessageW(text,EM_GETTEXTLENGTHEX,(WPARAM)&length,0);
        std::wstring s(n+1,L'\0');GETTEXTEX get{(DWORD)((n+1)*sizeof(wchar_t)),GT_RAWTEXT,1200,nullptr,nullptr};
        SendMessageW(text,EM_GETTEXTEX,(WPARAM)&get,(LPARAM)s.data());s.resize(n);return s;
    }
    void range(long a,long b,bool hidden) {
        CHARRANGE r{a,b};SendMessageW(text,EM_EXSETSEL,0,(LPARAM)&r);
        CHARFORMAT2W f{};f.cbSize=sizeof(f);f.dwMask=CFM_HIDDEN;f.dwEffects=hidden?CFE_HIDDEN:0;
        SendMessageW(text,EM_SETCHARFORMAT,SCF_SELECTION,(LPARAM)&f);
    }
    void apply() {
        if(!document||busy)return;
        indexHidden();
        ++revision;
        busy=true;
        CHARRANGE selection;POINT scroll;
        SendMessageW(text,EM_EXGETSEL,0,(LPARAM)&selection);SendMessageW(text,EM_GETSCROLLPOS,0,(LPARAM)&scroll);
        auto modified=SendMessageW(text,EM_GETMODIFY,0,0),mask=SendMessageW(text,EM_SETEVENTMASK,0,0);
        document->Undo(tomSuspend,nullptr);SendMessageW(text,WM_SETREDRAW,FALSE,0);
        range(0,-1,false);
        for(auto &b:model.blocks)if(b.collapsed) {
            long a=(long)model.lines[b.first+1],end=(long)model.lines[b.last];
            range(a,end,true);
            if(selection.cpMin>=a&&selection.cpMin<end)selection={(long)model.lines[b.first],(long)model.lines[b.first]};
        }
        SendMessageW(text,EM_EXSETSEL,0,(LPARAM)&selection);SendMessageW(text,EM_SETSCROLLPOS,0,(LPARAM)&scroll);
        SendMessageW(text,EM_SETMODIFY,modified,0);SendMessageW(text,EM_SETEVENTMASK,0,mask);
        document->Undo(tomResume,nullptr);SendMessageW(text,WM_SETREDRAW,TRUE,0);
        busy=false;RedrawWindow(text,nullptr,nullptr,RDW_INVALIDATE|RDW_ALLCHILDREN);
    }
    bool any()const{for(auto &b:model.blocks)if(b.collapsed)return true;return false;}
    void expand() {if(!any())return;for(auto &b:model.blocks)b.collapsed=false;apply();}
    void sync() {
        if(busy)return;
        auto s=read();
        if(s!=model.source) {
            bool had=any();model.parse(std::move(s),lua);
            indexHidden();
            if(had)apply();
        }
        RECT r;GetClientRect(text,&r);SetWindowPos(lane,HWND_TOP,left,0,18,r.bottom,SWP_NOACTIVATE);
        InvalidateRect(lane,nullptr,FALSE);
    }
    void reveal(long position) {
        if(busy||!any())return;
        bool changed=false;
        for(auto &b:model.blocks)if(b.collapsed&&position>=(long)model.lines[b.first+1]&&position<(long)model.lines[b.last]){b.collapsed=false;changed=true;}
        if(changed)apply();
    }
    void action(int mode) {
        sync();if(!document)return;
        if(mode==2){expand();return;}
        if(mode==1){for(auto &b:model.blocks)b.collapsed=true;apply();return;}
        CHARRANGE selection;SendMessageW(text,EM_EXGETSEL,0,(LPARAM)&selection);
        size_t line=model.lineAt(selection.cpMin);
        CodeFold::Block *chosen=nullptr;
        for(auto &b:model.blocks)if(line>=b.first&&line<=b.last)chosen=&b;
        if(chosen){chosen->collapsed=!chosen->collapsed;apply();}
    }
};
LRESULT CALLBACK textProc(HWND w,UINT m,WPARAM wp,LPARAM lp,UINT_PTR,DWORD_PTR data) {
    auto self=(View*)data;
    if(m==WM_TIMER&&wp==779){self->sync();return 0;}
    if(!self->busy) {
        if(m==WM_KEYDOWN&&wp==VK_F8){self->action(GetKeyState(VK_SHIFT)<0?2:GetKeyState(VK_CONTROL)<0?1:0);return 0;}
        if((m==EM_SETSEL||m==EM_EXSETSEL)&&(SendMessageW(w,EM_GETEVENTMASK,0,0)&ENM_SELCHANGE)) {
            long position=m==EM_SETSEL?(long)wp:((CHARRANGE*)lp)->cpMin;
            if(position>=0)self->reveal(position);
        }
        // Expand BEFORE editing: hidden code must never be accidentally removed.
        if(m==WM_CHAR||m==WM_CUT||m==WM_CLEAR||m==WM_PASTE||m==WM_SETTEXT||m==EM_SETTEXTEX||m==EM_REPLACESEL||m==EM_STREAMIN||m==WM_UNDO||m==EM_UNDO||m==EM_REDO||
            (m==WM_KEYDOWN&&(wp==VK_DELETE||wp==VK_BACK)))self->expand();
        if(m==WM_PASTE)return DefSubclassProc(w,EM_PASTESPECIAL,CF_UNICODETEXT,0);
    }
    return DefSubclassProc(w,m,wp,lp);
}
LRESULT CALLBACK laneProc(HWND w,UINT m,WPARAM wp,LPARAM lp,UINT_PTR,DWORD_PTR data) {
    auto self=(View*)data;
    if(m==WM_LBUTTONDOWN) {
        POINT p{(short)LOWORD(lp),(short)HIWORD(lp)};
        for(auto &hit:self->hits)if(PtInRect(&hit.first,p)) {
            self->model.blocks[hit.second].collapsed=!self->model.blocks[hit.second].collapsed;self->apply();break;
        }return 0;
    }
    if(m==WM_PAINT) {
        PAINTSTRUCT ps;HDC dc=BeginPaint(w,&ps);RECT r;GetClientRect(w,&r);
        FillRect(dc,&r,(HBRUSH)(COLOR_BTNFACE+1));self->hits.clear();
        long first=(long)SendMessageW(self->text,EM_LINEINDEX,SendMessageW(self->text,EM_GETFIRSTVISIBLELINE,0,0),0);
        POINTL bottom{self->left+24,r.bottom};
        long last=(long)SendMessageW(self->text,EM_CHARFROMPOS,0,(LPARAM)&bottom);
        for(size_t i=0;i<self->model.blocks.size();++i) {
            auto &b=self->model.blocks[i];if(self->isHidden(b.first))continue;
            long offset=(long)self->model.lines[b.first];if(offset<first)continue;if(offset>last)break;
            POINTL p;SendMessageW(self->text,EM_POSFROMCHAR,(WPARAM)&p,self->model.lines[b.first]);
            if(p.y<0||p.y>=r.bottom)continue;
            RECT box{2,p.y+2,14,p.y+14};Rectangle(dc,box.left,box.top,box.right,box.bottom);
            MoveToEx(dc,4,p.y+8,nullptr);LineTo(dc,12,p.y+8);
            if(b.collapsed){MoveToEx(dc,8,p.y+4,nullptr);LineTo(dc,8,p.y+12);}
            self->hits.push_back({box,i});
        }
        EndPaint(w,&ps);return 0;
    }
    return DefSubclassProc(w,m,wp,lp);
}
}
void CodeFolding_install(GuiText widget,int left,bool lua) {
    auto self=new View;self->text=widget->d_widget->window;self->left=left;self->lua=lua;views[widget]=self;
    IUnknown *unknown=nullptr;SendMessageW(self->text,EM_GETOLEINTERFACE,0,(LPARAM)&unknown);
    if(unknown){static const IID iid={0x8cc497c0,0xa1df,0x11ce,{0x80,0x98,0x00,0xaa,0x00,0x47,0xbe,0x5d}};unknown->QueryInterface(iid,(void**)&self->document);unknown->Release();}
    self->model.parse(self->read(),lua);
    self->lane=CreateWindowW(L"STATIC",L"Code folding (F8)",WS_CHILD|WS_VISIBLE|SS_NOTIFY,left,0,18,100,self->text,nullptr,GetModuleHandleW(nullptr),nullptr);
    SendMessageW(self->text,EM_SETMARGINS,EC_LEFTMARGIN|EC_RIGHTMARGIN,MAKELPARAM(left+22,4));
    SetWindowSubclass(self->lane,laneProc,779,(DWORD_PTR)self);SetWindowSubclass(self->text,textProc,779,(DWORD_PTR)self);
    SetTimer(self->text,779,180,nullptr);
}
void CodeFolding_destroy(GuiText widget) {
    auto it=views.find(widget);if(it==views.end())return;auto self=it->second;
    KillTimer(self->text,779);RemoveWindowSubclass(self->text,textProc,779);DestroyWindow(self->lane);
    if(self->document)self->document->Release();delete self;views.erase(it);
}
bool CodeFolding_hiddenLine(GuiText widget,long line) {auto it=views.find(widget);return it!=views.end()&&it->second->isHidden(line);}
void CodeFolding_sync(GuiText widget){auto it=views.find(widget);if(it!=views.end())it->second->sync();}
unsigned long CodeFolding_revision(GuiText widget){auto it=views.find(widget);return it==views.end()?0:it->second->revision;}
long CodeFolding_lineCount(GuiText widget){auto it=views.find(widget);return it==views.end()?0:(long)it->second->model.lines.size();}
long CodeFolding_lineStart(GuiText widget,long line){auto it=views.find(widget);if(it==views.end()||line<0||line>=(long)it->second->model.lines.size())return -1;return (long)it->second->model.lines[line];}
long CodeFolding_lineAt(GuiText widget,long position){auto it=views.find(widget);return it==views.end()?0:(long)it->second->model.lineAt(std::max(0L,position));}
long CodeFolding_firstVisible(GuiText widget){auto it=views.find(widget);if(it==views.end())return 0;HWND text=it->second->text;return CodeFolding_lineAt(widget,(long)SendMessageW(text,EM_LINEINDEX,SendMessageW(text,EM_GETFIRSTVISIBLELINE,0,0),0));}
long CodeFolding_lastVisible(GuiText widget,long first,long count){long total=CodeFolding_lineCount(widget),last=first;for(;last+1<total&&count>0;++last)if(!CodeFolding_hiddenLine(widget,last))--count;return last;}
std::wstring CodeFolding_lineText(GuiText widget,long line){auto it=views.find(widget);if(it==views.end())return {};auto &model=it->second->model;if(line<0||line>=(long)model.lines.size())return {};size_t start=model.lines[line],end=line+1<(long)model.lines.size()?model.lines[line+1]:model.source.size();while(end>start&&(model.source[end-1]==L'\r'||model.source[end-1]==L'\n'))--end;return model.source.substr(start,end-start);}
void CodeFolding_action(GuiText widget,int action){auto it=views.find(widget);if(it!=views.end())it->second->action(action);}
#else
void CodeFolding_install(GuiText,int,bool){}
void CodeFolding_destroy(GuiText){}
bool CodeFolding_hiddenLine(GuiText,long){return false;}
void CodeFolding_action(GuiText,int){}
#endif
