#ifndef PRAAT_LUA_DEBUG_RUN_H
#define PRAAT_LUA_DEBUG_RUN_H
#include "LuaRuntime.h"
#include "LuaDebugger.h"
void Lua_runDebug(conststring32 source,MelderFile file,std::function<bool()> poll,LuaDebugSession &debugger);
#endif
