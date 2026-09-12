// Retained plot attachments; hosts own no Lua pointers.
#ifndef PRAAT_PLOT_HOST_H
#define PRAAT_PLOT_HOST_H
#include "PlotModel.h"
#include "Editor.h"
#include <functional>
#include <memory>
struct PlotHostContext {
    double tmin=0,tmax=0,start=0,end=0,selectionStart=0,selectionEnd=0;
};
struct PlotHost {
    long id;
    Editor editor;
    std::string type;
    std::function<PlotHostContext()> context;
    std::function<void()> changed;
    std::unique_ptr<PlotFigure> panel,overlay;
    bool visible=true;
};
void PlotHost_register(Editor,const char *,std::function<PlotHostContext()>,std::function<void()>);
void PlotHost_remove(Editor);
PlotHost *PlotHost_find(Editor);
PlotHost &PlotHost_get(long);
std::vector<PlotHost*> PlotHost_all();
void PlotHost_attach(long,const PlotFigure &,const std::string &);
void PlotHost_detach(long,const std::string &);
void PlotHost_clear(Editor);
bool PlotHost_panel(Editor);
void PlotHost_draw(Editor,Graphics,PlotRect panel,PlotRect overlay,double ymin,double ymax,bool overlayVisible);
void PlotHost_addMenu(Editor);
#endif
