// Line-local lexer for display only; execution remains in Interpreter/Formula.
// Praat Custom Script IDE. GPL-3.0-or-later.
#include "ScriptSyntax.h"
#include <cwctype>
#include <unordered_set>

std::vector<ScriptSyntaxSpan> ScriptSyntax_scan (const std::wstring &line) {
    static const std::unordered_set<std::wstring> control {
        L"if",L"then",L"elsif",L"elif",L"else",L"endif",L"for",L"from",L"to",L"endfor",
        L"while",L"endwhile",L"repeat",L"until",L"goto",L"label",L"exit",L"exitScript",
        L"assert",L"assertError",L"inc",L"dec",L"stopwatch",L"include",L"nocheck",L"nowarn",L"noprogress",
        L"and",L"or",L"not",L"div",L"mod"
    };
    static const std::unordered_set<std::wstring> form {
        L"form",L"endform",L"word",L"sentence",L"text",L"real",L"positive",L"integer",L"natural",
        L"boolean",L"choice",L"option",L"comment",L"optionmenu",L"button",L"heading",L"caption",
        L"infile",L"outfile",L"folder",L"realvector",L"positivevector",L"integervector",L"naturalvector",
        L"stringarray",L"colour"
    };
    std::vector<ScriptSyntaxSpan> spans;
    bool first=true;
    for(size_t i=0;i<line.size();) {
        if(iswspace(line[i])) {++i;continue;}
        size_t start=i;
        auto emit=[&](ScriptToken t){spans.push_back({start,i-start,t});first=false;};
        wchar_t c=line[i];
        if(c==L';'||(first&&(c==L'#'||c==L'!'))) {
            i=line.size();emit(ScriptToken::Comment);break;
        }
        if(c==L'"') {
            ++i;
            while(i<line.size()) {
                if(line[i++]==L'"') {if(i<line.size()&&line[i]==L'"') ++i;else break;}
            }
            emit(ScriptToken::String);continue;
        }
        if(iswdigit(c)||(c==L'.'&&i+1<line.size()&&iswdigit(line[i+1]))) {
            ++i;
            while(i<line.size()&&(iswdigit(line[i])||line[i]==L'.')) ++i;
            if(i<line.size()&&(line[i]==L'e'||line[i]==L'E')) {
                ++i;if(i<line.size()&&(line[i]==L'+'||line[i]==L'-')) ++i;
                while(i<line.size()&&iswdigit(line[i])) ++i;
            }
            if(i<line.size()&&line[i]==L'%') ++i;
            emit(ScriptToken::Number);continue;
        }
        if(c==L'@'||iswalpha(c)||c==L'_'||c==L'.') {
            ++i;
            while(i<line.size()&&(iswalnum(line[i])||line[i]==L'_'||line[i]==L'.'||line[i]==L'$'||line[i]==L'#')) ++i;
            std::wstring word=line.substr(start,i-start);
            size_t next=i;while(next<line.size()&&iswspace(line[next])) ++next;
            ScriptToken t=ScriptToken::Variable;
            if(control.count(word)) t=ScriptToken::Keyword;
            else if(form.count(word)) t=ScriptToken::Form;
            else if(c==L'@'||word==L"procedure"||word==L"proc"||word==L"endproc"||word==L"call") t=ScriptToken::Procedure;
            else if(first&&iswupper(c)) {
                while(i<line.size()&&line[i]!=L':'&&line[i]!=L'"'&&line[i]!=L';') ++i;
                t=ScriptToken::Command;
            }
            else if(next<line.size()&&line[next]==L'(') t=ScriptToken::Function;
            else if(word.back()==L'#') t=ScriptToken::Array;
            else if(word.back()==L'$') t=ScriptToken::StringVariable;
            emit(t);continue;
        }
        ++i; first=false;
    }
    return spans;
}
