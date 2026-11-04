// Praat Pro. GPL-3.0-or-later.
#ifndef PRAAT_CODE_ASSIST_H
#define PRAAT_CODE_ASSIST_H
#include "Gui.h"
void CodeAssist_install (GuiText text, bool lua);
void CodeAssist_destroy (GuiText text);
// 0 completion, 1 problems, 2 next problem.
void CodeAssist_action (GuiText text, int action);
#endif
