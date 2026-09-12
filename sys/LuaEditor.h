// Praat Custom. GPL-3.0-or-later.
#ifndef _LuaEditor_h_
#define _LuaEditor_h_
#include "TextEditor.h"
Thing_define (LuaEditor, TextEditor) {
    bool running, stopRequested;
    void v_createChildren () override;
    void v_createMenus () override;
    void v_nameChanged () override;
    void v_goAway () override;
    void v9_destroy () noexcept override;
    conststring32 v_extension () const override { return U".lua"; }
};
autoLuaEditor LuaEditor_create ();
bool LuaEditors_dirty ();
#endif
