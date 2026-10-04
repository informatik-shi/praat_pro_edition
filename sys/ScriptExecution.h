// Praat Pro Edition. GPL-3.0-or-later.
#ifndef _ScriptExecution_h_
#define _ScriptExecution_h_

// A cooperative Stop control for scripts executed on the GUI thread.
// Praat object commands remain on that thread; cancellation is checked
// between script statements or Lua instruction slices.
class ScriptExecutionScope {
    bool enabled_ = false;
public:
    explicit ScriptExecutionScope (bool enable);
    ~ScriptExecutionScope ();
    ScriptExecutionScope (const ScriptExecutionScope &) = delete;
    ScriptExecutionScope &operator= (const ScriptExecutionScope &) = delete;
    bool poll ();
};

#endif
