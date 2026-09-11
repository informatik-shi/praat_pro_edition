# Architecture analysis: Praat Script IDE

Baseline: upstream 6f3da9ef1d8cce0d5684afc104d02888dfc71b25, Windows x64v1,
Clang 22.1.8 / MSYS2. Built and smoke-tested before changes; both upstream
batch suites passed (135 and 78 scripts). See project-docs/BASELINE_WINDOWS.md.

## Editor and Windows GUI

`sys/ScriptEditor.{h,cpp}` extends `TextEditor`; it adds Run, argument forms,
include expansion and an owned InterpreterStack. `TextEditor.cpp` owns file
operations, dirty state, clipboard, undo, Find/Replace and Go to line.
`GuiText.cpp` wraps the native Windows EDIT control; `motifEmulator.cpp`
dispatches Windows messages and menu accelerators. EDIT has no per-range
colour support. Use the Windows system RichEdit only for ScriptEditor,
behind a GuiText flag, preserving the shared text API and Unicode offsets.
Other text widgets retain their existing implementation.

## Execution

`Interpreter.cpp`: private_Interpreter_initialize splits physical lines,
retains slots for continuation lines, indexes labels/procedures and creates
variables. `Interpreter_resume` executes a loop over lineNumber. Its switch
handles if/elsif/else/endif, for/endfor, while/endwhile, repeat/until and goto.
Procedure calls (`@name:` and legacy `call`) update callDepth,
procedureStackNames and callStack; endproc restores the caller's line.
`Formula.cpp` compiles and evaluates expressions. Commands are delegated to
`praat_executeCommand` in praat_script.cpp.

Variables are actual InterpreterVariable instances in variablesMap: number,
string ($), numeric vector (#), matrix (##), string vector ($#). Dot-prefixed
locals are qualified by procedure name, not independent lexical activation
records: recursive calls share that procedure's local namespace.

Runtime errors propagate as MelderError. The outer catch in
Interpreter_resume adds the failed line before propagating. assertError
is handled internally and should not trigger an error stop.

## Integration plan

Add a nullable GUI-independent observer to Interpreter. Call before a real
nonempty instruction (before interpolation/evaluation), and on an unhandled
error. With no observer normal execution remains unchanged. Use actual
procedure depth for Step Over/Out and actual stack entries for Call Stack.

Windows controller owns breakpoints, state and inspection UI. Pausing runs
a bounded/restricted Windows event loop on the same thread, keeping the
interpreter stack intact. Disable source edits and unrelated editor actions
during execution; poll between statements so Stop works in script loops.
Never evaluate arbitrary side-effecting Formula functions from Watch: use
an explicit pure-function allowlist before the existing expression engine.

Keep changes localized: ScriptEditor, GuiText opt-in, small interpreter hook,
separate ScriptDebugger / ScriptSyntax / Windows controller modules, existing
sys Makefile, tests/debugger and Windows build/release scripts. No replacement
of Praat's object model, DSP, normal command dispatch or build system.
