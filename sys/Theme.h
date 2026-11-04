// Praat Pro Edition. GPL-3.0-or-later.
#ifndef PRAAT_PRO_THEME_H
#define PRAAT_PRO_THEME_H
#include "melder.h"
struct structGraphics;
void Theme_initialize();
bool Theme_enabled();
bool Theme_dark();
MelderColour Theme_colour(const char *key,MelderColour fallback);
unsigned long Theme_nativeColour(const char *key,unsigned long fallback);
MelderColour Theme_editorColour(MelderColour original);
bool Theme_editorGraphics(structGraphics *g);
MelderColour Theme_spectrogramGrey(double grey);
void Theme_installWindows();
#endif
