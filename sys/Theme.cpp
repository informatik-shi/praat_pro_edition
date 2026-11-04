// Praat Pro Edition. GPL-3.0-or-later.
#include "Theme.h"
#include "Graphics.h"
#include "DataGui.h"
#include "theme/Palette.h"
#include <filesystem>
#include <fstream>
#if defined(_WIN32)
#include <windows.h>
#endif
namespace {
PraatTheme::Palette &palette() {
    static auto p=[] {
        auto result=PraatTheme::Palette::defaults();
#if !defined(_WIN32)
        result.enabled=false;
#endif
        return result;
    }();
    return p;
}
MelderColour fromRGB(PraatTheme::RGB c) {return MelderColour(((c>>16)&255)/255.0,((c>>8)&255)/255.0,(c&255)/255.0);}
}
void Theme_initialize() {
    static bool initialized=false;if(initialized)return;initialized=true;
#if defined(_WIN32)
    wchar_t path[32768];std::filesystem::path file;
    DWORD n=GetEnvironmentVariableW(L"PRAAT_THEME_INI",path,32768);
    if(n&&n<32768)file=path;
    else {n=GetModuleFileNameW(nullptr,path,32768);if(n&&n<32768)file=std::filesystem::path(path).parent_path()/L"praat-theme.ini";}
    std::ifstream input(file);if(input)palette()=PraatTheme::Palette::read(input);
    else if(n&&GetEnvironmentVariableW(L"PRAAT_THEME_INI",nullptr,0))OutputDebugStringW(L"Praat: cannot read PRAAT_THEME_INI; using default dark palette.\n");
    for(auto &warning:palette().warnings)OutputDebugStringA(("Praat theme: "+warning+"\n").c_str());
#endif
    if(!Theme_enabled())return;
    DataGuiColour_WINDOW_BACKGROUND=Theme_colour("window.background",DataGuiColour_WINDOW_BACKGROUND);
    DataGuiColour_AREA_BACKGROUND=Theme_colour("analysis.background",DataGuiColour_AREA_BACKGROUND);
    DataGuiColour_EDITABLE=Theme_colour("analysis.waveform",DataGuiColour_EDITABLE);
    DataGuiColour_EDITABLE_FRAME=Theme_colour("analysis.frame",DataGuiColour_EDITABLE_FRAME);
    DataGuiColour_EDITABLE_SELECTED=Theme_colour("analysis.waveform_selected",DataGuiColour_EDITABLE_SELECTED);
    DataGuiColour_NONEDITABLE=Theme_colour("analysis.waveform",DataGuiColour_NONEDITABLE);
    DataGuiColour_NONEDITABLE_FRAME=Theme_colour("analysis.frame",DataGuiColour_NONEDITABLE_FRAME);
    DataGuiColour_NONEDITABLE_SELECTED=Theme_colour("analysis.cursor",DataGuiColour_NONEDITABLE_SELECTED);
}
bool Theme_enabled(){return palette().enabled;}
bool Theme_dark(){return Theme_enabled()&&palette().dark;}
MelderColour Theme_colour(const char *key,MelderColour fallback){
    auto found=palette().colours.find(key);return Theme_enabled()&&found!=palette().colours.end()?fromRGB(found->second):fallback;
}
unsigned long Theme_nativeColour(const char *key,unsigned long fallback){
    auto c=palette().get(key,((fallback&255)<<16)|(fallback&0xff00)|((fallback>>16)&255));
    return ((c&255)<<16)|(c&0xff00)|((c>>16)&255);
}
bool Theme_editorGraphics(structGraphics *g){return g&&g->editorTheme&&g->screen&&!g->printer&&!g->metafile&&Theme_enabled();}
MelderColour Theme_editorColour(MelderColour c){
    // Praat's named red/blue/green are not pure RGB primaries.
    if(MelderColour_equal(c,Melder_RED)||MelderColour_equal(c,1.2*Melder_RED))return Theme_colour("analysis.formants",c);
    if(MelderColour_equal(c,Melder_MAROON))return Theme_colour("analysis.cursor",c);
    if(MelderColour_equal(c,Melder_BLUE)||MelderColour_equal(c,1.2*Melder_BLUE)||MelderColour_equal(c,Melder_NAVY))return Theme_colour("analysis.pitch",c);
    if(MelderColour_equal(c,Melder_GREEN)||MelderColour_equal(c,1.2*Melder_GREEN)||MelderColour_equal(c,Melder_LIME))return Theme_colour("analysis.intensity",c);
    if(c.red==0&&c.green==0&&c.blue==0)return Theme_colour("analysis.text",c);
    if(c.red==1&&c.green==1&&c.blue==1)return Theme_colour("analysis.background",c);
    if(c.red==c.green&&c.green==c.blue&&c.red>0&&c.red<1)
        return fromRGB(PraatTheme::blend(palette().get("window.border",0x888888),palette().get("window.background",0x20242b),c.red));
    return c;
}
MelderColour Theme_spectrogramGrey(double grey){return fromRGB(PraatTheme::blend(palette().get("spectrogram.foreground",0),palette().get("spectrogram.background",0xffffff),grey));}
#if !defined(_WIN32)
void Theme_installWindows(){}
#endif
