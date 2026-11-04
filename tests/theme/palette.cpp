#include "../../sys/theme/Palette.h"
#include <cassert>
#include <sstream>
#include <iostream>
using namespace PraatTheme;
int main(){
 RGB colour=0;assert(parseColour(" #12aBcF ",colour)&&colour==0x12abcf);
 for(auto invalid:{"#123","#11223344","#gg0000","blue","#12345x"})assert(!parseColour(invalid,colour));
 std::istringstream input("\xef\xbb\xbf[analysis]\r\nwaveform=#aBcD09 ; note\r\n[Theme]\nBASE=light\n[window]\ntext=#102030\n[syntax]\nnumber=#abcdef\ncomment=invalid\n[unknown]\nkey=#000000\n");
 auto p=Palette::read(input);assert(p.enabled&&!p.dark);assert(p.get("analysis.waveform",0)==0xabcd09);
 assert(p.get("analysis.background",0)==0xffffff);assert(p.get("window.text",0)==0x102030);
 assert(p.get("syntax.number",0)==0xabcdef);assert(p.get("syntax.comment",0)==0x3a7845);assert(p.warnings.size()==2);
 std::istringstream disabled("[theme]\nenabled=false\n[analysis]\nwaveform=#ffffff");
 auto off=Palette::read(disabled);assert(!off.enabled&&off.get("analysis.waveform",0x123456)==0x123456);
 std::istringstream duplicate("[analysis]\nwaveform=#123456\nwaveform=#abcdef\n");assert(Palette::read(duplicate).get("analysis.waveform",0)==0xabcdef);
 assert(blend(0x102030,0xa0b0c0,0)==0x102030);assert(blend(0x102030,0xa0b0c0,1)==0xa0b0c0);
 assert(blend(0,0xffffff,0.5)==0x808080);assert(blend(0,0xffffff,2)==0xffffff);
 auto dark=Palette::defaults();assert(dark.dark);assert(dark.get("spectrogram.background",0)==0x14191f);
 assert(dark.colours.size()>45);std::cout<<"Theme palette: PASS\n";
}
