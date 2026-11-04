// Praat Pro Edition. GPL-3.0-or-later.
#ifndef PRAAT_THEME_PALETTE_H
#define PRAAT_THEME_PALETTE_H
#include <algorithm>
#include <cctype>
#include <cstdint>
#include <istream>
#include <map>
#include <string>
#include <vector>
namespace PraatTheme {
using RGB = std::uint32_t; // 0xRRGGBB (independent of Windows COLORREF)
inline std::string trim(std::string s) {
    auto first=s.find_first_not_of(" \t\r\n");
    return first==std::string::npos?"":s.substr(first,s.find_last_not_of(" \t\r\n")-first+1);
}
inline std::string lower(std::string s) {for(char &c:s)c=(char)std::tolower((unsigned char)c);return s;}
inline bool parseColour(std::string s,RGB &value) {
    s=trim(s);if(s.size()!=7||s[0]!='#')return false;
    RGB result=0;
    for(std::size_t i=1;i<s.size();++i) {
        unsigned char c=(unsigned char)s[i]; int n=c>='0'&&c<='9'?c-'0':c>='a'&&c<='f'?c-'a'+10:c>='A'&&c<='F'?c-'A'+10:-1;
        if(n<0)return false;result=(result<<4)|n;
    }
    value=result;return true;
}
struct Palette {
    bool enabled=true,dark=true;
    std::map<std::string,RGB> colours;
    std::vector<std::string> warnings;
    RGB get(const std::string &key,RGB fallback) const {auto it=colours.find(key);return enabled&&it!=colours.end()?it->second:fallback;}
    static Palette defaults(bool dark=true) {
        Palette p;p.dark=dark;
        p.colours={
            {"window.background",dark?0x20242b:0xe0e0e0},{"window.text",dark?0xe5e9f0:0x202328},
            {"window.control_background",dark?0x2c323c:0xffffff},{"window.border",dark?0x546174:0x9a9a9a},
            {"window.selection",dark?0x365b80:0x0078d7},{"window.selection_text",0xffffff},
            {"editor.background",dark?0x181c22:0xffffff},{"editor.text",dark?0xe5e9f0:0x202328},
            {"editor.gutter",dark?0x242a33:0xf2f4f7},{"editor.line_number",dark?0x9aa8ba:0x676f7b},
            {"editor.breakpoint",dark?0xff6b7a:0xbe2d32},{"editor.execution",dark?0x514323:0xffefa7},
            {"editor.error",dark?0xff6b7a:0xd22323},
            {"syntax.keyword",dark?0x82aaff:0x1c45b3},{"syntax.form",dark?0xc792ea:0x733796},
            {"syntax.comment",dark?0x89a88a:0x3a7845},{"syntax.string",dark?0xc3e88d:0x9b3725},
            {"syntax.number",dark?0xf7bf7c:0x7e3996},{"syntax.string_variable",dark?0xffcb8b:0x934e1c},
            {"syntax.array",dark?0x80cbc4:0x0c6f7a},{"syntax.function",dark?0x82d2ed:0x006382},
            {"syntax.procedure",dark?0xe6a0ee:0x8b2892},{"syntax.command",dark?0x89ddff:0x125d80},
            {"analysis.background",dark?0x14191f:0xffffff},{"analysis.text",dark?0xdce5f0:0x000000},
            {"analysis.waveform",dark?0x73d7e8:0x006680},{"analysis.waveform_selected",dark?0xffc078:0xbf5900},
            {"analysis.frame",dark?0x557487:0x00ccff},{"analysis.cursor",dark?0xff7b8b:0xcc0000},
            {"analysis.selection",dark?0x334d6b:0xffd2d2},{"analysis.pitch",dark?0x82aaff:0x0000ff},
            {"analysis.pitch_secondary",dark?0xb6caff:0x00ffff},{"analysis.intensity",dark?0xa9db70:0x00aa00},
            {"analysis.formants",dark?0xff8177:0xff0000},{"analysis.formants_secondary",dark?0xf4b4cc:0xff8080},
            {"analysis.spectrum",dark?0xc7e99b:0x000000},{"analysis.pulses",dark?0x8795a8:0xc0c0c0},
            {"spectrogram.background",dark?0x14191f:0xffffff},{"spectrogram.foreground",dark?0xbacbd9:0x000000}
        };
        const RGB tracks[]={0xff89cf,0x65d6e8,0xff9c75,0x9eacff,0xc8df80,0xd5a3f0,0x8bdec1,0xffd58a,0xd5b5a0,0xa0c8ff};
        for(int i=0;i<10;++i)p.colours["trajectory.f"+std::to_string(i+1)]=tracks[i];
        return p;
    }
    static Palette read(std::istream &in) {
        std::vector<std::pair<std::string,std::string>> entries;std::string line,section;unsigned lineNumber=0;
        while(std::getline(in,line)) {
            if(++lineNumber==1&&line.compare(0,3,"\xef\xbb\xbf")==0)line.erase(0,3);
            line=trim(line);if(line.empty()||line[0]==';'||line[0]=='#')continue;
            if(line[0]=='['&&line.back()==']'){section=lower(trim(line.substr(1,line.size()-2)));continue;}
            auto equals=line.find('=');if(equals==std::string::npos){entries.push_back({"!invalid",std::to_string(lineNumber)});continue;}
            auto value=trim(line.substr(equals+1));auto comment=value.find(';');if(comment!=std::string::npos)value=trim(value.substr(0,comment));
            entries.push_back({section+"."+lower(trim(line.substr(0,equals))),value});
        }
        bool dark=true;
        for(auto &e:entries)if(e.first=="theme.base")dark=lower(e.second)!="light";
        Palette p=defaults(dark);
        for(auto &e:entries) {
            auto v=lower(e.second);
            if(e.first=="theme.base") {if(v!="dark"&&v!="light")p.warnings.push_back("Invalid theme.base: "+e.second);continue;}
            if(e.first=="theme.enabled") {if(v=="false"||v=="0")p.enabled=false;else if(v=="true"||v=="1")p.enabled=true;else p.warnings.push_back("Invalid theme.enabled: "+e.second);continue;}
            auto found=p.colours.find(e.first);RGB colour;
            if(found==p.colours.end())p.warnings.push_back("Unknown theme key: "+e.first);
            else if(!parseColour(e.second,colour))p.warnings.push_back("Invalid #RRGGBB colour: "+e.first);
            else found->second=colour;
        }
        return p;
    }
};
inline RGB blend(RGB a,RGB b,double weight) {
    weight=std::max(0.0,std::min(1.0,weight));RGB result=0;
    for(int shift:{16,8,0})result|=RGB(((a>>shift)&255)*(1-weight)+((b>>shift)&255)*weight+0.5)<<shift;
    return result;
}
}
#endif
