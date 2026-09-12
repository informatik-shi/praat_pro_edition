#include "PlotHost.h"
#include "EditorM.h"
#include <map>
#include <algorithm>
namespace {std::map<long,PlotHost> hosts;long nextId=1;}
void PlotHost_register(Editor editor,const char *type,std::function<PlotHostContext()> context,std::function<void()> changed){long id=nextId++;hosts.emplace(id,PlotHost{id,editor,type,std::move(context),std::move(changed),nullptr,nullptr,true});}
PlotHost *PlotHost_find(Editor editor){for(auto &entry:hosts)if(entry.second.editor==editor)return &entry.second;return nullptr;}
PlotHost &PlotHost_get(long id){auto it=hosts.find(id);Melder_require(it!=hosts.end(),U"Plot editor is closed or does not exist.");return it->second;}
void PlotHost_remove(Editor editor){auto h=PlotHost_find(editor);if(h)hosts.erase(h->id);}
std::vector<PlotHost*> PlotHost_all(){std::vector<PlotHost*> result;for(auto &entry:hosts)result.push_back(&entry.second);return result;}
void PlotHost_attach(long id,const PlotFigure &figure,const std::string &target){
    auto &h=PlotHost_get(id);Melder_require(figure.axes.size()==1&&figure.rows==1&&figure.cols==1,U"Embedded plots require exactly one axes (1x1).");
    Melder_require(target=="panel"||target=="spectrogram",U"Plot target must be panel or spectrogram.");
    if(target=="spectrogram")for(auto &s:figure.axes[0].series)Melder_require(s.kind=="line"||s.kind=="scatter",U"Spectrogram overlays support lines and scatter only.");
    auto snapshot=std::make_unique<PlotFigure>(figure);
    (target=="panel"?h.panel:h.overlay)=std::move(snapshot);h.visible=true;h.changed();
}
void PlotHost_detach(long id,const std::string &target){auto &h=PlotHost_get(id);Melder_require(target=="all"||target=="panel"||target=="spectrogram",U"Unknown plot target.");if(target!="spectrogram")h.panel.reset();if(target!="panel")h.overlay.reset();h.changed();}
void PlotHost_clear(Editor editor){auto h=PlotHost_find(editor);if(h){h->panel.reset();h->overlay.reset();}}
bool PlotHost_panel(Editor editor){auto h=PlotHost_find(editor);return h&&h->visible&&h->panel;}
static void toggle(Editor me,EDITOR_ARGS){auto h=PlotHost_find(me);if(h){h->visible=!h->visible;h->changed();}}
static void clear(Editor me,EDITOR_ARGS){auto h=PlotHost_find(me);if(h)PlotHost_detach(h->id,"all");}
void PlotHost_addMenu(Editor me){Editor_addMenu(me,U"Lua plots",0);Editor_addCommand(me,U"Lua plots",U"Show / hide Lua plots",0,toggle);Editor_addCommand(me,U"Lua plots",U"Remove Lua plots",0,clear);}
void PlotHost_draw(Editor editor,Graphics g,PlotRect panel,PlotRect overlay,double ymin,double ymax,bool overlayVisible){
    auto h=PlotHost_find(editor);if(!h||!h->visible)return;auto c=h->context();if(c.start>=c.end)return;
    double vx1,vx2,vy1,vy2,x1,x2,y1,y2;Graphics_inqViewport(g,&vx1,&vx2,&vy1,&vy2);Graphics_inqWindow(g,&x1,&x2,&y1,&y2);auto colour=Graphics_inqColour(g);double width=Graphics_inqLineWidth(g),fontSize=Graphics_inqFontSize(g);int lineType=Graphics_inqLineType(g);Graphics_setLineType(g,Graphics_DRAWN);Graphics_setFontSize(g,10);
    if(h->overlay&&overlayVisible&&ymin<ymax){Graphics_setViewport(g,overlay.left,overlay.right,overlay.bottom,overlay.top);Plot_drawData(g,h->overlay->axes[0],c.start,c.end,ymin,ymax);}
    if(h->panel){
        const auto &a=h->panel->axes[0];Graphics_setViewport(g,panel.left,panel.right,panel.bottom,panel.top);Graphics_setWindow(g,0,1,0,1);
        Graphics_setColour(g,Melder_WHITE);Graphics_fillRectangle(g,0,1,0,1);
        if(c.selectionStart<c.selectionEnd){Graphics_setColour(g,MelderColour(.92,.95,1));double left=std::max(0.,(c.selectionStart-c.start)/(c.end-c.start)),right=std::min(1.,(c.selectionEnd-c.start)/(c.end-c.start));if(left<right)Graphics_fillRectangle(g,left,right,0,1);}
        if(a.grid){Graphics_setColour(g,MelderColour(.86));Graphics_setLineWidth(g,1);for(int k=1;k<4;++k){Graphics_line(g,0,k/4.,1,k/4.);Graphics_line(g,k/4.,0,k/4.,1);}}
        Plot_drawData(g,a,c.start,c.end,a.ymin,a.ymax);
        Graphics_setWindow(g,0,1,0,1);Graphics_setColour(g,Melder_BLACK);Graphics_setLineWidth(g,1);Graphics_rectangle(g,0,1,0,1);
        Graphics_setTextAlignment(g,Graphics_LEFT,Graphics_TOP);Graphics_text(g,.01,.98,U"Lua: ",a.title.c_str(),U"  [",Melder_half(a.ymin),U", ",Melder_half(a.ymax),U"] ",a.ylabel.c_str());
        if(c.selectionStart>=c.start&&c.selectionStart<=c.end){double x=(c.selectionStart-c.start)/(c.end-c.start);Graphics_line(g,x,0,x,1);}
    }
    Graphics_setViewport(g,vx1,vx2,vy1,vy2);Graphics_setWindow(g,x1,x2,y1,y2);Graphics_setColour(g,colour);Graphics_setLineWidth(g,width);Graphics_setLineType(g,lineType);Graphics_setFontSize(g,fontSize);
}
