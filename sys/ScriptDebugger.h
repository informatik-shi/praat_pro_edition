#ifndef _ScriptDebugger_h_
#define _ScriptDebugger_h_
// Praat Custom Script IDE. GPL-3.0-or-later.
#include <set>
#include <string>
struct structInterpreter;
using ScriptDebugInterpreter = structInterpreter *;

enum class ScriptDebugState { Idle, Running, Paused, Stopping, Finished, Error };
enum class ScriptDebugStep { Continue, Into, Over, Out };

// Optional observer. Interpreter never depends on a concrete editor or event loop.
struct ScriptDebugger {
    ScriptDebugState state = ScriptDebugState::Idle;
    ScriptDebugStep step = ScriptDebugStep::Continue;
    std::set<long long> breakpoints;
    int stepDepth = 0;
    long long currentLine = 0;
    ScriptDebugInterpreter current = nullptr;
    std::u32string error;
    virtual ~ScriptDebugger () = default;
    void start ();
    void resume (ScriptDebugStep mode);
    void stop ();
    void finish ();
    void beforeStatement (ScriptDebugInterpreter interpreter);
    void onError (ScriptDebugInterpreter interpreter, const char32_t *message);
    virtual void poll () {} // cooperative cancellation between statements
    virtual void paused () {} // frontend must resume or request stop before returning
    virtual void changed () {}
    bool active () const;
};

std::u32string ScriptDebugger_variables (ScriptDebugInterpreter interpreter);
std::u32string ScriptDebugger_stack (ScriptDebugInterpreter interpreter);
std::u32string ScriptDebugger_watch (ScriptDebugInterpreter interpreter, const std::u32string &expression);
#endif
