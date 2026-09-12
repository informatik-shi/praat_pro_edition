/* SpectrogramEditor.cpp
 *
 * Copyright (C) 1992-2005,2007-2012,2014-2022 Paul Boersma
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

#include "SpectrogramEditor.h"
#include "luaplot/PlotHost.h"

Thing_implement (SpectrogramEditor, FunctionEditor, 0);

void structSpectrogramEditor::v9_destroy() noexcept{PlotHost_remove(this);SpectrogramEditor_Parent::v9_destroy();}
void structSpectrogramEditor::v_createMenus(){SpectrogramEditor_Parent::v_createMenus();PlotHost_addMenu(this);}
bool structSpectrogramEditor::hasLuaPanel(){return PlotHost_panel(this);}
void structSpectrogramEditor::clearLuaPlots(){PlotHost_clear(this);}
void structSpectrogramEditor::v_draw(){
    SpectrogramEditor_Parent::v_draw();auto area=spectrogramArea().get();double bottom=dataBottom_pxlt(),height=dataTop_pxlt()-bottom;
    FunctionArea_setViewport(area);PlotRect overlay;Graphics_inqViewport(graphics.get(),&overlay.left,&overlay.right,&overlay.bottom,&overlay.top);
    PlotHost_draw(this,graphics.get(),{dataLeft_pxlt(),dataRight_pxlt(),bottom,bottom+.28*height},
        overlay,0,area->maximum,true);
}

autoSpectrogramEditor SpectrogramEditor_create (conststring32 title, Spectrogram spectrogram) {
	try {
		autoSpectrogramEditor me = Thing_new (SpectrogramEditor);
		my spectrogramArea() = SpectrogramArea_create (true, nullptr, me.get());
		FunctionEditor_init (me.get(), title, spectrogram);
		auto host=me.get();
		PlotHost_register(host,"SpectrogramEditor",[host](){return PlotHostContext{host->tmin,host->tmax,host->startWindow,host->endWindow,host->startSelection,host->endSelection};},[host](){host->v_distributeAreas();FunctionEditor_redraw(host);});
		return me;
	} catch (MelderError) {
		Melder_throw (U"Spectrogram window not created.");
	}
}

/* End of file SpectrogramEditor.cpp */
