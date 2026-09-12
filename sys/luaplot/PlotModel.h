// Retained, Lua-independent figure data for windows and future embedded views.
#ifndef PRAAT_PLOT_MODEL_H
#define PRAAT_PLOT_MODEL_H
#include "Graphics.h"
#include <string>
#include <vector>
struct PlotSeries {
    std::string kind;
    std::u32string label;
    std::vector<double> x,y,z;
    int rows=0,cols=0;
    double width=1.5,size=1.5,barWidth=0.8;
    double left=0,right=1,bottom=0,top=1;
    MelderColour colour {0.12,0.47,0.71};
};
struct PlotAxes {
    int row=1,col=1;
    std::u32string title,xlabel,ylabel;
    bool grid=true,legend=false;
    double xmin=0,xmax=1,ymin=0,ymax=1;
    std::vector<PlotSeries> series;
};
struct PlotFigure {
    int rows=1,cols=1;
    double width=9,height=6;
    std::u32string title=U"Lua Figure";
    std::vector<PlotAxes> axes;
};
struct PlotRect {double left,right,bottom,top;};
PlotRect Plot_rect(const PlotFigure &,const PlotAxes &);
void Plot_draw(Graphics,const PlotFigure &);
void Plot_drawData(Graphics,const PlotAxes &,double xmin,double xmax,double ymin,double ymax);
long Plot_show(PlotFigure,long existingId);
void Plot_close(long id);
#endif
