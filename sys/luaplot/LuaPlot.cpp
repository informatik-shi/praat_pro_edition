// Praat Custom. GPL-3.0-or-later.
#include "LuaPlot.h"
#include "PlotModel.h"
#include "../../external/lua-5.5.1/src/lua.h"
#include "../../external/lua-5.5.1/src/lauxlib.h"
#include <algorithm>
#include <cmath>
#include <cstring>
namespace {
void field(lua_State *L,int i,const char *key){i=lua_absindex(L,i);lua_pushstring(L,key);lua_rawget(L,i);}
double number(lua_State *L,int i){double v=luaL_checknumber(L,i);if(!std::isfinite(v)||std::abs(v)>1e100)luaL_error(L,"Plot numbers must be finite and within +/-1e100");return v;}
double num(lua_State *L,int i,const char *key,double fallback){field(L,i,key);double v=lua_isnil(L,-1)?fallback:number(L,-1);lua_pop(L,1);return v;}
int integerField(lua_State *L,int i,const char *key,int fallback,int max){double v=num(L,i,key,fallback);if(v<1||v>max||v!=std::floor(v))luaL_error(L,"%s: integer 1..%d required",key,max);return (int)v;}
std::string str(lua_State *L,int i,const char *key,const char *fallback="") {
    field(L,i,key);std::string s=fallback;
    if(!lua_isnil(L,-1)){size_t n;const char *p=luaL_checklstring(L,-1,&n);if(n>4096||std::memchr(p,0,n))luaL_error(L,"Plot label too long or contains NUL");s.assign(p,n);}lua_pop(L,1);return s;
}
std::u32string text(lua_State *L,int i,const char *key,const char *fallback=""){auto s=str(L,i,key,fallback);auto u=Melder_8to32_e(s.c_str());return u.get();}
bool flag(lua_State *L,int i,const char *key,bool fallback){field(L,i,key);bool v=lua_isnil(L,-1)?fallback:lua_toboolean(L,-1);lua_pop(L,1);return v;}
std::vector<double> array(lua_State *L,int i,size_t &budget) {
    luaL_checktype(L,i,LUA_TTABLE);i=lua_absindex(L,i);size_t n=lua_rawlen(L,i);
    if(n>budget)luaL_error(L,"Figure exceeds one million numeric values");budget-=n;
    std::vector<double> values;values.reserve(n);
    for(size_t k=1;k<=n;++k){lua_rawgeti(L,i,k);values.push_back(number(L,-1));lua_pop(L,1);}return values;
}
bool limits(lua_State *L,int i,const char *key,double &a,double &b) {
    field(L,i,key);if(lua_isnil(L,-1)){lua_pop(L,1);return false;}size_t budget=2;auto v=array(L,-1,budget);lua_pop(L,1);
    if(v.size()!=2||v[0]>=v[1])luaL_error(L,"%s must contain two increasing numbers",key);a=v[0];b=v[1];return true;
}
MelderColour colour(lua_State *L,int i) {
    auto s=str(L,i,"color","#1f77b4");
    if(s=="red")s="#d62728";if(s=="blue")s="#1f77b4";if(s=="green")s="#2ca02c";if(s=="black")s="#000000";if(s=="orange")s="#ff7f0e";
    if(s.size()!=7||s[0]!='#')luaL_error(L,"color: use #RRGGBB or red/blue/green/black/orange");
    unsigned value=0;for(size_t k=1;k<7;++k){char c=s[k];int d=c>='0'&&c<='9'?c-'0':c>='a'&&c<='f'?c-'a'+10:c>='A'&&c<='F'?c-'A'+10:-1;if(d<0)luaL_error(L,"Invalid hexadecimal color");value=value*16+d;}
    return MelderColour(((value>>16)&255)/255.,((value>>8)&255)/255.,(value&255)/255.);
}
PlotFigure parse(lua_State *L) {
    luaL_checktype(L,1,LUA_TTABLE);PlotFigure f;size_t budget=1000000;
    f.rows=integerField(L,1,"rows",1,4);f.cols=integerField(L,1,"cols",1,4);
    f.width=num(L,1,"width",9);f.height=num(L,1,"height",6);f.title=text(L,1,"title","Lua Figure");
    if(f.width<3||f.width>24||f.height<3||f.height>24)luaL_error(L,"Figure width/height must be 3..24 inches");
    field(L,1,"axes");luaL_checktype(L,-1,LUA_TTABLE);int axes=lua_gettop(L);size_t count=lua_rawlen(L,axes);
    if(count<1||count>16)luaL_error(L,"Figure requires 1..16 axes");bool used[16]{};
    for(size_t k=1;k<=count;++k) {
        lua_rawgeti(L,axes,k);luaL_checktype(L,-1,LUA_TTABLE);int ai=lua_gettop(L);PlotAxes a;
        a.row=integerField(L,ai,"row",1,f.rows);a.col=integerField(L,ai,"col",1,f.cols);
        int cell=(a.row-1)*f.cols+a.col-1;if(used[cell])luaL_error(L,"Duplicate subplot cell");used[cell]=true;
        a.title=text(L,ai,"title");a.xlabel=text(L,ai,"xlabel");a.ylabel=text(L,ai,"ylabel");
        a.grid=flag(L,ai,"grid_enabled",true);a.legend=flag(L,ai,"legend_enabled",false);
        bool data=false;auto extend=[&](double x,double y){if(!data){a.xmin=a.xmax=x;a.ymin=a.ymax=y;data=true;}else{a.xmin=std::min(a.xmin,x);a.xmax=std::max(a.xmax,x);a.ymin=std::min(a.ymin,y);a.ymax=std::max(a.ymax,y);}};
        field(L,ai,"series");luaL_checktype(L,-1,LUA_TTABLE);int si=lua_gettop(L);size_t n=lua_rawlen(L,si);
        if(n>128)luaL_error(L,"At most 128 series per axes");
        for(size_t j=1;j<=n;++j) {
            lua_rawgeti(L,si,j);luaL_checktype(L,-1,LUA_TTABLE);int item=lua_gettop(L);PlotSeries s;s.kind=str(L,item,"kind");s.label=text(L,item,"label");
            if(s.kind=="heatmap") {
                field(L,item,"z");luaL_checktype(L,-1,LUA_TTABLE);int zi=lua_gettop(L);s.rows=(int)lua_rawlen(L,zi);
                if(s.rows<1||s.rows>1000)luaL_error(L,"Heatmap needs 1..1000 rows");
                for(int r=1;r<=s.rows;++r){lua_rawgeti(L,zi,r);auto values=array(L,-1,budget);lua_pop(L,1);if(r==1)s.cols=(int)values.size();if(s.cols<1||s.cols>1000||values.size()!=(size_t)s.cols)luaL_error(L,"Heatmap must be rectangular, with 1..1000 columns");s.z.insert(s.z.end(),values.begin(),values.end());}lua_pop(L,1);
                field(L,item,"extent");size_t b=4;auto e=array(L,-1,b);lua_pop(L,1);if(e.size()!=4||e[0]>=e[1]||e[2]>=e[3])luaL_error(L,"extent: xmin,xmax,ymin,ymax in increasing order");
                s.left=e[0];s.right=e[1];s.bottom=e[2];s.top=e[3];extend(s.left,s.bottom);extend(s.right,s.top);
            } else {
                if(s.kind!="line"&&s.kind!="scatter"&&s.kind!="bar")luaL_error(L,"Unknown plot kind");
                field(L,item,"x");s.x=array(L,-1,budget);lua_pop(L,1);field(L,item,"y");s.y=array(L,-1,budget);lua_pop(L,1);
                if(s.x.empty()||s.x.size()!=s.y.size())luaL_error(L,"x and y must have the same nonzero length");
                s.colour=colour(L,item);s.width=num(L,item,"linewidth",1.5);s.size=num(L,item,"size",1.5);s.barWidth=num(L,item,"width",0.8);
                if(s.width<=0||s.width>20||s.size<=0||s.size>20||s.barWidth<=0)luaL_error(L,"Invalid line, marker or bar width");
                for(size_t p=0;p<s.x.size();++p){extend(s.x[p],s.y[p]);if(s.kind=="bar"){extend(s.x[p]-s.barWidth/2,0);extend(s.x[p]+s.barWidth/2,0);}}
            }
            a.series.push_back(std::move(s));lua_pop(L,1);
        }lua_pop(L,1);
        auto pad=[](double &lo,double &hi){double d=hi-lo;if(d==0)d=std::max(1.,std::abs(lo)*0.1);lo-=d*0.05;hi+=d*0.05;};
        pad(a.xmin,a.xmax);pad(a.ymin,a.ymax);limits(L,ai,"xlim",a.xmin,a.xmax);limits(L,ai,"ylim",a.ymin,a.ymax);
        f.axes.push_back(std::move(a));lua_pop(L,1);
    }lua_pop(L,1);return f;
}
int error(lua_State *L){lua_pushstring(L,Melder_peek32to8(Melder_getError()));Melder_clearError();return lua_error(L);}
int show(lua_State *L){try{auto f=parse(L);long id=(long)luaL_optinteger(L,2,0);if(Melder_batch)return luaL_error(L,"show() requires GUI; use savefig() in batch mode");lua_pushinteger(L,Plot_show(std::move(f),id));return 1;}catch(MelderError){return error(L);}}
int close(lua_State *L){Plot_close((long)luaL_checkinteger(L,1));return 0;}
int save(lua_State *L){try {
    auto f=parse(L);size_t n;const char *path=luaL_checklstring(L,2,&n);if(std::strlen(path)!=n||n<4||std::string(path+n-4)!=".png")return luaL_error(L,"savefig currently supports .png files");
    lua_Integer requested=luaL_optinteger(L,3,150);if(requested!=100&&requested!=150&&requested!=300&&requested!=600)return luaL_error(L,"dpi must be 100, 150, 300 or 600");
    int dpi=(int)requested;if(f.width*dpi*f.height*dpi>40000000)return luaL_error(L,"Image must be at most 40 megapixels");
    auto p=Melder_8to32_e(path);structMelderFile file;Melder_relativePathToFile(p.get(),&file);
    {auto g=Graphics_create_pngfile(&file,dpi,0,f.width,0,f.height);Graphics_setWsWindow(g.get(),0,1,0,1);Plot_draw(g.get(),f);}
    if(!MelderFile_exists(&file))Melder_throw(U"PNG export failed: ",p.get());return 0;
}catch(MelderError){return error(L);}}
int open(lua_State *L) {
    static const char source[]=
#include "plot.lua.inc"
    ;
    if(luaL_loadbuffer(L,source,sizeof(source)-1,"@praat.plot")!=LUA_OK)return lua_error(L);
    lua_newtable(L);const luaL_Reg functions[]={{"show",show},{"save",save},{"close",close},{nullptr,nullptr}};luaL_setfuncs(L,functions,0);
    lua_call(L,1,1);return 1;
}
}
void LuaPlot_install(lua_State *L){luaL_getsubtable(L,LUA_REGISTRYINDEX,LUA_PRELOAD_TABLE);lua_pushcfunction(L,open);lua_setfield(L,-2,"praat.plot");lua_pop(L,1);}
