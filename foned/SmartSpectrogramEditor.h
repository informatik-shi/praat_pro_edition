// Praat Pro Edition. GPL-3.0-or-later.
#ifndef _SmartSpectrogramEditor_h_
#define _SmartSpectrogramEditor_h_
#include "FunctionEditor.h"
#include "SoundAnalysisArea.h"
#include "SoundArea.h"
#include "SmartSpectrogram.h"

Thing_define (SmartSpectrogramArea, SoundAnalysisArea) {
    SmartSpectrogram smartSpectrum = nullptr;
    SmartSpectrogram spectrum () const { return smartSpectrum; }
    bool v_hasNativeSpectrogramMenus () override { return false; }
    double displayedMaximumFrequencyHz () const;
    double dynamicRangeDb = 60.0;
    bool interpolateFrames = true;
    integer selectedFrame = 1, selectedBin = 1;
    void v_draw_analysis () override;
    bool v_draw_analysis_spectrogram () override;
    double v_spectrogramViewFrom () const override { return spectrum()->firstFrequencyHz; }
    double v_spectrogramViewTo () const override { return displayedMaximumFrequencyHz(); }
    bool v_mouse (GuiDrawingArea_MouseEvent event, double x, double y) override;
    void v_createMenus () override;
    #include "SmartSpectrogramArea_prefs.h"
};
DEFINE_FunctionArea_create (SmartSpectrogramArea, Sound)

Thing_define (SmartSpectrogramEditor, FunctionEditor) {
    DEFINE_FunctionArea (1, SmartSpectrogramArea, spectrumArea)
    DEFINE_FunctionArea (2, SoundArea, soundArea)
    void v1_dataChanged (Editor sender) override;
    void v_distributeAreas () override {
        spectrumArea()->setGlobalYRange_fraction (0.0, soundArea() ? 0.5 : 1.0);
        if (soundArea()) soundArea()->setGlobalYRange_fraction (0.5, 1.0);
    }
    void v_draw () override;
    void v_play (double start, double end) override {
        if (soundArea()) Sound_playPart (soundArea()->sound(), start, end, theFunctionEditor_playCallback, this);
    }
    void v_drawLegends () override;
};
autoSmartSpectrogramEditor SmartSpectrogramEditor_create (conststring32 title, SmartSpectrogram spectrum, Sound optionalSound);
#endif
