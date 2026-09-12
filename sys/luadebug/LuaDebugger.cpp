// Praat Custom. GPL-3.0-or-later.
#include "LuaDebugger.h"
#include "../../external/lua-5.5.1/src/lua.h"
#include <sstream>
#include <iomanip>
#include <cstring>
namespace {
struct StackGuard {lua_State *L;int top;explicit StackGuard(lua_State *s):L(s),top(lua_gettop(s)){}~StackGuard(){lua_settop(L,top);}};
std::string quote(const char *s,size_t length) {
    std::string out="\"";size_t end=std::min(length,size_t(160));
    if(end<length)while(end&&((unsigned char)s[end]&0xc0)==0x80)--end;
    for(size_t i=0;i<end;++i){unsigned char c=s[i];if(c=='\n')out+="\\n";else if(c=='\r')out+="\\r";else if(c=='\t')out+="\\t";else if(c=='\"'||c=='\\'){out+='\\';out+=c;}else if(c<32||c==127){const char *hex="0123456789abcdef";out+="\\x";out+=hex[c>>4];out+=hex[c&15];}else out+=c;}
    if(end<length)out+="...";return out+'"';
}
std::string value(lua_State *L,int index,int depth=0) {
    StackGuard guard(L);index=lua_absindex(L,index);
    switch(lua_type(L,index)) {
        case LUA_TNIL:return "nil";
        case LUA_TBOOLEAN:return lua_toboolean(L,index)?"true":"false";
        case LUA_TNUMBER:{if(lua_isinteger(L,index))return std::to_string(lua_tointeger(L,index));std::ostringstream out;out<<std::setprecision(15)<<lua_tonumber(L,index);return out.str();}
        case LUA_TSTRING:{size_t n;const char *s=lua_tolstring(L,index,&n);return quote(s,n);}
        case LUA_TTABLE:{if(depth)return "<table>";std::string out="{";int count=0;lua_pushnil(L);while(lua_next(L,index)){if(count==8){out+=", ...";break;}if(count++)out+=", ";out+='['+value(L,-2,1)+"]="+value(L,-1,1);lua_pop(L,1);}return out+'}';}
        default:return std::string("<")+lua_typename(L,lua_type(L,index))+">";
    }
}
int stackDepth(lua_State *L){lua_Debug frame;int n=0;while(n<10000&&lua_getstack(L,n,&frame))++n;return n;}
}
void LuaDebugSession::begin(const std::string &file) {
    source=file;state=LuaDebugState::Running;step=breakOnEntry?LuaDebugStep::Into:LuaDebugStep::Continue;
    pauseNext=false;line=0;depth=originDepth=0;thread=originThread=nullptr;
    variables.clear();callStack.clear();error.clear();
}
void LuaDebugSession::resume(LuaDebugStep next) {
    if(state!=LuaDebugState::Paused)return;
    step=next;originDepth=depth;originThread=thread;state=LuaDebugState::Running;
}
bool LuaDebugSession::onLine(lua_State *L,lua_Debug *event) {
    if(state==LuaDebugState::Stopped)return false;
    lua_getinfo(L,"S",event);
    // The current buffer owns breakpoints. Other source files are atomic for stepping.
    if(!event->source||source!=event->source||event->currentline<1)return true;
    bool breakpoint=breakpoints.count(event->currentline)!=0;
    if(!breakpoint&&!pauseNext&&step==LuaDebugStep::Continue)return true;
    int currentDepth=stackDepth(L);
    bool stepping=step==LuaDebugStep::Into ||
        (L==originThread && ((step==LuaDebugStep::Over&&currentDepth<=originDepth)||(step==LuaDebugStep::Out&&currentDepth<originDepth)));
    if(!breakpoint&&!pauseNext&&!stepping)return true;
    line=event->currentline;depth=currentDepth;thread=L;pauseNext=false;state=LuaDebugState::Paused;
    capture(L);
    if(paused)paused(*this);else resume(LuaDebugStep::Continue);
    return state!=LuaDebugState::Stopped;
}
void LuaDebugSession::capture(lua_State *L,bool atError) {
    StackGuard guard(L);variables.clear();callStack.clear();
    if(atError)line=0;
    lua_Debug frame;int inspect=-1;
    for(int level=0;level<64&&lua_getstack(L,level,&frame);++level) {
        lua_getinfo(L,"nSl",&frame);
        callStack+="#"+std::to_string(level)+"  "+(frame.name?frame.name:frame.what?frame.what:"?")+"  "+(frame.source?frame.source:"")+":"+std::to_string(frame.currentline)+"\n";
        if(inspect<0&&frame.what&&std::strcmp(frame.what,"C")!=0) {
            inspect=level;
            if(atError&&frame.source&&source==frame.source)line=frame.currentline;
        }
    }
    variables="Scope\tName\tValue\n";
    if(inspect>=0&&lua_getstack(L,inspect,&frame)) {
        for(int n=1;n<=128;++n) {
            const char *name=lua_getlocal(L,&frame,n);if(!name)break;
            if(name[0]!='(')variables+=std::string("local\t")+name+'\t'+value(L,-1)+'\n';lua_pop(L,1);
        }
        lua_getinfo(L,"f",&frame);int function=lua_gettop(L);
        for(int n=1;n<=64;++n) {
            const char *name=lua_getupvalue(L,function,n);if(!name)break;
            if(std::strcmp(name,"_ENV")!=0)variables+=std::string("upvalue\t")+name+'\t'+value(L,-1)+'\n';lua_pop(L,1);
        }
        lua_pop(L,1);
    }
    lua_pushglobaltable(L);int globals=lua_gettop(L);lua_pushnil(L);int count=0;
    while(lua_next(L,globals)) {
        if(lua_type(L,-2)==LUA_TSTRING&&lua_type(L,-1)!=LUA_TFUNCTION) {
            const char *name=lua_tostring(L,-2);
            // Built-in library tables obscure user variables; locals and upvalues above remain complete.
            if(std::strcmp(name,"_G")&&std::strcmp(name,"package")&&std::strcmp(name,"debug")&&std::strcmp(name,"io")&&std::strcmp(name,"os")&&std::strcmp(name,"math")&&std::strcmp(name,"string")&&std::strcmp(name,"table")&&std::strcmp(name,"utf8")&&std::strcmp(name,"coroutine")&&std::strcmp(name,"praat")) {
                variables+=std::string("global\t")+name+'\t'+value(L,-1)+'\n';
                if(++count>=64){variables+="... globals truncated\n";break;}
            }
        }lua_pop(L,1);
    }
}
