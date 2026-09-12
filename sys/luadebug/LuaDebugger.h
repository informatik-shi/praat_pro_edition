// Praat Custom. GPL-3.0-or-later. The debugger never evaluates user expressions.
#ifndef PRAAT_LUA_DEBUGGER_H
#define PRAAT_LUA_DEBUGGER_H
#include <functional>
#include <set>
#include <string>
struct lua_State;
struct lua_Debug;
enum class LuaDebugState {Idle,Running,Paused,Finished,Stopped,Error};
enum class LuaDebugStep {Continue,Into,Over,Out};
struct LuaDebugSession {
    std::set<long> breakpoints;
    LuaDebugState state=LuaDebugState::Idle;
    LuaDebugStep step=LuaDebugStep::Continue;
    bool breakOnEntry=false, pauseNext=false;
    long line=0;
    int depth=0, originDepth=0;
    lua_State *thread=nullptr, *originThread=nullptr;
    std::string source, variables, callStack, error;
    std::function<void(LuaDebugSession&)> paused;
    void begin(const std::string &file);
    void resume(LuaDebugStep next);
    void stop(){state=LuaDebugState::Stopped;}
    bool onLine(lua_State *L,lua_Debug *event);
    void capture(lua_State *L,bool atError=false);
};
#endif
