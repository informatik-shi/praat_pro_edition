// Praat Custom. GPL-3.0-or-later.
#include "PlotModel.h"
#include <algorithm>
#include <cmath>
#include <cstdio>
PlotRect Plot_rect(const PlotFigure &f,const PlotAxes &a) {
    double w=1./f.cols,h=.88/f.rows;
    return {(a.col-1)*w+.17*w,a.col*w-.06*w, .04+(f.rows-a.row)*h+.20*h,.04+(f.rows-a.row+1)*h-.12*h};
}
namespace {
void label(Graphics g,double x,double y,const std::u32string &s,kGraphics_horizontalAlignment align=Graphics_CENTRE,int vertical=Graphics_HALF){Graphics_setTextAlignment(g,align,vertical);Graphics_text(g,x,y,s.c_str());}
std::u32string tick(double n){char b[64];std::snprintf(b,sizeof b,"%.4g",n);auto t=Melder_8to32_e(b);return t.get();}
bool clip(double &x1,double &y1,double &x2,double &y2) {
    double dx=x2-x1,dy=y2-y1,t0=0,t1=1;
    const double p[]={-dx,dx,-dy,dy},q[]={x1,1-x1,y1,1-y1};
    for(int k=0;k<4;++k){if(p[k]==0){if(q[k]<0)return false;}else{double t=q[k]/p[k];if(p[k]<0)t0=std::max(t0,t);else t1=std::min(t1,t);if(t0>t1)return false;}}
    x2=x1+t1*dx;y2=y1+t1*dy;x1+=t0*dx;y1+=t0*dy;return true;
}
}
void Plot_draw(Graphics g,const PlotFigure &f) {
    Graphics_setViewport(g,0,1,0,1);Graphics_setWindow(g,0,1,0,1);
    Graphics_setColour(g,Melder_WHITE);Graphics_fillRectangle(g,0,1,0,1);
    Graphics_setPercentSignIsItalic(g,false);Graphics_setNumberSignIsBold(g,false);Graphics_setUnderscoreIsSubscript(g,false);Graphics_setCircumflexIsSuperscript(g,false);Graphics_setDollarSignIsCode(g,false);Graphics_setAtSignIsLink(g,false);
    Graphics_setFont(g,kGraphics_font::HELVETICA);Graphics_setFontSize(g,14);Graphics_setColour(g,Melder_BLACK);label(g,.5,.97,f.title);
    for(const auto &a:f.axes) {
        auto r=Plot_rect(f,a);double w=r.right-r.left,h=r.top-r.bottom;
        Graphics_setViewport(g,0,1,0,1);Graphics_setWindow(g,0,1,0,1);Graphics_setFontSize(g,10);Graphics_setColour(g,Melder_BLACK);Graphics_setLineWidth(g,1);
        label(g,(r.left+r.right)/2,r.top+.028/f.rows,a.title);
        label(g,(r.left+r.right)/2,r.bottom-.095/f.rows,a.xlabel);
        Graphics_setTextRotation(g,90);label(g,r.left-.12/f.cols,(r.bottom+r.top)/2,a.ylabel);Graphics_setTextRotation(g,0);
        for(int k=0;k<=4;++k){double t=k/4.;
            if(a.grid){Graphics_setColour(g,MelderColour(.88));Graphics_line(g,r.left+t*w,r.bottom,r.left+t*w,r.top);Graphics_line(g,r.left,r.bottom+t*h,r.right,r.bottom+t*h);}
            Graphics_setColour(g,Melder_BLACK);Graphics_setFontSize(g,8);
            label(g,r.left+t*w,r.bottom-.018/f.rows,tick(a.xmin+t*(a.xmax-a.xmin)),Graphics_CENTRE,Graphics_TOP);
            label(g,r.left-.012/f.cols,r.bottom+t*h,tick(a.ymin+t*(a.ymax-a.ymin)),Graphics_RIGHT);
        }
        Graphics_setViewport(g,r.left,r.right,r.bottom,r.top);Graphics_setWindow(g,0,1,0,1);
        Plot_drawData(g,a,a.xmin,a.xmax,a.ymin,a.ymax);
        Graphics_setLineWidth(g,1);Graphics_setColour(g,Melder_BLACK);Graphics_rectangle(g,0,1,0,1);
        if(a.legend){int row=0;Graphics_setFontSize(g,9);for(const auto &s:a.series)if(!s.label.empty()&&row<8){double y=.95-.07*row++;Graphics_setColour(g,Melder_WHITE);Graphics_fillRectangle(g,.62,.99,y-.03,y+.03);Graphics_setColour(g,s.colour);Graphics_line(g,.64,y,.71,y);Graphics_setColour(g,Melder_BLACK);label(g,.73,y,s.label,Graphics_LEFT);}}
    }
}

void Plot_drawData(Graphics g,const PlotAxes &a,double xmin,double xmax,double ymin,double ymax) {
    Graphics_setWindow(g,0,1,0,1);
        auto X=[&](double x){return (x-xmin)/(xmax-xmin);};auto Y=[&](double y){return (y-ymin)/(ymax-ymin);};
        for(const auto &s:a.series) {
            Graphics_setColour(g,s.colour);Graphics_setLineWidth(g,s.width);
            if(s.kind=="heatmap") {
                auto bounds=std::minmax_element(s.z.begin(),s.z.end());double lo=*bounds.first,range=*bounds.second-lo;if(range==0)range=1;
                for(int row=0;row<s.rows;++row)for(int col=0;col<s.cols;++col){double v=(s.z[row*s.cols+col]-lo)/range;
                    // Monotonic blue-to-yellow palette; row 1 is the lower row.
                    Graphics_setColour(g,MelderColour(.12+.86*v,.14+.76*v,.48-.34*v));
                    double x1=std::max(0.,X(s.left+(s.right-s.left)*col/s.cols)),x2=std::min(1.,X(s.left+(s.right-s.left)*(col+1)/s.cols));
                    double y1=std::max(0.,Y(s.bottom+(s.top-s.bottom)*row/s.rows)),y2=std::min(1.,Y(s.bottom+(s.top-s.bottom)*(row+1)/s.rows));
                    if(x1<x2&&y1<y2)Graphics_fillRectangle(g,x1,x2,y1,y2);
                }
            } else for(size_t i=0;i<s.x.size();++i) {
                double x=X(s.x[i]),y=Y(s.y[i]);
                if(s.kind=="line"&&i){double x0=X(s.x[i-1]),y0=Y(s.y[i-1]);if(clip(x0,y0,x,y))Graphics_line(g,x0,y0,x,y);}
                else if(s.kind=="scatter"&&x>=0&&x<=1&&y>=0&&y<=1)Graphics_fillCircle_mm(g,x,y,s.size);
                else if(s.kind=="bar") {double x1=std::max(0.,X(s.x[i]-s.barWidth/2)),x2=std::min(1.,X(s.x[i]+s.barWidth/2));double y1=std::max(0.,std::min(y,Y(0))),y2=std::min(1.,std::max(y,Y(0)));if(x1<x2&&y1<y2)Graphics_fillRectangle(g,x1,x2,y1,y2);}
            }
        }
}
