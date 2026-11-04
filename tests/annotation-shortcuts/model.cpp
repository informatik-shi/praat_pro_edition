#include "../../foned/shortcuts/Model.h"
#include <cassert>
#include <iostream>
using namespace AnnotationKeys;
template<class F> void rejects(F f){bool failed=false;try{f();}catch(const std::exception&){failed=true;}assert(failed);}
int main(){
    assert(parse(" ctrl + shift + b ")==Key({'B',3}));
    assert(parse("Alt+Right")==Key({5,4}));
    assert(parse("F12")==Key({31,0}));
    assert(parse("Ctrl+Space")==Key({32,2}));
    assert(format(parse("Control+Return"))=="Ctrl+Enter");
    assert(parse("None")==Key());
    assert(parse("")==Key());
    for(auto s:{"Q","Shift+Q","Space","Ctrl+A","Ctrl+Shift+C","Ctrl+Z","Alt+F4","Ctrl++","Ctrl+","Ctrl+Ctrl+J","F13","F2+Ctrl","Win+B"})rejects([&]{parse(s);});
    std::vector<Action> actions={{"Boundary/Add on selected tier",{16,0}},{"Play/Play or stop",{12,0}},{"Time/Zoom in",{'I',2}}};
    std::istringstream ini("\xef\xbb\xbf; comment\r\n[other]\r\nx=y\r\n[TextGridEditor.Shortcuts]\r\nBoundary/Add on selected tier = F4 ; keep this\r\nUnknown future command=Ctrl+K\r\n[another]\r\nx=1\r\n");
    auto doc=Document::read(ini);doc.validate(actions);
    assert(doc.effective(actions[0])==parse("F4"));
    assert(doc.effective(actions[1])==Key({12,0}));
    doc.change(actions[1].id,parse("Ctrl+Enter"));doc.validate(actions);
    doc.change(actions[0].id,parse("Ctrl+Enter"));rejects([&]{doc.validate(actions);});
    doc.change(actions[0].id,{},true);doc.validate(actions);
    assert(doc.effective(actions[0])==Key({16,0}));
    std::ostringstream output;doc.write(output);auto saved=output.str();
    assert(saved.find("; keep this")!=std::string::npos);
    assert(saved.find("Unknown future command=Ctrl+K")!=std::string::npos);
    assert(saved.find("Play/Play or stop=Ctrl+Enter")<saved.find("[another]"));
    std::istringstream roundtrip(saved);auto copy=Document::read(roundtrip);copy.validate(actions);
    assert(copy.effective(actions[1])==parse("Ctrl+Enter"));
    std::istringstream duplicate("[TextGridEditor.Shortcuts]\na=F4\na=F5\n");rejects([&]{Document::read(duplicate);});
    std::istringstream malformed("[TextGridEditor.Shortcuts]\na=Ctrl+\n");rejects([&]{Document::read(malformed);});
    // Inherited collisions do not block configuration; new collisions do.
    std::vector<Action> baseline={{"one",{20,0}},{"two",{20,0}}};Document empty;empty.validate(baseline);
    empty.change("one",parse("F2"));empty.validate(baseline);
    empty.change("two",parse("F2"));rejects([&]{empty.validate(baseline);});
    std::cout<<"ANNOTATION SHORTCUT MODEL: PASS\n";
}
