// Praat Pro Edition. GPL-3.0-or-later. No GUI dependencies.
#pragma once
#include <algorithm>
#include <cctype>
#include <map>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

namespace AnnotationKeys {
// Key numbers match GuiMenu; modifier bits match the Windows menu dispatcher.
struct Key {
    int code=0, modifiers=0; // Shift=1, Ctrl=2, Alt=4
    bool operator==(Key other) const {return code==other.code&&modifiers==other.modifiers;}
    bool operator!=(Key other) const {return !(*this==other);}
};
inline std::string trim(std::string s) {
    auto first=s.find_first_not_of(" \t\r\n"),last=s.find_last_not_of(" \t\r\n");
    return first==std::string::npos?"":s.substr(first,last-first+1);
}
inline std::string lower(std::string s) {
    for(char &c:s)c=(char)std::tolower((unsigned char)c);return s;
}
inline const std::vector<std::string>& names() {
    static const std::vector<std::string> value={"", "", "", "", "Left", "Right", "Up", "Down", "Pause", "Delete", "Insert", "Backspace", "Tab", "LineFeed", "Home", "End", "Enter", "PageUp", "PageDown", "Escape", "F1", "F2", "F3", "F4", "F5", "F6", "F7", "F8", "F9", "F10", "F11", "F12"};
    return value;
}
inline std::string format(Key k) {
    if(!k.code)return "None";
    std::string result;
    if(k.modifiers&2)result+="Ctrl+";
    if(k.modifiers&4)result+="Alt+";
    if(k.modifiers&1)result+="Shift+";
    return result+(k.code<32?names().at(k.code):k.code==32?"Space":std::string(1,(char)k.code));
}
inline Key parse(std::string text) {
    text=trim(text);if(text.empty()||lower(text)=="none")return {};
    Key result;std::istringstream input(text);std::string part;bool keySeen=false;
    while(std::getline(input,part,'+')) {
        part=lower(trim(part));int modifier=part=="ctrl"||part=="control"?2:part=="shift"?1:part=="alt"?4:0;
        if(modifier) {
            if(keySeen||(result.modifiers&modifier))throw std::runtime_error("Put each modifier once, before the key.");
            result.modifiers|=modifier;continue;
        }
        if(keySeen||part.empty())throw std::runtime_error("Use one key, e.g. Ctrl+Enter or F4.");
        if(part=="return")part="enter";
        if(part=="esc")part="escape";
        if(part=="pgup")part="pageup";
        if(part=="pgdn")part="pagedown";
        if(part=="space")result.code=32;
        else if(part.size()==1&&std::isalnum((unsigned char)part[0])&&part[0]<127)result.code=std::toupper((unsigned char)part[0]);
        else for(size_t i=4;i<names().size();++i)if(part==lower(names()[i])){result.code=(int)i;break;}
        if(!result.code)throw std::runtime_error("Unknown key. Use A-Z, 0-9, F1-F12, Enter, Tab, arrows, or navigation keys.");
        keySeen=true;
    }
    if(!keySeen||text.back()=='+')throw std::runtime_error("A key must follow the modifiers.");
    if(result.code>=48&&result.code<=90&&!(result.modifiers&6))throw std::runtime_error("Letters and digits need Ctrl or Alt so that labels can still be typed.");
    if(result.code==32&&!(result.modifiers&6))throw std::runtime_error("Space needs Ctrl or Alt so that spaces can still be typed in labels.");
    if((result.modifiers&2)&&!(result.modifiers&4)&&(result.code=='C'||result.code=='X'||result.code=='V'||result.code=='A'||result.code=='Z'))
        throw std::runtime_error("Ctrl+A/C/X/V/Z (including Shift variants) are reserved for label text editing.");
    // Leave Windows close/system-menu combinations to the operating system.
    if(result.modifiers==4&&(result.code==23||result.code==32))throw std::runtime_error("Alt+F4 and Alt+Space are reserved for Windows.");
    return result;
}
struct Action {std::string id;Key defaultKey;};
struct Document {
    std::vector<std::string> lines;
    std::map<std::string,Key> overrides;
    static Document read(std::istream &input) {
        Document result;std::string line,section;size_t number=0;
        while(std::getline(input,line)) {
            if(++number==1&&line.substr(0,3)=="\xef\xbb\xbf")line.erase(0,3);
            if(!line.empty()&&line.back()=='\r')line.pop_back();
            result.lines.push_back(line);auto clean=trim(line);
            if(clean.empty()||clean[0]==';'||clean[0]=='#')continue;
            if(clean[0]=='[') {
                if(clean.back()!=']')throw std::runtime_error("INI section is not closed on line "+std::to_string(number));
                section=clean.substr(1,clean.size()-2);continue;
            }
            if(section!="TextGridEditor.Shortcuts")continue;
            auto equal=clean.find('=');
            if(equal==std::string::npos)throw std::runtime_error("Expected command=shortcut on INI line "+std::to_string(number));
            auto id=trim(clean.substr(0,equal));auto value=trim(clean.substr(equal+1));
            auto comment=value.find_first_of(";#");if(comment!=std::string::npos)value=trim(value.substr(0,comment));
            if(id.empty()||result.overrides.count(id))throw std::runtime_error("Empty or duplicate INI command on line "+std::to_string(number));
            try {result.overrides[id]=parse(value);}catch(const std::exception &e){throw std::runtime_error("INI line "+std::to_string(number)+": "+e.what());}
        }
        if(input.bad())throw std::runtime_error("Cannot read shortcut INI.");
        return result;
    }
    void change(const std::string &id,Key value,bool useDefault=false) {
        bool inSection=false,found=false,hasSection=false;size_t sectionEnd=lines.size();
        for(size_t i=0;i<lines.size();++i) {
            auto clean=trim(lines[i]);
            if(!clean.empty()&&clean[0]=='[') {
                if(inSection)sectionEnd=i;
                inSection=clean=="[TextGridEditor.Shortcuts]";hasSection|=inSection;continue;
            }
            auto equal=clean.find('=');
            if(inSection&&equal!=std::string::npos&&trim(clean.substr(0,equal))==id) {
                auto comment=lines[i].find_first_of(";#",lines[i].find('=')+1);
                std::string tail=comment==std::string::npos?"":" "+lines[i].substr(comment);
                lines[i]=useDefault?"; Default restored: "+id+tail:id+"="+format(value)+tail;found=true;
            }
        }
        if(!useDefault&&!found) {
            if(!hasSection){lines.push_back("");lines.push_back("[TextGridEditor.Shortcuts]");sectionEnd=lines.size();}
            lines.insert(lines.begin()+sectionEnd,id+"="+format(value));
        }
        if(useDefault)overrides.erase(id);else overrides[id]=value;
    }
    Key effective(const Action &a) const {auto i=overrides.find(a.id);return i==overrides.end()?a.defaultKey:i->second;}
    void validate(const std::vector<Action> &actions) const {
        std::map<std::pair<int,int>,const Action*> assigned;
        for(const auto &a:actions) {
            Key key=effective(a);if(!key.code)continue;
            auto inserted=assigned.emplace(std::make_pair(key.code,key.modifiers),&a);
            if(!inserted.second) {
                auto &other=*inserted.first->second;
                // Preserve pre-existing Praat defaults, but never introduce a new collision.
                if(key!=a.defaultKey||key!=other.defaultKey)
                    throw std::runtime_error(format(key)+" conflicts: "+other.id+" / "+a.id+". Disable the other assignment first.");
            }
        }
    }
    void write(std::ostream &output) const {
        for(const auto &line:lines)output<<line<<'\n';
        if(!output)throw std::runtime_error("Cannot write shortcut INI.");
    }
};
}
