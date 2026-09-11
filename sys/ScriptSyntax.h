#ifndef _ScriptSyntax_h_
#define _ScriptSyntax_h_
// Praat Custom Script IDE. GPL-3.0-or-later.
#include <string>
#include <vector>
enum class ScriptToken { Text, Keyword, Form, Comment, String, Number, Variable, StringVariable, Array, Function, Procedure, Command };
struct ScriptSyntaxSpan { size_t start, length; ScriptToken token; };
std::vector<ScriptSyntaxSpan> ScriptSyntax_scan (const std::wstring &line);
#endif
