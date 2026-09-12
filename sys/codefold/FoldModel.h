// Praat Custom. GPL-3.0-or-later. Display-only block parser.
#ifndef PRAAT_FOLD_MODEL_H
#define PRAAT_FOLD_MODEL_H
#include <algorithm>
#include <string>
#include <vector>
namespace CodeFold {
struct Block { size_t first, last; bool collapsed=false; };
struct Model {
    std::wstring source;
    std::vector<size_t> lines;
    std::vector<Block> blocks;
    size_t lineAt(size_t offset) const {
        return (size_t)(std::upper_bound(lines.begin(),lines.end(),offset)-lines.begin()-1);
    }
    bool hidden(size_t line) const {
        for(const auto &b:blocks)if(b.collapsed && line>b.first && line<b.last)return true;
        return false;
    }
    void parse(std::wstring text,bool lua) {
        source=std::move(text);lines={0};blocks.clear();
        for(size_t i=0;i<source.size();++i) {
            if(source[i]==L'\r') {if(i+1<source.size()&&source[i+1]==L'\n')++i;lines.push_back(i+1);}
            else if(source[i]==L'\n')lines.push_back(i+1);
        }
        struct Open {std::wstring close;size_t line;bool needsDo=false;};
        std::vector<Open> stack;
        auto finish=[&](const std::wstring &word,size_t line) {
            if(stack.empty()||stack.back().close!=word)return;
            size_t first=stack.back().line;stack.pop_back();
            if(line>first+1)blocks.push_back({first,line});
        };
        if(!lua) {
            for(size_t line=0;line<lines.size();++line) {
                size_t p=lines[line],end=line+1<lines.size()?lines[line+1]:source.size();
                while(p<end&&(source[p]==L' '||source[p]==L'\t'))++p;
                size_t start=p;while(p<end&&((source[p]>=L'a'&&source[p]<=L'z')||(source[p]>=L'A'&&source[p]<=L'Z')))++p;
                auto word=source.substr(start,p-start);
                // Only standalone leading keywords; never names such as ifValue or for$.
                if(p<end&&source[p]!=L' '&&source[p]!=L'\t'&&source[p]!=L'\r'&&source[p]!=L'\n')continue;
                std::wstring close;
                if(word==L"if")close=L"endif";
                else if(word==L"for")close=L"endfor";
                else if(word==L"while")close=L"endwhile";
                else if(word==L"repeat")close=L"until";
                else if(word==L"procedure"||word==L"proc")close=L"endproc";
                else if(word==L"form")close=L"endform";
                if(!close.empty())stack.push_back({close,line});else finish(word,line);
            }
        } else {
            auto letter=[](wchar_t c){return (c>=L'a'&&c<=L'z')||(c>=L'A'&&c<=L'Z')||c==L'_';};
            for(size_t i=0;i<source.size();) {
                size_t start=i;
                bool comment=source.compare(i,2,L"--")==0;
                if(comment)i+=2;
                if(i<source.size()&&source[i]==L'[') {
                    size_t j=i+1;while(j<source.size()&&source[j]==L'=')++j;
                    if(j<source.size()&&source[j]==L'[') {
                        auto closing=L"]"+std::wstring(j-i-1,L'=')+L"]";
                        size_t end=source.find(closing,j+1);
                        if(end==std::wstring::npos){i=source.size();continue;}
                        size_t first=lineAt(start),last=lineAt(end);
                        if(last>first+1)blocks.push_back({first,last});
                        i=end+closing.size();continue;
                    }
                }
                if(comment){while(i<source.size()&&source[i]!=L'\r'&&source[i]!=L'\n')++i;continue;}
                if(source[i]==L'\''||source[i]==L'"') {
                    wchar_t quote=source[i++];
                    while(i<source.size()){if(source[i]==L'\\'){i=std::min(source.size(),i+2);continue;}if(source[i++]==quote)break;}
                    continue;
                }
                size_t line=lineAt(i);
                if(source[i]==L'{'){stack.push_back({L"}",line});++i;continue;}
                if(source[i]==L'}'){finish(L"}",line);++i;continue;}
                if(!letter(source[i])){++i;continue;}
                while(i<source.size()&&(letter(source[i])||(source[i]>=L'0'&&source[i]<=L'9')))++i;
                auto word=source.substr(start,i-start);
                if(word==L"function"||word==L"if")stack.push_back({L"end",line});
                else if(word==L"for"||word==L"while")stack.push_back({L"end",line,true});
                else if(word==L"do") {
                    if(!stack.empty()&&stack.back().needsDo)stack.back().needsDo=false;
                    else stack.push_back({L"end",line});
                } else if(word==L"repeat")stack.push_back({L"until",line});
                else if(word==L"end"||word==L"until")finish(word,line);
            }
        }
        std::sort(blocks.begin(),blocks.end(),[](const Block&a,const Block&b){return a.first!=b.first?a.first<b.first:a.last>b.last;});
        // One marker per opening line; choose the outer block on that line.
        blocks.erase(std::unique(blocks.begin(),blocks.end(),[](const Block&a,const Block&b){return a.first==b.first;}),blocks.end());
    }
};
}
#endif
