// Praat Pro Edition. GPL-3.0-or-later.
#ifndef _ProSoundEditor_h_
#define _ProSoundEditor_h_

#include "SoundEditor.h"
#include "TextGridArea.h"
#include "FrequencyTrajectories.h"
#include "SmartSpectrogram.h"
#include "Spectrogram.h"

Thing_define (ProTextGridArea, TextGridArea) {
    bool v_mouse (GuiDrawingArea_MouseEvent event, double x, double) override {
        return FunctionEditor_defaultMouseInWideDataView (functionEditor(), event, x);
    }
    void v_createMenus () override { }
    void v_createMenuItems_edit (EditorMenu) override { }
    void v_createMenuItems_save (EditorMenu) override { }
};
DEFINE_FunctionArea_create (ProTextGridArea, TextGrid)

Thing_define (ProSoundAnalysisArea, SoundAnalysisArea) {
    FrequencyTrajectories trajectories = nullptr;
    SmartSpectrogram smartSpectrogram = nullptr;
    Spectrogram selectedSpectrogram = nullptr;
    bool useSmartSpectrogram = true;
    integer selectedTrack = 1;
    integer selectedPoint = 0;
    integer draggingPoint = 0;
    integer draggingTrack = 0;
    double dragTime = undefined, dragFrequency = undefined;
    bool v_draw_analysis_spectrogram () override;
    void v_draw_analysis () override;
    bool v_mouse (GuiDrawingArea_MouseEvent event, double x, double y) override;
    void v_createMenus () override;
};
DEFINE_FunctionArea_create (ProSoundAnalysisArea, Sound)

Thing_define (ProSoundEditor, SoundEditor) {
    DEFINE_FunctionArea (3, ProTextGridArea, textGridArea)
    TextGrid referenceTextGrid = nullptr;
    ProSoundAnalysisArea overlayArea () {
        return static_cast<ProSoundAnalysisArea> (soundAnalysisArea().get());
    }
    void v1_dataChanged (Editor sender) override;
    void v_distributeAreas () override;
    void v_draw () override;
    void v_drawLegends () override;
};

autoProSoundEditor ProSoundEditor_create (conststring32 title, Sound sound,
    TextGrid textGrid, FrequencyTrajectories trajectories,
    SmartSpectrogram smartSpectrogram, Spectrogram selectedSpectrogram);

#endif
