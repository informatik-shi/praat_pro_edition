// Praat Custom. GPL-3.0-or-later. Lua itself retains its MIT license.
#include "LuaRuntime.h"
#include "luaplot/LuaPlot.h"
#include "luadebug/LuaDebugRun.h"
#include "praatP.h"
#include "Interpreter.h"
#include "Formula.h"
#include "../external/lua-5.5.1/src/lua.h"
#include "../external/lua-5.5.1/src/lauxlib.h"
#include "../external/lua-5.5.1/src/lualib.h"
#include <memory>
#include <string>
#include <vector>

namespace {
bool running = false;
struct Context {
    Interpreter interpreter;
    lua_State *mainThread=nullptr;
    std::function<bool()> *poll=nullptr;
    LuaDebugSession *debugger=nullptr;
};
Context &context (lua_State *L) {
    return **static_cast<Context **>(lua_getextraspace (L));
}
void pushText (lua_State *L, conststring32 text) {
    lua_pushstring (L, Melder_peek32to8 (text ? text : U""));
}
autostring32 stringArgument (lua_State *L, int index) {
    size_t size;
    const char *value = luaL_checklstring (L, index, &size);
    if (strlen (value) != size) luaL_error (L, "Praat strings cannot contain NUL bytes");
    return Melder_8to32_e (value);
}
int bridgeError (lua_State *L) {
    pushText (L, Melder_getError());
    Melder_clearError();
    return lua_error (L);
}
int selected (lua_State *L) {
    lua_newtable (L); int n = 0;
    for (integer i = 1; i <= theCurrentPraatObjects->n; ++ i)
        if (theCurrentPraatObjects->list[i].isSelected) {
            lua_pushinteger (L, theCurrentPraatObjects->list[i].id);
            lua_rawseti (L, -2, ++ n);
        }
    return 1;
}
int objects (lua_State *L) {
    lua_newtable (L);
    for (integer i = 1; i <= theCurrentPraatObjects->n; ++ i) {
        auto &object = theCurrentPraatObjects->list[i];
        lua_newtable (L);
        lua_pushinteger (L, object.id); lua_setfield (L, -2, "id");
        pushText (L, object.name.get()); lua_setfield (L, -2, "name");
        pushText (L, object.klas->className); lua_setfield (L, -2, "class");
        lua_rawseti (L, -2, i);
    }
    return 1;
}
int selectObjects (lua_State *L) {
    try {
        std::vector<integer> indices;
        const bool table = lua_istable (L, 1);
        int count = table ? (int) lua_rawlen (L, 1) : lua_gettop (L);
        for (int k = 1; k <= count; ++ k) {
            if (table) lua_rawgeti (L, 1, k);
            lua_Integer id = luaL_checkinteger (L, table ? -1 : k);
            integer index = 0;
            for (integer i = 1; i <= theCurrentPraatObjects->n; ++ i)
                if (theCurrentPraatObjects->list[i].id == id) { index = i; break; }
            if (! index) return luaL_error (L, "No Praat object with id %I", id);
            indices.push_back (index);
            if (table) lua_pop (L, 1);
        }
        praat_deselectAll();
        for (integer i : indices) praat_select (i);
        praat_updateSelection();
        praat_show();
        return 0;
    } catch (MelderError) { return bridgeError (L); }
}
int call (lua_State *L) {
    try {
        auto title32 = stringArgument (L, 1);
        std::u32string title (title32.get());
        int count = lua_gettop (L) - 1;
        if (! title.empty() && title.back() == U':') { title.pop_back(); title += U"..."; }
        if (count && (title.size() < 3 || title.substr (title.size()-3) != U"...")) title += U"...";
        if (title == U"Quit" || title == Melder_cat(U"Quit ",Melder_upperCaseAppName()) || title == U"Run Lua file...")
            return luaL_error (L, "This command cannot be called from Lua");
        auto args = std::make_unique<structStackel[]> (count + 1);
        for (int i = 1; i <= count; ++ i) {
            switch (lua_type (L, i + 1)) {
                case LUA_TNUMBER: args[i].number = lua_tonumber (L, i+1); break;
                case LUA_TBOOLEAN: args[i].number = lua_toboolean (L, i+1); break;
                case LUA_TSTRING: args[i].setString (stringArgument (L, i+1)); break;
                default: return luaL_error (L, "Command arguments must be numbers, booleans or strings");
            }
        }
        Interpreter interpreter = context (L).interpreter;
        interpreter->returnType = kInterpreter_ReturnType::VOID_;
        autoMelderString info;
        {
            autoMelderDivertInfo capture (&info);
            if (! praat_doAction (title.c_str(), count, args.get(), interpreter) &&
                ! praat_doMenuCommand (title.c_str(), count, args.get(), interpreter))
                Melder_throw (U"Command not available for the current selection: ", title.c_str());
        }
        praat_updateSelection();
        switch (interpreter->returnType) {
            case kInterpreter_ReturnType::REAL_:
            case kInterpreter_ReturnType::INTEGER_:
                lua_pushnumber (L, Melder_atof (info.string)); return 1;
            case kInterpreter_ReturnType::BOOLEAN_:
                lua_pushboolean (L, Melder_atof (info.string) != 0); return 1;
            case kInterpreter_ReturnType::OBJECT_:
                return selected (L);
            case kInterpreter_ReturnType::REALVECTOR_:
                lua_newtable (L);
                for (integer i=1; i<=interpreter->returnedRealVector.size; ++i) {
                    lua_pushnumber (L, interpreter->returnedRealVector[i]); lua_rawseti (L,-2,i);
                } return 1;
            case kInterpreter_ReturnType::INTEGERVECTOR_:
                lua_newtable (L);
                for (integer i=1; i<=interpreter->returnedIntegerVector.size; ++i) {
                    lua_pushinteger (L, interpreter->returnedIntegerVector[i]); lua_rawseti (L,-2,i);
                } return 1;
            case kInterpreter_ReturnType::REALMATRIX_:
                lua_newtable (L);
                for (integer i=1; i<=interpreter->returnedRealMatrix.nrow; ++i) {
                    lua_newtable (L);
                    for (integer j=1; j<=interpreter->returnedRealMatrix.ncol; ++j) {
                        lua_pushnumber (L,interpreter->returnedRealMatrix[i][j]); lua_rawseti (L,-2,j);
                    } lua_rawseti (L,-2,i);
                } return 1;
            case kInterpreter_ReturnType::STRINGARRAY_:
                lua_newtable (L);
                for (integer i=1; i<=interpreter->returnedStringArray.size; ++i) {
                    pushText (L,interpreter->returnedStringArray[i].get()); lua_rawseti (L,-2,i);
                } return 1;
            case kInterpreter_ReturnType::STRING_:
                pushText (L, info.string); return 1;
            default: return 0;
        }
    } catch (MelderError) { return bridgeError (L); }
}
int print (lua_State *L) {
    try {
        autoMelderString output;
        for (int i=1, n=lua_gettop(L); i<=n; ++i) {
            size_t length;
            const char *value = luaL_tolstring (L,i,&length);
            auto value32 = Melder_8to32_e (value);
            MelderString_append (&output, i>1 ? U"\t" : U"", value32.get());
            lua_pop (L,1);
        }
        MelderInfo_writeLine (output.string); MelderInfo_drain();
        return 0;
    } catch (MelderError) { return bridgeError (L); }
}
int noExit (lua_State *L) { return luaL_error (L, "os.exit is disabled in the Praat host"); }
int noNativeModules (lua_State *L) { return luaL_error (L, "Native Lua modules are not supported in this static C++ host"); }
int noHookReplacement (lua_State *L) { return luaL_error (L, "The execution hook is reserved for the Praat Stop command"); }
void yieldHook (lua_State *L, lua_Debug *event) {
    auto &host=context(L);
    if(host.debugger&&event->event==LUA_HOOKLINE) {
        if(!host.debugger->onLine(L,event))luaL_error(L,"Lua execution stopped");
        return;
    }
    if (L==host.mainThread && lua_isyieldable (L)) lua_yield (L,0);
    else if(host.poll && *host.poll && !(*host.poll)()) luaL_error(L,"Lua execution stopped");
}
}

static void Lua_runImpl (conststring32 source, MelderFile file, bool checkOnly, std::function<bool()> poll,LuaDebugSession *debugger) {
    if (running) Melder_throw (U"A Lua script is already running.");
    struct RunningGuard { RunningGuard(){running=true;} ~RunningGuard(){running=false;} } guard;
    autoInterpreterStack stack = InterpreterStack_create (nullptr);
    autoInterpreter interpreter = Interpreter_createFromEnvironment (stack.get(), nullptr, file);
    structMelderFile unnamed {};
    if (!file || MelderFile_isNull(file)) { Melder_relativePathToFile (U"untitled.lua", &unnamed); file=&unnamed; }
    Interpreter_rememberScript (interpreter.get(), file, true);
    autoMelderFileSetCurrentFolder folder (file);
    std::unique_ptr<lua_State, decltype(&lua_close)> state (luaL_newstate(), lua_close);
    if (!state) Melder_throw (U"Cannot allocate Lua state.");
    lua_State *L = state.get();
    Context contextData {interpreter.get()};
    *static_cast<Context **>(lua_getextraspace(L)) = &contextData;
    luaL_openlibs (L);
    LuaPlot_install(L);
    lua_getglobal(L,"debug");lua_pushcfunction(L,noHookReplacement);lua_setfield(L,-2,"sethook");lua_pop(L,1);
    lua_pushcfunction (L, print); lua_setglobal (L,"print");
    lua_getglobal (L,"os"); lua_pushcfunction(L,noExit); lua_setfield(L,-2,"exit"); lua_pop(L,1);
    lua_getglobal (L,"package");
    lua_pushcfunction(L,noNativeModules); lua_setfield(L,-2,"loadlib");
    lua_getfield(L,-1,"searchers");
    lua_pushnil(L);lua_rawseti(L,-2,3);lua_pushnil(L);lua_rawseti(L,-2,4);lua_pop(L,2);
    lua_newtable (L);
    const luaL_Reg api[] = {{"call",call},{"select",selectObjects},{"selected",selected},{"objects",objects},{nullptr,nullptr}};
    luaL_setfuncs(L,api,0); lua_setglobal(L,"praat");
    std::string text (Melder_peek32to8(source));
    std::string name = "@"; name += Melder_peek32to8(MelderFile_peekPath(file));
    if(debugger)debugger->begin(name);
    lua_State *thread = lua_newthread (L);
    contextData.mainThread=thread;contextData.poll=&poll;
    contextData.debugger=debugger;
    if (luaL_loadbufferx(thread,text.data(),text.size(),name.c_str(),"t") != LUA_OK) {
        auto error=Melder_8to32_e(lua_tostring(thread,-1));
        Melder_throw (error.get());
    }
    if (checkOnly) return;
    lua_sethook (thread,yieldHook,LUA_MASKCOUNT|(debugger?LUA_MASKLINE:0),10000);
    MelderInfo_open();
    int status, results=0;
    do {
        if (poll && !poll()) Melder_throw (U"Lua execution stopped.");
        status=lua_resume(thread,nullptr,0,&results);
        if (status==LUA_YIELD) lua_pop(thread,results);
    } while (status==LUA_YIELD);
    MelderInfo_close();
    if (status != LUA_OK) {
        if(debugger)debugger->capture(thread,true);
        const char *message=lua_tostring(thread,-1);
        luaL_traceback(L,thread,message ? message : "Lua error (non-string value)",1);
        auto error=Melder_8to32_e(lua_tostring(L,-1));
        Melder_throw (error.get());
    }
}
void Lua_run (conststring32 source,MelderFile file,bool checkOnly,std::function<bool()> poll) {
    Lua_runImpl(source,file,checkOnly,std::move(poll),nullptr);
}
void Lua_runDebug(conststring32 source,MelderFile file,std::function<bool()> poll,LuaDebugSession &debugger) {
    try {
        Lua_runImpl(source,file,false,std::move(poll),&debugger);
        debugger.state=LuaDebugState::Finished;debugger.thread=debugger.originThread=nullptr;
    } catch(MelderError) {
        if(debugger.state!=LuaDebugState::Stopped)debugger.state=LuaDebugState::Error;
        debugger.error=Melder_peek32to8(Melder_getError());debugger.thread=debugger.originThread=nullptr;
        throw;
    }
}
