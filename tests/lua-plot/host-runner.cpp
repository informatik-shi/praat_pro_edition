#include "praat.h"
#include "LuaRuntime.h"
#include "luaplot/PlotHost.h"
#include "../main/main_Praat.h"
int main(int,char**){
    try {
        char a0[]="plot-host-tests",a1[]="--run",a2[]="tests/lua-plot/test.praat",a3[]="--no-pref-files",a4[]="--utf8";char *args[]={a0,a3,a4,a1,a2};
        praat_init(U"Praat",U"" stringize(PRAAT_VERSION_STR),PRAAT_VERSION_NUM,PRAAT_YEAR,PRAAT_MONTH,PRAAT_DAY,U"",U"",5,args);
        INCLUDE_LIBRARY(praat_uvafon_init)
        auto editor=Thing_new(Editor);int changes=0;
        PlotHostContext context{0,2,.5,1.5,.7,1.1};
        PlotHost_register(editor.get(),"TestHost",[&](){return context;},[&](){++changes;});
        auto h=PlotHost_find(editor.get());long id=h->id;
        Lua_run(U"local p=require('praat.plot')\nlocal e=p.editors()[1]\nassert(e.start==.5 and e.finish==1.5 and e.selection_start==.7)\nlocal f,a=p.subplots()\na:plot({0,1,2},{1,2,3})\nf:attach(e.id,'panel')\nf:attach(e.id,'spectrogram')\na.series[1].y[1]=99\n",nullptr,false,[]{return true;});
        Melder_require(changes==2&&h->panel&&h->overlay&&h->panel->axes[0].series[0].y[0]==1,U"Attachments must copy Lua data.");
        Lua_run(U"local p=require('praat.plot')\nlocal id=p.editors()[1].id\nlocal f,a=p.subplots()\na:bar({1},{2})\nassert(not pcall(function() f:attach(id,'spectrogram') end))\nlocal g=p.subplots(2,1)\nassert(not pcall(function() g:attach(id,'panel') end))\nassert(not pcall(function() f:attach(id,'bad') end))\np.detach(id,'panel')\n",nullptr,false,[]{return true;});
        Melder_require(!h->panel&&h->overlay&&changes==3,U"Detach must affect requested slot only.");
        context.start=1;context.end=2;Melder_require(h->context().start==1,U"Context must be live.");
        PlotHost_clear(editor.get());Melder_require(!h->overlay,U"Data change invalidates attachments.");
        PlotHost_remove(editor.get());bool rejected=false;try{PlotHost_get(id);}catch(MelderError){rejected=true;Melder_clearError();}
        Melder_require(rejected&&PlotHost_all().empty(),U"Closed editors must not leave stale handles.");
        Lua_run(U"assert(#require('praat.plot').editors()==0)",nullptr,false,[]{return true;});
        Melder_casual(U"LUA PLOT HOST TESTS: PASS");return 0;
    }catch(MelderError){Melder_flushError();return 1;}
}
