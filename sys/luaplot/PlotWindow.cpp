// Praat Custom. GPL-3.0-or-later.
#include "PlotModel.h"
#include "Editor.h"
#include "EditorM.h"
#include "machine.h"
#include <map>
#include <algorithm>
Thing_define(LuaPlotWindow,Editor) {
    GuiDrawingArea area;
    autoGraphics graphics;
    PlotFigure figure,original;
    long id=0;
    int selected=0;
    double dragX=0,dragY=0;
    bool dragging=false;
    void v_createChildren() override;
    void v_createMenus() override;
    void v9_destroy() noexcept override;
    bool v_scriptable() override{return false;}
    bool v_hasEditMenu() override{return false;}
};
Thing_implement(LuaPlotWindow,Editor,0);
static std::map<long,LuaPlotWindow> windows;
static long nextId=1;
static void redraw(LuaPlotWindow me){if(my graphics)Graphics_updateWs(my graphics.get());}
static void expose(LuaPlotWindow me,GuiDrawingArea_ExposeEvent){if(my graphics)Plot_draw(my graphics.get(),my figure);}
static void resize(LuaPlotWindow me,GuiDrawingArea_ResizeEvent event){if(my graphics){Graphics_setWsViewport(my graphics.get(),0,event->width,0,event->height);redraw(me);}}
static void home(LuaPlotWindow me,EDITOR_ARGS){my figure=my original;redraw(me);}
static void zoomBy(LuaPlotWindow me,double factor){if(my figure.axes.empty())return;auto &a=my figure.axes[my selected];double x=(a.xmin+a.xmax)/2,y=(a.ymin+a.ymax)/2,dx=(a.xmax-a.xmin)*factor/2,dy=(a.ymax-a.ymin)*factor/2;if(dx<1e-14||dy<1e-14||dx>1e100||dy>1e100)return;a.xmin=x-dx;a.xmax=x+dx;a.ymin=y-dy;a.ymax=y+dy;redraw(me);}
static void zoomIn(LuaPlotWindow me,EDITOR_ARGS){zoomBy(me,.5);}
static void zoomOut(LuaPlotWindow me,EDITOR_ARGS){zoomBy(me,2);}
static void key(LuaPlotWindow me,GuiDrawingArea_KeyEvent e){if(e->key==U'+'||e->key==U'=')zoomBy(me,.5);else if(e->key==U'-')zoomBy(me,2);else if(e->key==U'h'||e->key==U'H'){my figure=my original;redraw(me);}}
static void mouse(LuaPlotWindow me,GuiDrawingArea_MouseEvent e){
    double x=(double)e->x/std::max<integer>(1,GuiControl_getWidth(my area));double y=1.-(double)e->y/std::max<integer>(1,GuiControl_getHeight(my area));
    if(e->isClick()){my dragging=false;for(size_t i=0;i<my figure.axes.size();++i){auto r=Plot_rect(my figure,my figure.axes[i]);if(x>=r.left&&x<=r.right&&y>=r.bottom&&y<=r.top){my selected=(int)i;my dragX=x;my dragY=y;my dragging=true;break;}}}
    if(e->isDrop()&&my dragging){my dragging=false;auto &a=my figure.axes[my selected];auto r=Plot_rect(my figure,a);x=std::clamp(x,r.left,r.right);y=std::clamp(y,r.bottom,r.top);
        if(std::abs(x-my dragX)>.015&&std::abs(y-my dragY)>.015){double dx=(a.xmax-a.xmin)/(r.right-r.left),dy=(a.ymax-a.ymin)/(r.top-r.bottom);double x1=a.xmin+(std::min(x,my dragX)-r.left)*dx,x2=a.xmin+(std::max(x,my dragX)-r.left)*dx,y1=a.ymin+(std::min(y,my dragY)-r.bottom)*dy,y2=a.ymin+(std::max(y,my dragY)-r.bottom)*dy;a.xmin=x1;a.xmax=x2;a.ymin=y1;a.ymax=y2;redraw(me);}}
}
void structLuaPlotWindow::v_createChildren(){area=GuiDrawingArea_createShown(windowForm,0,0,Machine_getMenuBarBottom(),0,expose,mouse,key,resize,nullptr,this,0);}
void structLuaPlotWindow::v_createMenus(){LuaPlotWindow_Parent::v_createMenus();Editor_addMenu(this,U"View",0);Editor_addCommand(this,U"View",U"Reset view",'H',home);Editor_addCommand(this,U"View",U"Zoom in",0,zoomIn);Editor_addCommand(this,U"View",U"Zoom out",0,zoomOut);}
void structLuaPlotWindow::v9_destroy() noexcept{windows.erase(id);LuaPlotWindow_Parent::v9_destroy();}
long Plot_show(PlotFigure figure,long existingId){
    auto it=windows.find(existingId);
    if(it!=windows.end()){auto me=it->second;my figure=figure;my original=std::move(figure);my selected=0;GuiShell_setTitle(my windowForm,my figure.title.c_str());redraw(me);Editor_raise(me);return existingId;}
    auto me=Thing_new(LuaPlotWindow);my figure=figure;my original=std::move(figure);
    Editor_init(me.get(),0,0,960,700,my figure.title.c_str(),nullptr);
    my graphics=Graphics_create_xmdrawingarea(my area);Graphics_setWsWindow(my graphics.get(),0,1,0,1);Graphics_setWsViewport(my graphics.get(),0,GuiControl_getWidth(my area),0,GuiControl_getHeight(my area));
    my id=nextId++;windows[my id]=me.get();long id=my id;redraw(me.get());me.releaseToUser();return id;
}
void Plot_close(long id){auto it=windows.find(id);if(it!=windows.end())forget_nozero(it->second);}
