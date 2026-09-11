// Praat Custom Script IDE. GPL-3.0-or-later.
#include "Interpreter.h"
#include "ScriptDebugger.h"
#include <algorithm>
#include <vector>
#include <unordered_set>

bool ScriptDebugger::active () const {
    return state == ScriptDebugState::Running || state == ScriptDebugState::Paused || state == ScriptDebugState::Stopping;
}
void ScriptDebugger::start () {
    error.clear(); current = nullptr; currentLine = 0;
    step = ScriptDebugStep::Continue; state = ScriptDebugState::Running;
}
void ScriptDebugger::resume (ScriptDebugStep mode) {
    if (state != ScriptDebugState::Paused) return;
    step = mode; stepDepth = current ? current->callDepth : 0;
    state = ScriptDebugState::Running;
}
void ScriptDebugger::stop () { if (active()) state = ScriptDebugState::Stopping; }
void ScriptDebugger::finish () {
    if (state != ScriptDebugState::Error) state = ScriptDebugState::Finished;
    changed();
}
void ScriptDebugger::beforeStatement (ScriptDebugInterpreter interpreter) {
    if (!active()) return;
    current = interpreter;
    poll();
    if (state == ScriptDebugState::Stopping) { Interpreter_stop (interpreter); return; }
    const char32 *text = interpreter->lines [interpreter->lineNumber];
    // Empty/comment lines are not execution stops. Structural instructions remain real stops.
    if (! *text || *text == U'#' || *text == U';' || *text == U'!') return;
    bool shouldPause = breakpoints.count (interpreter->lineNumber) != 0;
    shouldPause |= step == ScriptDebugStep::Into;
    shouldPause |= step == ScriptDebugStep::Over && interpreter->callDepth <= stepDepth;
    shouldPause |= step == ScriptDebugStep::Out && interpreter->callDepth < stepDepth;
    if (shouldPause) {
        currentLine = interpreter->lineNumber;
        state = ScriptDebugState::Paused;
        changed(); paused();
        if (state == ScriptDebugState::Stopping) Interpreter_stop (interpreter);
    }
}
void ScriptDebugger::onError (ScriptDebugInterpreter interpreter, const char32_t *message) {
    current = interpreter; currentLine = interpreter->lineNumber;
    error = message ? message : U"Runtime error";
    state = ScriptDebugState::Error;
    changed();
}

static std::u32string number (double value) { return Melder_double (value); }
static std::u32string shortString (const char32 *value) {
    std::u32string s;
    if(value) {size_t i=0;while(i<160&&value[i])s+=value[i++];if(value[i])s+=U"…";}
    for (auto &c : s) if (c == U'\n' || c == U'\r' || c == U'\t') c = U' ';
    return U"\"" + s + U"\"";
}
static std::u32string describe (const std::u32string &name, InterpreterVariable v) {
    if (name.size() >= 2 && name.substr(name.size()-2) == U"##") {
        std::u32string result = U"matrix\t" + number(v->numericMatrixValue.nrow) + U" x " + number(v->numericMatrixValue.ncol);
        if (v->numericMatrixValue.nrow > 0) {
            result += U"  row 1: [";
            for (integer i=1; i<=std::min<integer>(6,v->numericMatrixValue.ncol); ++i)
                result += (i>1 ? U", " : U"") + number(v->numericMatrixValue[1][i]);
            result += U"]";
        }
        return result;
    }
    if (name.size() >= 2 && name.substr(name.size()-2) == U"$#") {
        std::u32string result = U"string vector\tsize=" + number(v->stringArrayValue.size) + U" [";
        for(integer i=1;i<=std::min<integer>(4,v->stringArrayValue.size);++i)
            result += (i>1?U", ":U"") + shortString(v->stringArrayValue[i].get());
        return result + U"]";
    }
    if (!name.empty() && name.back()==U'#') {
        std::u32string result=U"vector\tsize="+number(v->numericVectorValue.size)+U" [";
        for(integer i=1;i<=std::min<integer>(8,v->numericVectorValue.size);++i)
            result+=(i>1?U", ":U"")+number(v->numericVectorValue[i]);
        return result+(v->numericVectorValue.size>8?U", …]":U"]");
    }
    if (!name.empty() && name.back()==U'$') return U"string\t"+shortString(v->stringValue.get());
    return U"number\t"+number(v->numericValue);
}
std::u32string ScriptDebugger_variables (Interpreter interpreter) {
    if (!interpreter) return U"Start debugging to inspect variables.";
    std::vector<std::u32string> names;
    for (auto &entry:interpreter->variablesMap) names.push_back(entry.first);
    std::sort(names.begin(),names.end());
    std::u32string result=U"Name\tScope\tType\tValue\n";
    std::u32string procedure=interpreter->procedureStackNames[interpreter->callDepth];
    for (auto &name:names) {
        auto dot=name.find(U'.');
        std::u32string scope=dot==std::u32string::npos?U"global":(name.substr(0,dot)==procedure?U"local":U"procedure");
        result+=name+U"\t"+scope+U"\t"+describe(name,interpreter->variablesMap.at(name).get())+U"\n";
    }
    return result;
}
std::u32string ScriptDebugger_stack (Interpreter interpreter) {
    if (!interpreter) return U"No active interpreter.";
    std::u32string result=U"Procedure\tSource line\n";
    for(int depth=0;depth<=interpreter->callDepth;++depth) {
        result+=(depth?std::u32string(interpreter->procedureStackNames[depth]):U"main");
        result+=U"\t"+number(depth==interpreter->callDepth?interpreter->lineNumber:interpreter->callStack[depth+1])+U"\n";
    }
    return result;
}
std::u32string ScriptDebugger_watch (Interpreter interpreter, const std::u32string &expression) {
    if (!interpreter) return U"<not running>";
    // Formula can execute commands, random generators and file operations. Fail closed.
    static const std::unordered_set<std::u32string> pure {
        U"abs",U"sqrt",U"sin",U"cos",U"tan",U"exp",U"ln",U"log10",U"log2",
        U"floor",U"ceiling",U"round",U"min",U"max",U"length",U"size",U"numberOfRows",
        U"numberOfColumns",U"left$",U"right$",U"mid$",U"string$",U"number",U"index",U"rindex"
    };
    static const std::unordered_set<std::u32string> words {
        U"pi",U"e",U"undefined",U"true",U"false",U"and",U"or",U"not",U"div",U"mod",
        U"if",U"then",U"else",U"endif"
    };
    if(expression.size()>2048) return U"<expression too long>";
    bool quoted=false;
    for(size_t i=0;i<expression.size();) {
        char32 c=expression[i];
        if(c==U'"') { quoted=!quoted; ++i; continue; }
        if(quoted) { ++i; continue; }
        if(c==U'@'||c==U':'||c==U'\''||c==U';'||c==U'\n'||c==U'\r') return U"<not a read-only expression>";
        if((c>=U'0'&&c<=U'9')||(c==U'.'&&i+1<expression.size()&&expression[i+1]>=U'0'&&expression[i+1]<=U'9')) {
            ++i;
            while(i<expression.size()&&((expression[i]>=U'0'&&expression[i]<=U'9')||expression[i]==U'.'))++i;
            if(i<expression.size()&&(expression[i]==U'e'||expression[i]==U'E')) {
                ++i;if(i<expression.size()&&(expression[i]==U'+'||expression[i]==U'-'))++i;
                while(i<expression.size()&&expression[i]>=U'0'&&expression[i]<=U'9')++i;
            }
            continue;
        }
        if(Melder_isLetter(c)||c==U'_'||c==U'.') {
            size_t start=i++;
            while(i<expression.size()&&(Melder_isLetter(expression[i])||(expression[i]>=U'0'&&expression[i]<=U'9')||expression[i]==U'_'||expression[i]==U'.'||expression[i]==U'$'||expression[i]==U'#')) ++i;
            auto token=expression.substr(start,i-start);
            size_t next=i; while(next<expression.size()&&Melder_isHorizontalSpace(expression[next])) ++next;
            if(next<expression.size()&&expression[next]==U'(') {
                if(!pure.count(token)) return U"<function not allowed in Watch>";
            } else if(!words.count(token)&&!Interpreter_hasVariable(interpreter,token.c_str()))
                return U"<unknown variable or symbol not allowed in Watch>";
        } else ++i;
    }
    if(quoted) return U"<unterminated string>";
    try {
        auto found=Interpreter_hasVariable(interpreter,expression.c_str());
        if(found) return describe(expression,found);
        Formula_Result result;
        Interpreter_anyExpression(interpreter,expression.c_str(),&result);
        switch(result.expressionType) {
            case kFormula_EXPRESSION_TYPE_NUMERIC: return number(result.numericResult);
            case kFormula_EXPRESSION_TYPE_STRING: return shortString(result.stringResult.get());
            default: return U"<array expression: watch the variable for a bounded preview>";
        }
    } catch(MelderError) {
        std::u32string result=Melder_getError(); Melder_clearError(); return result;
    }
}
