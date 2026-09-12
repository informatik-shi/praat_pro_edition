// Integration tests against the embedded Lua and real Praat libraries.
#include "praat.h"
#include "luadebug/LuaDebugRun.h"
#include "../main/main_Praat.h"
#include <vector>
static void run(conststring32 text,LuaDebugSession &debug) {
    structMelderFile file{};Melder_relativePathToFile(U"tests/lua-debugger/demo.lua",&file);
    Lua_runDebug(text,&file,[]{return true;},debug);
}
static conststring32 program=U"local function add(x)\n local y=x+1\n return y\nend\nlocal value=10\nvalue=add(value)\nvalue=value+2\nassert(value==13)\n";
int main(int,char**) {
    try {
        char a0[]="lua-debugger-tests",a1[]="--run",a2[]="tests/lua-debugger/demo.lua",a3[]="--no-pref-files",a4[]="--utf8";
        char *args[]={a0,a3,a4,a1,a2};
        praat_init(U"Praat",U"" stringize(PRAAT_VERSION_STR),PRAAT_VERSION_NUM,PRAAT_YEAR,PRAAT_MONTH,PRAAT_DAY,U"",U"",5,args);
        INCLUDE_LIBRARY(praat_uvafon_init)
        {
            LuaDebugSession d;d.breakpoints={6};std::vector<long> stops;
            d.paused=[&](LuaDebugSession &p){stops.push_back(p.line);
                if(stops.size()==1){Melder_require(p.variables.find("local\tvalue\t10")!=std::string::npos,U"Breakpoint must precede execution.");p.resume(LuaDebugStep::Into);}
                else if(stops.size()==2){Melder_require(p.line==2&&p.variables.find("local\tx\t10")!=std::string::npos,U"Step Into and arguments.");p.resume(LuaDebugStep::Over);}
                else if(stops.size()==3){Melder_require(p.line==3&&p.variables.find("local\ty\t11")!=std::string::npos,U"Step Over and local values.");Melder_require(p.callStack.find("demo.lua:6")!=std::string::npos,U"Caller frame missing.");p.resume(LuaDebugStep::Out);}
                else {Melder_require(p.line==7&&p.variables.find("local\tvalue\t11")!=std::string::npos,U"Step Out to caller.");p.resume(LuaDebugStep::Continue);}
            };
            run(program,d);Melder_require(stops==std::vector<long>({6,2,3,7})&&d.state==LuaDebugState::Finished,U"Stepping sequence.");
        }
        {
            LuaDebugSession d;d.breakpoints={6};int stops=0;
            d.paused=[&](LuaDebugSession&p){++stops;if(stops==1)p.resume(LuaDebugStep::Over);else{Melder_require(p.line==7,U"Over entered child function.");p.resume(LuaDebugStep::Continue);}};
            run(program,d);Melder_require(stops==2,U"Step Over count.");
        }
        {
            LuaDebugSession d;d.breakpoints={3};int stops=0;
            d.paused=[&](LuaDebugSession&p){++stops;if(stops==1){p.breakpoints.clear();p.resume(LuaDebugStep::Over);}else{Melder_require(p.line==4&&p.depth==2,U"Step Over recursion depth.");p.resume(LuaDebugStep::Continue);}};
            run(U"local function rec(n)\n if n>0 then\n  local v=rec(n-1)\n  return v+1\n end\n return 0\nend\nlocal result=rec(3)\nassert(result==3)\n",d);
            Melder_require(stops==2,U"Recursive Over count.");
        }
        {
            LuaDebugSession d;d.breakpoints={4};int stops=0;
            d.paused=[&](LuaDebugSession&p){++stops;Melder_require(p.variables.find("local\tt\t{")!=std::string::npos&&p.variables.find("upvalue\tcaptured\t7")!=std::string::npos,U"Table/upvalue snapshot.");p.resume(LuaDebugStep::Continue);};
            run(U"local captured=7\nlocal function f()\n local t=setmetatable({a=captured},{__tostring=function() error('INSPECTOR EXECUTED CODE') end,__pairs=function() error('PAIRS EXECUTED') end})\n assert(t.a==7)\nend\nf()\n",d);
            Melder_require(stops==1,U"Snapshot count.");
        }
        {
            LuaDebugSession d;d.breakpoints={3};int stops=0;
            d.paused=[&](LuaDebugSession&p){++stops;p.resume(LuaDebugStep::Continue);};
            run(U"local sum=0\nfor i=1,3 do\n sum=sum+i\nend\nassert(sum==6)\n",d);
            Melder_require(stops==3,U"Loop breakpoint must hit each iteration.");
        }
        {
            LuaDebugSession d;d.breakpoints={2};int stops=0;
            d.paused=[&](LuaDebugSession&p){++stops;p.resume(LuaDebugStep::Continue);};
            run(U"local co=coroutine.create(function()\n local sum=0\n for i=1,100000 do sum=sum+i end\n return sum\nend)\nlocal ok,value=coroutine.resume(co)\nassert(ok and value==5000050000)\n",d);
            Melder_require(stops==1,U"Coroutine breakpoint/resume semantics.");
        }
        {
            LuaDebugSession d;d.breakpoints={6};int stops=0;
            d.paused=[&](LuaDebugSession&p){++stops;if(stops==1)p.resume(LuaDebugStep::Over);else{Melder_require(p.line==7,U"Over entered a different coroutine.");p.resume(LuaDebugStep::Continue);}};
            run(U"local co=coroutine.create(function()\n local n=3\n n=n+1\n return n\nend)\nlocal ok,value=coroutine.resume(co)\nassert(ok and value==4)\n",d);Melder_require(stops==2,U"Coroutine Over count.");
        }
        {
            LuaDebugSession d;d.breakpoints={6};d.paused=[](LuaDebugSession&p){p.stop();};bool failed=false;
            try{run(program,d);}catch(MelderError){failed=true;Melder_clearError();}
            Melder_require(failed&&d.state==LuaDebugState::Stopped&&!d.thread,U"Stop must unwind and clear pointers.");
            d.breakpoints.clear();d.paused={};run(program,d);Melder_require(d.state==LuaDebugState::Finished,U"Restart after Stop.");
        }
        {
            LuaDebugSession d;bool failed=false;
            try{run(U"local function fail()\n error('expected debugger failure')\nend\nfail()\n",d);}catch(MelderError){failed=true;Melder_clearError();}
            Melder_require(failed&&d.state==LuaDebugState::Error&&d.line==2&&d.error.find("expected debugger failure")!=std::string::npos&&!d.thread,U"Error state and location.");
            run(program,d);Melder_require(d.state==LuaDebugState::Finished,U"Restart after Error.");
        }
        {
            LuaDebugSession d;d.breakpoints={2};int stops=0;
            d.paused=[&](LuaDebugSession&p){++stops;Melder_require(p.variables.find("local\tids\t{")!=std::string::npos,U"Praat object ids in Lua locals.");p.resume(LuaDebugStep::Continue);};
            run(U"local ids=praat.call('Create Sound from formula','debug_tone',1,0,1,44100,'0.1*sin(2*pi*440*x)')\nassert(praat.call('Get number of samples')==44100)\npraat.call('Remove')\n",d);
            Melder_require(stops==1,U"Praat API debugger integration.");
        }
        Melder_casual(U"LUA DEBUGGER TESTS: PASS");return 0;
    }catch(MelderError){Melder_flushError();return 1;}
}
