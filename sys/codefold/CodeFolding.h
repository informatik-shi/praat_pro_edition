// Praat Custom. GPL-3.0-or-later.
#ifndef PRAAT_CODE_FOLDING_H
#define PRAAT_CODE_FOLDING_H
#include "Gui.h"
#include <string>
void CodeFolding_sync(GuiText text);
unsigned long CodeFolding_revision(GuiText text);
long CodeFolding_lineCount(GuiText text);
long CodeFolding_lineStart(GuiText text,long line);
long CodeFolding_lineAt(GuiText text,long position);
long CodeFolding_firstVisible(GuiText text);
long CodeFolding_lastVisible(GuiText text,long first,long count);
std::wstring CodeFolding_lineText(GuiText text,long line);
void CodeFolding_install(GuiText text,int gutterWidth,bool lua);
void CodeFolding_destroy(GuiText text);
bool CodeFolding_hiddenLine(GuiText text,long zeroBasedLine);
// 0 toggle containing block; 1 collapse all; 2 expand all.
void CodeFolding_action(GuiText text,int action);
#endif
