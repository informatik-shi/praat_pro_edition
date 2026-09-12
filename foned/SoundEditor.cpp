/* SoundEditor.cpp
 *
 * Copyright (C) 1992-2022 Paul Boersma
 *
 * This code is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or (at
 * your option) any later version.
 *
 * This code is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this work. If not, see <http://www.gnu.org/licenses/>.
 */

#include "SoundEditor.h"
#include "EditorM.h"
#include "luaplot/PlotHost.h"

Thing_implement (SoundEditor, FunctionEditor, 0);

void structSoundEditor::v9_destroy() noexcept {PlotHost_remove(this);SoundEditor_Parent::v9_destroy();}
void structSoundEditor::v_createMenus(){SoundEditor_Parent::v_createMenus();PlotHost_addMenu(this);}
bool structSoundEditor::hasLuaPanel(){return PlotHost_panel(this);}
void structSoundEditor::clearLuaPlots(){PlotHost_clear(this);}
void structSoundEditor::drawLuaPlots(){
    auto host=PlotHost_find(this);if(!host||!host->visible||(!host->panel&&!host->overlay))return;
    auto area=soundAnalysisArea().get();double bottom=dataBottom_pxlt(),height=dataTop_pxlt()-bottom;
    PlotRect overlay{dataLeft_pxlt(),dataRight_pxlt(),bottom,dataTop_pxlt()};
    if(area->hasContentToShow()){FunctionArea_setViewport(area);Graphics_inqViewport(graphics.get(),&overlay.left,&overlay.right,&overlay.bottom,&overlay.top);}
    PlotHost_draw(this,graphics.get(),{dataLeft_pxlt(),dataRight_pxlt(),bottom,bottom+.28*height},
        overlay,
        area->instancePref_spectrogram_viewFrom(),area->instancePref_spectrogram_viewTo(),area->instancePref_spectrogram_show());
}

static void menu_cb_SoundEditorHelp (SoundEditor, EDITOR_ARGS) { Melder_help (U"SoundEditor"); }
static void menu_cb_LongSoundEditorHelp (SoundEditor, EDITOR_ARGS) { Melder_help (U"LongSoundEditor"); }

void structSoundEditor :: v_createMenuItems_help (EditorMenu menu) {
	structFunctionEditor :: v_createMenuItems_help (menu);
	EditorMenu_addCommand (menu, U"SoundEditor help", '?', menu_cb_SoundEditorHelp);
	EditorMenu_addCommand (menu, U"LongSoundEditor help", 0, menu_cb_LongSoundEditorHelp);
	// BUG: add help on Sound area and Sound analysis area
}

autoSoundEditor SoundEditor_create (conststring32 title, SampledXY soundOrLongSound) {
	Melder_assert (soundOrLongSound);
	Melder_assert (soundOrLongSound -> ny > 0);
	try {
		autoSoundEditor me = Thing_new (SoundEditor);
		if (Thing_isa (soundOrLongSound, classSound))
			my soundArea() = SoundArea_create (true, nullptr, me.get());
		else
			my soundArea() = LongSoundArea_create (false, nullptr, me.get());
		my soundAnalysisArea() = SoundAnalysisArea_create (false, nullptr, me.get());
		FunctionEditor_init (me.get(), title, soundOrLongSound);
		auto host=me.get();
		PlotHost_register(host,"SoundEditor",[host](){return PlotHostContext{host->tmin,host->tmax,host->startWindow,host->endWindow,host->startSelection,host->endSelection};},[host](){host->v_distributeAreas();FunctionEditor_redraw(host);});
		return me;
	} catch (MelderError) {
		Melder_throw (U"Sound window not created.");
	}
}

/* End of file SoundEditor.cpp */
