#include "../../sys/codefold/FoldModel.h"
#include <cassert>
#include <iostream>
int main() {
    CodeFold::Model p;
    p.parse(L"procedure test\n if 1\n  x = 2\n endif\nendproc\n",false);
    assert(p.blocks.size()==2&&p.blocks[0].first==0&&p.blocks[0].last==4);
    p.blocks[0].collapsed=true;assert(p.hidden(1)&&p.hidden(3)&&!p.hidden(4));
    p.parse(L"# if not code\nx$ = \"endproc\"\nfor i to 2\n x=1\nendfor\n",false);
    assert(p.blocks.size()==1&&p.blocks[0].first==2);
    p.parse(L"form title\n sentence name if end\nendform\nrepeat\n x=1\nuntil 1\n",false);
    assert(p.blocks.size()==2);
    p.parse(L"function run()\n for i=1,10 do\n  if i>2 then\n   print(\"end\") -- end\n  end\n end\nend\n",true);
    assert(p.blocks.size()==3&&p.blocks[0].last==6&&p.blocks[1].last==5);
    p.parse(L"--[=[\n function falseBlock()\n end\n]=]\nlocal s=[==[\n if end\n]==]\n",true);
    assert(p.blocks.size()==2&&p.blocks[0].last==3&&p.blocks[1].last==6);
    p.parse(L"local t={\n  f=function()\n   return 'end'\n  end,\n}\nrepeat\n x=1\nuntil true\n",true);
    assert(p.blocks.size()==3&&p.blocks[0].last==4);
    p.parse(L"while (function() return true end)() do\n x=1\nend\n",true);
    assert(p.blocks.size()==1&&p.blocks[0].last==2);
    p.parse(L"-- function\nprint(\"if \\\" end\")\nif true then\n print(1)\n",true);
    assert(p.blocks.empty());
    p.parse(L"if 1\r\n x$=\"𝄞\"\r\nendif",false);
    assert(p.blocks.size()==1&&p.lines[1]==6&&p.lines[2]==p.source.find(L"endif"));
    std::wstring large;for(int i=0;i<10000;++i)large+=L"for i to 2\n x=1\nendfor\n";
    p.parse(large,false);assert(p.blocks.size()==10000);
    std::cout<<"FOLD MODEL TESTS: PASS\n";
}
