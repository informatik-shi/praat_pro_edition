// Linux component runner: the production editors plus an oversized GTK dialog.
#include "praat.h"
#include "GuiP.h"
#include "LuaEditor.h"
#include "ScriptEditor.h"
#include "../main/main_Praat.h"
#include <chrono>
#include <thread>

static void pump () {
    for (int i = 0; i < 30; ++ i) {
        while (gtk_events_pending ()) gtk_main_iteration ();
        std::this_thread::sleep_for (std::chrono::milliseconds (10));
    }
}

int main (int argc, char **argv) {
    try {
        praat_init (U"Praat", U"" stringize(PRAAT_VERSION_STR), PRAAT_VERSION_NUM,
            PRAAT_YEAR, PRAAT_MONTH, PRAAT_DAY, U"", U"", argc, argv);
        INCLUDE_LIBRARY (praat_uvafon_init)
        auto lua = LuaEditor_create ();
        GuiText_setString (lua -> textWidget, U"local x = 1\nprint(x)\n-- Проверка UTF-8\n");
        lua -> dirty = false;
        GuiShell_setTitle (lua -> windowForm, U"tablet_lua_editor");
        lua.releaseToUser ();
        auto script = ScriptEditor_createFromText (nullptr, U"writeInfoLine: \"Проверка UTF-8\"\n");
        GuiShell_setTitle (script -> windowForm, U"tablet_praat_editor");
        script.releaseToUser ();

        // Preserve every field in an oversized form instead of shrinking its contents.
        GuiDialog dialog = GuiDialog_create (nullptr, 0, 0, 1100, 1600,
            U"tablet_scroll_dialog", nullptr, nullptr, GuiDialog_Modality::MODELESS);
        GuiLabel_createShown (dialog, 20, 500, 20, 50, U"First field — Первая строка", 0);
        GuiLabel_createShown (dialog, 20, 500, 1500, 1530, U"Last field — Последняя строка", 0);
        GuiButton_createShown (dialog, 20, 150, 1540, 1575, U"OK", nullptr, nullptr, 0);
        GuiThing_show (dialog);
        pump ();
        GtkWidget *viewport = gtk_widget_get_parent (GTK_WIDGET (dialog -> d_widget));
        GtkWidget *scroll = gtk_widget_get_parent (viewport);
        Melder_require (GTK_IS_SCROLLED_WINDOW (scroll), U"Oversized dialog has no scroll container.");
        GtkAdjustment *vertical = gtk_scrolled_window_get_vadjustment (GTK_SCROLLED_WINDOW (scroll));
        Melder_require (gtk_adjustment_get_upper (vertical) > gtk_adjustment_get_page_size (vertical),
            U"Last fields cannot be reached by scrolling.");
        gtk_adjustment_set_value (vertical, gtk_adjustment_get_upper (vertical));
        pump ();
        Melder_require (gtk_adjustment_get_value (vertical) > 0, U"Dialog does not scroll.");
        GuiThing_hide (dialog);
        Melder_require (! gtk_widget_get_visible (GTK_WIDGET (dialog -> d_gtkWindow)), U"Dialog did not hide.");
        GuiThing_show (dialog);
        Melder_require (gtk_widget_get_visible (GTK_WIDGET (dialog -> d_gtkWindow)), U"Dialog did not reopen.");
        praat_run ();
        return 0;
    } catch (MelderError) {
        Melder_casual (Melder_getError ());
        Melder_clearError ();
        return 1;
    }
}
