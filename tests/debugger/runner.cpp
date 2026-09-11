// Integration tests: actual Praat interpreter, same libraries as the desktop app.
// GPL-3.0-or-later.
#include "praat.h"
#include "Interpreter.h"
#include "ScriptDebugger.h"
#include "ScriptSyntax.h"
#include "../main/main_Praat.h"
#include <functional>
#include <vector>

struct Probe : ScriptDebugger {
    std::vector<std::pair<long long,int>> stops;
    std::function<void(Probe&)> atPause;
    int polls=0, stopAfter=0;
    void paused () override {
        stops.push_back({currentLine,current->callDepth});
        auto callback=atPause;
        if(callback)callback(*this);else resume(ScriptDebugStep::Continue);
    }
    void poll () override { if(stopAfter && ++polls>=stopAfter)stop(); }
};
static void run (conststring32 fileName,Probe &probe) {
    structMelderFile file {};Melder_relativePathToFile(fileName,&file);
    auto text=MelderFile_readText(&file);
    auto stack=InterpreterStack_create(nullptr);
    auto interpreter=Interpreter_createFromEnvironment(stack.get(),nullptr,&file);
    Interpreter_readParameters(interpreter.get(),text.get());
    Interpreter_rememberScript_override(interpreter.get(),&file,true);
    probe.start();interpreter->debugger=&probe;
    try {stack->runDown(interpreter.move(),text.move(),false);}
    catch(MelderError){if(probe.state!=ScriptDebugState::Error)throw;Melder_clearError();}
    probe.finish();probe.current=nullptr;
}
int main (int, char **) {
    try {
        char arg0[]="debugger-tests",arg1[]="--run",arg2[]="tests/debugger/test_variables.praat",arg3[]="--no-pref-files",arg4[]="--utf8";
        char *args[]={arg0,arg3,arg4,arg1,arg2};
        praat_init(U"Praat",U"" stringize(PRAAT_VERSION_STR),PRAAT_VERSION_NUM,PRAAT_YEAR,PRAAT_MONTH,PRAAT_DAY,U"",U"",5,args);
        INCLUDE_LIBRARY(praat_uvafon_init)
        {
            Probe p;p.breakpoints={2};p.atPause=[](Probe &s){
                Melder_require(Interpreter_hasVariable(s.current,U"x")->numericValue==1,U"Breakpoint must be before instruction.");
                s.resume(ScriptDebugStep::Over);
                // After first stop, only stop once at caller's next instruction.
                s.atPause=[](Probe &next){Melder_require(next.currentLine==3&&next.current->callDepth==0,U"Step Over entered procedure.");next.resume(ScriptDebugStep::Continue);};
            };
            run(U"tests/debugger/test_procedure.praat",p);
            Melder_require(p.stops.size()==2,U"Step Over stop count.");
        }
        {
            Probe p;p.breakpoints={2};p.atPause=[](Probe &s){
                size_t n=s.stops.size();
                if(n==1)s.resume(ScriptDebugStep::Into);
                else if(n==2){Melder_require(s.currentLine==6&&s.current->callDepth==1,U"Step Into.");s.resume(ScriptDebugStep::Over);}
                else if(n==3){Melder_require(s.currentLine==7,U"Next statement.");s.resume(ScriptDebugStep::Into);}
                else if(n==4){Melder_require(s.currentLine==10&&s.current->callDepth==2,U"Nested Into.");Melder_require(ScriptDebugger_stack(s.current).find(U"normalize")!=std::u32string::npos,U"Real call stack.");s.resume(ScriptDebugStep::Out);}
                else if(n==5){Melder_require(s.currentLine==8&&s.current->callDepth==1,U"Nested Out.");s.resume(ScriptDebugStep::Out);}
                else {Melder_require(s.currentLine==3&&s.current->callDepth==0,U"Out to main.");s.resume(ScriptDebugStep::Continue);}
            };
            run(U"tests/debugger/test_procedure.praat",p);Melder_require(p.stops.size()==6,U"Stepping stop count.");
        }
        {Probe p;p.breakpoints={3};run(U"tests/debugger/test_loop.praat",p);Melder_require(p.stops.size()==5,U"Loop breakpoint hit count.");}
        {Probe p;p.breakpoints={3,5,7};run(U"tests/debugger/test_if.praat",p);Melder_require(p.stops.size()==1&&p.stops[0].first==5,U"Skipped branches must not stop.");}
        {
            Probe p;p.breakpoints={8};p.atPause=[](Probe &s){
                Melder_require(ScriptDebugger_watch(s.current,U"x * 2")==U"30",U"Numeric Watch.");
                Melder_require(ScriptDebugger_watch(s.current,U"length (name$)")==U"11",U"String Watch.");
                Melder_require(ScriptDebugger_watch(s.current,U"randomUniform (0, 1)").find(U"not allowed")!=std::u32string::npos,U"Unsafe Watch.");
                Melder_require(ScriptDebugger_watch(s.current,U"stopwatch").find(U"not allowed")!=std::u32string::npos,U"Bare stateful symbol must be blocked.");
                Melder_require(ScriptDebugger_watch(s.current,U"1e-3 * x")==U"0.015",U"Exponent Watch.");
                auto variables=ScriptDebugger_variables(s.current);
                Melder_require(variables.find(U"matrix")!=std::u32string::npos&&variables.find(U"string vector")!=std::u32string::npos,U"Variable types.");
                s.resume(ScriptDebugStep::Continue);
            };run(U"tests/debugger/test_variables.praat",p);
        }
        {Probe p;run(U"tests/debugger/test_error.praat",p);Melder_require(p.state==ScriptDebugState::Error&&p.currentLine==3,U"Runtime error location.");}
        {Probe p;p.stopAfter=100;run(U"tests/debugger/test_stop.praat",p);Melder_require(p.polls==100,U"Stop in infinite loop.");}
        {Probe p;p.breakpoints={1,2,3,4};run(U"tests/debugger/test_continuation.praat",p);Melder_require(p.stops.size()==2&&p.stops[1].first==4,U"Continuation source mapping.");}
        {Probe p;run(U"tests/debugger/test_unicode.praat",p);}
        {
            auto tokens=ScriptSyntax_scan(L"name$ = \"a\"\"b\" ; comment");
            Melder_require(tokens.size()==3&&tokens[0].token==ScriptToken::StringVariable&&tokens[1].token==ScriptToken::String&&tokens[2].token==ScriptToken::Comment,U"Syntax lexer.");
        }
        Melder_casual(U"DEBUGGER INTEGRATION TESTS: PASS");return 0;
    }catch(MelderError){Melder_flushError(U"DEBUGGER INTEGRATION TESTS: FAIL");return 1;}
}
