// Pure, non-executing language assistance. GPL-3.0-or-later.
#ifndef PRAAT_ASSIST_MODEL_H
#define PRAAT_ASSIST_MODEL_H
#include <algorithm>
#include <cwctype>
#include <set>
#include <string>
#include <vector>
namespace CodeAssist {
struct Problem { size_t line; std::wstring message; };
struct Entry { std::wstring name, help; };
struct Context { size_t start=0, end=0; bool commands=false, suppressed=false; std::wstring prefix; };
inline bool identifier(wchar_t c) { return iswalnum(c)||c==L'_'||c==L'.'||c==L'$'||c==L'#'; }
inline std::wstring trim(std::wstring s) {
    auto a=s.find_first_not_of(L" \t\r\n");
    return a==s.npos?L"":s.substr(a,s.find_last_not_of(L" \t\r\n")-a+1);
}
// Mask comments/strings while preserving offsets and physical line endings.
inline std::wstring mask(const std::wstring &s,bool lua) {
    auto out=s; bool leading=true;
    auto blank=[&](size_t a,size_t b){for(size_t j=a;j<b;++j)if(out[j]!=L'\n'&&out[j]!=L'\r')out[j]=L' ';};
    for(size_t i=0;i<s.size();) {
        if(s[i]==L'\n'||s[i]==L'\r'){leading=true;++i;continue;}
        if(iswspace(s[i])){++i;continue;}
        size_t a=i; bool comment=lua?s.compare(i,2,L"--")==0:(s[i]==L';'||(leading&&(s[i]==L'#'||s[i]==L'!')));
        leading=false;
        if(lua) {
            size_t p=i+(comment?2:0),q=p+1;
            if(p<s.size()&&s[p]==L'[') {
                while(q<s.size()&&s[q]==L'=')++q;
                if(q<s.size()&&s[q]==L'[') {
                    auto close=L"]"+std::wstring(q-p-1,L'=')+L"]";
                    auto end=s.find(close,q+1);i=end==s.npos?s.size():end+close.size();blank(a,i);continue;
                }
            }
        }
        if(comment){while(i<s.size()&&s[i]!=L'\n'&&s[i]!=L'\r')++i;blank(a,i);continue;}
        if(s[i]==L'"'||(lua&&s[i]==L'\'')) {
            wchar_t quote=s[i++];
            while(i<s.size()) {
                if(!lua&&(s[i]==L'\r'||s[i]==L'\n'))break;
                if(lua&&s[i]==L'\\'){i=std::min(i+2,s.size());continue;}
                if(s[i++]==quote){if(!lua&&i<s.size()&&s[i]==quote){++i;continue;}break;}
            }
            blank(a,i);continue;
        }
        ++i;
    }
    return out;
}
inline Context context(const std::wstring &s,size_t caret,bool lua) {
    Context c;c.start=c.end=std::min(caret,s.size());
    auto line=c.start?s.find_last_of(L"\r\n",c.start-1):s.npos;size_t begin=line==s.npos?0:line+1;
    if(lua) {
        auto before=s.substr(begin,c.start-begin);
        auto call=before.rfind(L"praat.call");
        if(call!=before.npos) {
            size_t p=call+10;while(p<before.size()&&iswspace(before[p]))++p;
            if(p<before.size()&&before[p++]==L'(') {
                while(p<before.size()&&iswspace(before[p]))++p;
                if(p<before.size()&&(before[p]==L'"'||before[p]==L'\'')) {
                    wchar_t quote=before[p++];
                    if(before.find(quote,p)==before.npos) {
                        // A call inside a comment/string must not trigger command completion.
                        auto masked=mask(s,true);
                        if(masked.compare(begin+call,10,L"praat.call")==0){c.start=begin+p;c.commands=true;}
                    }
                }
            }
        }
    } else {
        size_t p=begin;while(p<c.start&&iswspace(s[p]))++p;
        if(p<c.start&&iswupper(s[p])&&s.substr(p,c.start-p).find_first_of(L":=\"")==s.npos){c.start=p;c.commands=true;}
    }
    if(!c.commands) {
        while(c.start>0&&(identifier(s[c.start-1])||(lua&&s[c.start-1]==L':')))--c.start;
        auto masked=mask(s,lua);
        if(c.end>c.start&&masked.substr(c.start,c.end-c.start)!=s.substr(c.start,c.end-c.start))c.suppressed=true;
        if(c.end==c.start&&mask(s.substr(0,c.end)+L"X",lua).back()!=L'X')c.suppressed=true;
    }
    c.prefix=s.substr(c.start,c.end-c.start);return c;
}
inline std::vector<Entry> complete(const std::wstring &s,const Context &c,bool lua,const std::vector<Entry>& commands) {
    if(c.suppressed)return {};
    std::vector<Entry> all;
    if(c.commands)all=commands;
    else {
        const wchar_t *luaWords[]={L"local",L"function",L"end",L"if",L"then",L"else",L"elseif",L"for",L"while",L"do",L"repeat",L"until",L"return",L"true",L"false",L"nil",L"and",L"or",L"not",L"in",L"break",L"pairs",L"ipairs",L"type",L"tostring",L"tonumber",L"require",L"print",L"assert",L"error",L"pcall",L"math.sin",L"math.cos",L"math.sqrt",L"math.log",L"math.abs",L"math.pi",L"math.floor",L"math.max",L"math.min",L"string.format",L"string.sub",L"string.match",L"string.gsub",L"table.insert",L"table.remove",L"table.concat",L"table.sort",L"lfs.dir",L"lfs.attributes",L"lfs.currentdir"};
        const wchar_t *praatWords[]={L"if",L"elsif",L"else",L"endif",L"for",L"from",L"to",L"endfor",L"while",L"endwhile",L"repeat",L"until",L"procedure",L"endproc",L"form",L"endform",L"include",L"assert",L"writeInfoLine",L"appendInfoLine",L"selectObject",L"removeObject",L"selected",L"selected$",L"numberOfSelected",L"sin",L"cos",L"sqrt",L"abs",L"length",L"length$",L"fileReadable",L"readFile$",L"exitScript"};
        if(lua)for(auto w:luaWords)all.push_back({w,L"Lua"});
        else for(auto w:praatWords)all.push_back({w,L"Praat"});
        if(lua) {
            all.push_back({L"praat.call",L"praat.call(command, ...): run a Praat command; required objects must be selected"});
            all.push_back({L"praat.select",L"praat.select(ids): select object ID(s)"});
            all.push_back({L"praat.objects",L"praat.objects(): list objects"});
            all.push_back({L"praat.selected",L"praat.selected(): selected object IDs"});
            const Entry signatures[]={
                {L"math.sin",L"math.sin(x): x in radians"},{L"math.cos",L"math.cos(x): x in radians"},
                {L"math.sqrt",L"math.sqrt(x)"},{L"math.log",L"math.log(x [, base])"},
                {L"string.format",L"string.format(format, ...)"},{L"string.sub",L"string.sub(s, first [, last])"},
                {L"string.match",L"string.match(s, pattern [, init])"},{L"string.gsub",L"string.gsub(s, pattern, replacement [, limit])"},
                {L"table.insert",L"table.insert(list, [position,] value)"},{L"table.remove",L"table.remove(list [, position])"},
                {L"table.concat",L"table.concat(list [, separator [, first [, last]]])"},
                {L"lfs.dir",L"lfs.dir(path): iterator over directory entries"},{L"lfs.attributes",L"lfs.attributes(path [, attribute])"},
                {L"require",L"require(moduleName)"},{L"print",L"print(...)"}
            };
            // Prefer descriptive signatures to generic library labels.
            all.insert(all.begin(),std::begin(signatures),std::end(signatures));
        }
        auto code=mask(s,lua);
        for(size_t i=0;i<code.size();) {
            if(!iswalpha(code[i])&&code[i]!=L'_'){++i;continue;}
            size_t a=i++;while(i<code.size()&&(identifier(code[i])||(lua&&code[i]==L':')) )++i;
            auto name=code.substr(a,i-a);
            all.push_back({name,L"Identifier in this document (scope/type not inferred)"});
            if((lua&&name==L"function")||(!lua&&(name==L"procedure"||name==L"proc"))) {
                size_t p=i;while(p<code.size()&&(code[p]==L' '||code[p]==L'\t'))++p;
                size_t start=p;while(p<code.size()&&(identifier(code[p])||(lua&&code[p]==L':')))++p;
                auto functionName=code.substr(start,p-start);
                if(!lua&&!functionName.empty()&&functionName.back()==L':')functionName.pop_back();
                auto finish=code.find_first_of(lua?L")\r\n":L"\r\n",p);
                if(!functionName.empty()&&finish!=code.npos) {
                    if(lua&&code[finish]==L')')++finish;
                    all.insert(all.begin(),{functionName,trim(s.substr(start,finish-start))+L" | defined in this document"});
                }
            }
        }
    }
    std::stable_sort(all.begin(),all.end(),[](const Entry&a,const Entry&b){return a.name<b.name;});
    all.erase(std::unique(all.begin(),all.end(),[](const Entry&a,const Entry&b){return a.name==b.name;}),all.end());
    all.erase(std::remove_if(all.begin(),all.end(),[&](const Entry&e){return e.name.compare(0,c.prefix.size(),c.prefix)!=0;}),all.end());
    if(all.size()>200)all.resize(200);return all;
}
inline std::vector<Problem> checkPraat(const std::wstring &s) {
    std::vector<Problem> errors;struct Open{std::wstring close;size_t line;bool hadElse=false;};std::vector<Open> stack;
    auto code=mask(s,false);size_t line=1;
    // Include expansion can supply block delimiters; avoid definite errors in that case.
    bool includes=false;
    for(size_t a=0;a<=code.size();++line) {
        auto b=code.find_first_of(L"\r\n",a);if(b==code.npos)b=code.size();
        auto text=trim(code.substr(a,b-a));auto end=text.find_first_of(L" \t");auto w=text.substr(0,end);
        if(w==L"include")includes=true;
        std::wstring close;
        if(w==L"if")close=L"endif";else if(w==L"for")close=L"endfor";else if(w==L"while")close=L"endwhile";
        else if(w==L"repeat")close=L"until";else if(w==L"procedure"||w==L"proc")close=L"endproc";else if(w==L"form")close=L"endform";
        if(!close.empty())stack.push_back({close,line});
        else if(w==L"endif"||w==L"endfor"||w==L"endwhile"||w==L"until"||w==L"endproc"||w==L"endform") {
            if(stack.empty()||stack.back().close!=w)errors.push_back({line,L"Unexpected "+w});else stack.pop_back();
        } else if(w==L"else"||w==L"elsif"||w==L"elif") {
            if(stack.empty()||stack.back().close!=L"endif"||stack.back().hadElse)errors.push_back({line,L"Unexpected "+w});
            else if(w==L"else")stack.back().hadElse=true;
        }
        if(b==code.size())break;a=b+1;if(code[b]==L'\r'&&a<code.size()&&code[a]==L'\n')++a;
    }
    for(auto &o:stack)errors.push_back({o.line,L"Missing "+o.close});
    if(includes)errors.clear();
    std::stable_sort(errors.begin(),errors.end(),[](const Problem&a,const Problem&b){return a.line<b.line;});return errors;
}
}
#endif
