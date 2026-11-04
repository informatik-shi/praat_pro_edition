// Explicit UI roles bypass the generic black/white axis contrast mapping.
#ifndef PRAAT_THEME_PAINT_H
#define PRAAT_THEME_PAINT_H
#include "../Theme.h"
#include "../Graphics.h"
struct ThemePaint_Explicit {
    Graphics g;bool previous;
    explicit ThemePaint_Explicit(Graphics canvas):g(canvas),previous(canvas->editorTheme){g->editorTheme=false;}
    ~ThemePaint_Explicit(){g->editorTheme=previous;}
};
inline void ThemePaint_set(Graphics g,const char *role,MelderColour fallback) {
    const auto value=Theme_colour(role,fallback);
    const bool editor=g->editorTheme;g->editorTheme=false;
    Graphics_setColour(g,value);g->editorTheme=editor;
}
#endif
