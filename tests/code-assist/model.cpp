#include "../../sys/codefold/AssistModel.h"
#include <cassert>
#include <iostream>
using namespace CodeAssist;
int main() {
    std::vector<Entry> commands={{L"To Spectrogram",L"Sound"},{L"Read seg files",L"Pro"}};
    auto check=[&](std::wstring s,bool lua,std::wstring expected,bool command=false) {
        auto c=context(s,s.size(),lua);assert(c.commands==command);
        auto results=complete(s,c,lua,commands);
        assert(std::any_of(results.begin(),results.end(),[&](const Entry&e){return e.name==expected;}));
    };
    check(L"praat.ca",true,L"praat.call");
    check(L"math.s",true,L"math.sin");
    check(L"local measurement = 1\nmeas",true,L"measurement");
    check(L"praat.call(\"To Spe",true,L"To Spectrogram",true);
    check(L"praat.call ( 'Read seg",true,L"Read seg files",true);
    check(L"  To Spe",false,L"To Spectrogram",true);
    check(L"result$ = \"a\"\nres",false,L"result$");
    for(auto s:{L"-- praat.call(\"To Spe",L"-- math.s",L"-- ",L"\"math.s",L"[=[ math.s"}) {
        auto c=context(s,wcslen(s),true);assert(complete(s,c,true,commands).empty());
    }
    assert(checkPraat(L"if 1\r\nfor i to 3\r\nendfor\r\nelse\r\nendif").empty());
    assert(checkPraat(L"ifValue = 1\n# if\nwriteInfoLine: \"if\"\n; endfor").empty());
    auto e=checkPraat(L"if 1\nwriteInfoLine: \"hello\"");assert(e.size()==1&&e[0].line==1);
    e=checkPraat(L"if 1\nendfor");assert(e.size()==2&&e[1].line==2);
    e=checkPraat(L"if 1\nelse\nelse\nendif");assert(e.size()==1&&e[0].line==3);
    assert(checkPraat(L"include blocks.praat\nendif").empty());
    auto own=std::wstring(L"local function spectrum(samples, rate)\nend\nspec");
    auto suggestions=complete(own,context(own,own.size(),true),true,commands);
    assert(std::any_of(suggestions.begin(),suggestions.end(),[](const Entry&e){return e.name==L"spectrum"&&e.help.find(L"samples, rate")!=std::wstring::npos;}));
    auto c=context(L"praat.call(\"To Spectrogram\", math.s",33,true);assert(!c.commands);
    std::wstring unicode=L"local измерение = 1\nизмер";
    // Lexical masking must retain source offsets, including surrogate pairs on Windows.
    assert(mask(L"-- 😀\r\nlocal x=1",true).size()==std::wstring(L"-- 😀\r\nlocal x=1").size());
    std::cout<<"Code assist model tests PASS\n";
}
