#ifndef _WinScriptEditor_h_
#define _WinScriptEditor_h_
// Windows-only frontend. Non-Windows builds keep the original ScriptEditor.
struct structScriptEditor;
struct structInterpreter;
struct ScriptDebugger;
enum class ScriptIDEAction { Start, Stop, Over, Into, Out, Toggle, Clear, List };
void WinScriptEditor_create (structScriptEditor *editor);
void WinScriptEditor_destroy (structScriptEditor *editor);
void WinScriptEditor_action (structScriptEditor *editor, ScriptIDEAction action);
bool WinScriptEditor_active (structScriptEditor *editor);
ScriptDebugger *WinScriptEditor_debugger (structScriptEditor *editor);
void WinScriptEditor_finished (structScriptEditor *editor);
void WinScriptEditor_begin (structScriptEditor *editor);
void WinScriptEditor_reset (structScriptEditor *editor);
void ScriptEditor_runForDebugger (structScriptEditor *editor);
#endif
