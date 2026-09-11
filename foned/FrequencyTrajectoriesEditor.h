// Praat Custom. GPL-3.0-or-later.
#ifndef _FrequencyTrajectoriesEditor_h_
#define _FrequencyTrajectoriesEditor_h_
#include "FrequencyTrajectories.h"
#include "FunctionEditor.h"
#include "RealTierArea.h"
#include "SoundArea.h"
#include "Spectrogram.h"
Thing_define(FrequencyTrajectoriesArea, RealTierArea) {
    FrequencyTrajectories trajectories=nullptr;
    Sound backgroundSound=nullptr;
    integer selectedTrack=1;
    double ceiling=5500.0, windowLength=0.005, dynamicRange=50.0;
    double cachedStart=undefined,cachedEnd=undefined;
    autoSpectrogram spectrogram;
    void v_updateScaling() override {ymin=0;ymax=ceiling;ycursor=Melder_clipped(0.0,ycursor,ceiling);}
    double v_minimumLegalY() override {return 0.0;}
    double v_maximumLegalY() override {return ceiling;}
    conststring32 v_rightTickUnits() override {return U" Hz";}
    void v_drawInside() override;
    bool v_mouse(GuiDrawingArea_MouseEvent event,double x,double y) override;
    void v_createMenus() override;
};
DEFINE_FunctionArea_create(FrequencyTrajectoriesArea, RealTier)
Thing_define(FrequencyTrajectoriesEditor, FunctionEditor) {
    DEFINE_FunctionArea(1, FrequencyTrajectoriesArea, trajectoriesArea)
    DEFINE_FunctionArea(2, SoundArea, soundArea)
    void v1_dataChanged(Editor sender) override;
    void v_distributeAreas() override {
        trajectoriesArea()->setGlobalYRange_fraction(0.0,soundArea()?0.72:1.0);
        if(soundArea())soundArea()->setGlobalYRange_fraction(0.72,1.0);
    }
    void v_play(double start,double end) override {
        if(soundArea())Sound_playPart(soundArea()->sound(),start,end,theFunctionEditor_playCallback,this);
    }
    void v_drawLegends() override;
};
autoFrequencyTrajectoriesEditor FrequencyTrajectoriesEditor_create(conststring32 title,FrequencyTrajectories tracks,Sound optionalSound);
#endif
