// Praat Pro Edition. GPL-3.0-or-later.
#include "Theme.h"
#include "GuiP.h"
#if defined(_WIN32)
#include <windows.h>
#include <commctrl.h>
#include <richedit.h>
#include <map>
#include <string>
#include <vector>
#include <algorithm>
#include <cwctype>
namespace {
struct MenuLabel {HMENU menu,subMenu;std::wstring text;bool bar;UINT type;};
std::map<ULONG_PTR,MenuLabel> menuLabels;
ULONG_PTR nextLabel=1;
COLORREF colour(const char *key,COLORREF fallback=0){return Theme_nativeColour(key,fallback);}
HBRUSH brush(COLORREF c){static std::map<COLORREF,HBRUSH> brushes;auto &b=brushes[c];if(!b)b=CreateSolidBrush(c);return b;}
std::wstring className(HWND w){wchar_t name[128]{};GetClassNameW(w,name,128);return name;}
void title(HWND w){
    if(GetWindowLongPtrW(w,GWL_STYLE)&WS_CHILD)return;
    using DwmAttribute=HRESULT(WINAPI*)(HWND,DWORD,LPCVOID,DWORD);
    static auto module=LoadLibraryW(L"dwmapi.dll");
    static auto set=module?(DwmAttribute)GetProcAddress(module,"DwmSetWindowAttribute"):nullptr;
    if(set){BOOL dark=Theme_dark();set(w,20,&dark,sizeof dark);
        auto bg=colour("window.background"),fg=colour("window.text");set(w,35,&bg,sizeof bg);set(w,36,&fg,sizeof fg);}
}
void prepareMenu(HMENU menu,bool bar=false){
    static bool busy=false;if(!menu||!IsMenu(menu)||busy)return;busy=true;
    for(auto it=menuLabels.begin();it!=menuLabels.end();)if(!IsMenu(it->second.menu))it=menuLabels.erase(it);else ++it;
    MENUINFO info{};info.cbSize=sizeof info;info.fMask=MIM_BACKGROUND;GetMenuInfo(menu,&info);
    auto background=brush(colour("window.background"));if(info.hbrBack!=background){info.hbrBack=background;SetMenuInfo(menu,&info);}
    std::vector<ULONG_PTR> used;
    for(int i=0;i<GetMenuItemCount(menu);++i){
        MENUITEMINFOW item{};item.cbSize=sizeof item;item.fMask=MIIM_FTYPE|MIIM_DATA|MIIM_SUBMENU;
        if(!GetMenuItemInfoW(menu,i,TRUE,&item))continue;
        auto found=menuLabels.find(item.dwItemData);
        if(!(item.fType&MFT_OWNERDRAW)||found==menuLabels.end()){
            wchar_t label[2048]{};MENUITEMINFOW text{};text.cbSize=sizeof text;text.fMask=MIIM_STRING;text.dwTypeData=label;text.cch=2047;
            GetMenuItemInfoW(menu,i,TRUE,&text);item.dwItemData=nextLabel++;
            menuLabels.emplace(item.dwItemData,MenuLabel{menu,item.hSubMenu,label,bar,item.fType});
            item.fType|=MFT_OWNERDRAW;item.fMask=MIIM_FTYPE|MIIM_DATA;SetMenuItemInfoW(menu,i,TRUE,&item);
        }else {found->second.subMenu=item.hSubMenu;found->second.bar=bar;}
        used.push_back(item.dwItemData);
    }
    for(auto it=menuLabels.begin();it!=menuLabels.end();)
        if(it->second.menu==menu&&std::find(used.begin(),used.end(),it->first)==used.end())it=menuLabels.erase(it);else ++it;
    busy=false;
}
void drawMenu(HWND owner,DRAWITEMSTRUCT *d){
    MENUITEMINFOW fresh{};fresh.cbSize=sizeof fresh;fresh.fMask=MIIM_DATA;
    auto data=d->itemData;
    if(!menuLabels.count(data)&&GetMenuItemInfoW((HMENU)d->hwndItem,d->itemID,FALSE,&fresh)&&menuLabels.count(fresh.dwItemData))data=fresh.dwItemData;
    auto found=menuLabels.find(data);if(found==menuLabels.end())return;auto &item=found->second;
    bool selected=d->itemState&ODS_SELECTED,disabled=d->itemState&(ODS_DISABLED|ODS_GRAYED);
    FillRect(d->hDC,&d->rcItem,brush(colour(selected?"window.selection":"window.background")));
    SetBkMode(d->hDC,TRANSPARENT);SetTextColor(d->hDC,colour(disabled?"editor.line_number":selected?"window.selection_text":"window.text"));
    RECT r=d->rcItem;
    if(item.type&MFT_SEPARATOR){r.top=(r.top+r.bottom)/2;r.bottom=r.top+1;r.left+=8;r.right-=8;FillRect(d->hDC,&r,brush(colour("window.border")));return;}
    auto font=SelectObject(d->hDC,GetStockObject(DEFAULT_GUI_FONT));
    r.left+=item.bar?4:24;r.right-=item.bar?4:20;
    std::wstring label=item.text;
    if(item.bar){
        auto shell=(GuiObject)GetWindowLongPtrW(owner,GWLP_USERDATA);
        if(shell&&shell->widgetClass==xmShellWidgetClass&&shell->nat.shell.menuBar)
            for(auto child=shell->nat.shell.menuBar->firstChild;child;child=child->nextSibling)
                if(child->widgetClass==xmPulldownMenuWidgetClass&&child->nat.menu.id==(int)d->itemID){label=Melder_peek32toW(child->name.get());break;}
    }
    auto tab=label.find(L'\t');
    if(tab!=std::wstring::npos){auto shortcut=label.substr(tab+1);label.resize(tab);DrawTextW(d->hDC,shortcut.c_str(),-1,&r,DT_RIGHT|DT_VCENTER|DT_SINGLELINE|DT_NOPREFIX);}
    DrawTextW(d->hDC,label.c_str(),-1,&r,DT_LEFT|DT_VCENTER|DT_SINGLELINE|((d->itemState&ODS_NOACCEL)?DT_HIDEPREFIX:0));
    if(d->itemState&ODS_CHECKED){RECT check=d->rcItem;check.left+=6;check.right=check.left+16;DrawTextW(d->hDC,L"\x2713",1,&check,DT_CENTER|DT_VCENTER|DT_SINGLELINE);}
    if(item.subMenu&&!item.bar){RECT arrow=d->rcItem;arrow.left=arrow.right-17;DrawTextW(d->hDC,L"\x203a",1,&arrow,DT_CENTER|DT_VCENTER|DT_SINGLELINE);}
    SelectObject(d->hDC,font);
}
void drawButton(HWND w,HDC dc,RECT r,UINT state){
    auto style=GetWindowLongPtrW(w,GWL_STYLE)&BS_TYPEMASK;
    bool disabled=!IsWindowEnabled(w),pressed=state&CDIS_SELECTED;
    bool radio=style==BS_RADIOBUTTON||style==BS_AUTORADIOBUTTON;
    bool toggle=radio||style==BS_CHECKBOX||style==BS_AUTOCHECKBOX||style==BS_3STATE||style==BS_AUTO3STATE;
    FillRect(dc,&r,brush(colour(toggle||style==BS_GROUPBOX?"window.background":pressed?"window.selection":"window.control_background")));
    SetBkMode(dc,TRANSPARENT);SetTextColor(dc,colour(disabled?"editor.line_number":"window.text"));
    if(toggle){RECT box{r.left+2,(r.top+r.bottom)/2-6,r.left+15,(r.top+r.bottom)/2+7};
        if(radio){auto pen=CreatePen(PS_SOLID,1,colour("window.border"));auto oldPen=SelectObject(dc,pen);auto oldBrush=SelectObject(dc,brush(colour("window.background")));Ellipse(dc,box.left,box.top,box.right,box.bottom);SelectObject(dc,oldPen);SelectObject(dc,oldBrush);DeleteObject(pen);}
        else FrameRect(dc,&box,brush(colour("window.border")));
        if(SendMessageW(w,BM_GETCHECK,0,0)!=BST_UNCHECKED){InflateRect(&box,-3,-3);FillRect(dc,&box,brush(colour("analysis.pitch")));}r.left+=21;
    }else {FrameRect(dc,&r,brush(colour("window.border")));InflateRect(&r,-4,-2);}
    wchar_t text[2048]{};GetWindowTextW(w,text,2048);auto old=SelectObject(dc,(HFONT)SendMessageW(w,WM_GETFONT,0,0));
    DrawTextW(dc,text,-1,&r,DT_SINGLELINE|(style==BS_GROUPBOX?DT_TOP:DT_VCENTER)|(toggle||style==BS_GROUPBOX?DT_LEFT:DT_CENTER));SelectObject(dc,old);
    if(state&CDIS_FOCUS){InflateRect(&r,-2,-2);DrawFocusRect(dc,&r);}
}
LRESULT CALLBACK themed(HWND w,UINT msg,WPARAM wp,LPARAM lp,UINT_PTR,DWORD_PTR){
    if(msg==WM_ACTIVATE&&LOWORD(wp)!=WA_INACTIVE){
        EnumChildWindows(w,[](HWND child,LPARAM)->BOOL{SetWindowSubclass(child,themed,0x505254,0);InvalidateRect(child,nullptr,TRUE);return TRUE;},0);
    }
    if(msg>=WM_CTLCOLORMSGBOX&&msg<=WM_CTLCOLORSTATIC){
        auto dc=(HDC)wp;auto child=(HWND)lp;auto name=className(child);
        bool edit=name==L"Edit"||name.find(L"RICHEDIT")!=std::wstring::npos;
        auto bg=colour(edit?"editor.background":msg==WM_CTLCOLORLISTBOX?"window.control_background":"window.background");
        SetTextColor(dc,colour(edit?"editor.text":"window.text"));SetBkColor(dc,bg);SetBkMode(dc,OPAQUE);return (LRESULT)brush(bg);
    }
    if(msg==WM_ERASEBKGND){auto name=className(w);if(name.find(L"Praat")!=std::wstring::npos||name==L"Static"){
        RECT r;GetClientRect(w,&r);FillRect((HDC)wp,&r,brush(colour("window.background")));return 1;}}
    if(msg==WM_PAINT&&className(w)==L"Button"){
        PAINTSTRUCT paint;HDC dc=BeginPaint(w,&paint);RECT r;GetClientRect(w,&r);
        auto state=SendMessageW(w,BM_GETSTATE,0,0);drawButton(w,dc,r,((state&BST_PUSHED)?CDIS_SELECTED:0)|((state&BST_FOCUS)?CDIS_FOCUS:0));
        EndPaint(w,&paint);return 0;
    }
    if(msg==WM_SHOWWINDOW)title(w);
    if(msg==WM_SHOWWINDOW||msg==WM_INITMENU||msg==WM_NCPAINT){prepareMenu(GetMenu(w),true);
        
        if(className(w).find(L"RICHEDIT")!=std::wstring::npos){SendMessageW(w,EM_SETBKGNDCOLOR,0,colour("editor.background"));}}
    if(msg==WM_INITMENUPOPUP)prepareMenu((HMENU)wp);
    if(msg==WM_MENUCHAR){
        HMENU menu=(HMENU)lp;
        for(int i=0;i<GetMenuItemCount(menu);++i){MENUITEMINFOW item{};item.cbSize=sizeof item;item.fMask=MIIM_DATA;GetMenuItemInfoW(menu,i,TRUE,&item);
            auto found=menuLabels.find(item.dwItemData);if(found==menuLabels.end())continue;auto &label=found->second.text;
            for(std::size_t j=0;j+1<label.size();++j)if(label[j]==L'&'){
                if(label[j+1]==L'&'){++j;continue;}
                if(std::towlower(label[j+1])==std::towlower((wchar_t)LOWORD(wp)))return MAKELRESULT(i,MNC_EXECUTE);
            }
        }
    }
    if(msg==WM_MEASUREITEM){auto m=(MEASUREITEMSTRUCT*)lp;if(m&&m->CtlType==ODT_LISTBOX){m->itemHeight=18;return TRUE;}if(m&&m->CtlType==ODT_MENU){
        auto found=menuLabels.find(m->itemData);if(found==menuLabels.end())return DefSubclassProc(w,msg,wp,lp);
        auto &item=found->second;HDC dc=GetDC(w);auto old=SelectObject(dc,GetStockObject(DEFAULT_GUI_FONT));SIZE size{};
        GetTextExtentPoint32W(dc,item.text.c_str(),(int)item.text.size(),&size);SelectObject(dc,old);ReleaseDC(w,dc);
        m->itemWidth=size.cx+(item.bar?4:56);m->itemHeight=(item.type&MFT_SEPARATOR)?8:22;return TRUE;
    }}
    if(msg==WM_DRAWITEM){auto d=(DRAWITEMSTRUCT*)lp;
        if(d&&d->CtlType==ODT_LISTBOX){bool selected=d->itemState&ODS_SELECTED;
            FillRect(d->hDC,&d->rcItem,brush(colour(selected?"window.selection":"window.control_background")));
            if(d->itemID!=(UINT)-1){auto n=SendMessageW(d->hwndItem,LB_GETTEXTLEN,d->itemID,0);if(n>=0){std::wstring label(n+1,L'\0');SendMessageW(d->hwndItem,LB_GETTEXT,d->itemID,(LPARAM)label.data());
                auto old=SelectObject(d->hDC,(HFONT)SendMessageW(d->hwndItem,WM_GETFONT,0,0));SetBkMode(d->hDC,TRANSPARENT);SetTextColor(d->hDC,colour(selected?"window.selection_text":"window.text"));RECT r=d->rcItem;r.left+=3;DrawTextW(d->hDC,label.c_str(),(int)n,&r,DT_LEFT|DT_VCENTER|DT_SINGLELINE|DT_NOPREFIX);SelectObject(d->hDC,old);
            }}if(d->itemState&ODS_FOCUS)DrawFocusRect(d->hDC,&d->rcItem);return TRUE;
        }
        if(d&&d->CtlType==ODT_MENU){drawMenu(w,d);return TRUE;}}
    if(msg==WM_NCDESTROY)RemoveWindowSubclass(w,themed,0x505254);
    return DefSubclassProc(w,msg,wp,lp);
}
LRESULT CALLBACK watch(int code,WPARAM wp,LPARAM lp){
    if(code>=0){auto c=(CWPSTRUCT*)lp;
        if(c->message==WM_CREATE||c->message==WM_WINDOWPOSCHANGED||(c->message==WM_SHOWWINDOW&&c->wParam))SetWindowSubclass(c->hwnd,themed,0x505254,0);
        if(c->message==WM_PARENTNOTIFY&&LOWORD(c->wParam)==WM_CREATE)SetWindowSubclass((HWND)c->lParam,themed,0x505254,0);
    }
    return CallNextHookEx(nullptr,code,wp,lp);
}
}
void Theme_installWindows(){if(!Theme_enabled())return;static HHOOK hook=SetWindowsHookExW(WH_CALLWNDPROC,watch,nullptr,GetCurrentThreadId());(void)hook;}
#endif
