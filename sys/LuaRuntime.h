// Praat Custom. GPL-3.0-or-later.
#ifndef _LuaRuntime_h_
#define _LuaRuntime_h_
#include "melder.h"
#include <functional>
// poll returns false to cancel; called between Lua instruction slices.
void Lua_run (conststring32 source, MelderFile file, bool checkOnly = false,
    std::function<bool()> poll = {});
#endif
