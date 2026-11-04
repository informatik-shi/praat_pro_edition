#include "theme/Paint.h"
// Praat Pro Edition. GPL-3.0-or-later.
#include "ProSoundEditor.h"
#include "smart/PowerDisplay.h"
#include "../fon/smart/FrameAccess.h"
#include "EditorM.h"
#include "luaplot/PlotHost.h"
#include <algorithm>
#include <cmath>
#include <vector>

Thing_implement (ProTextGridArea, TextGridArea, 0);
Thing_implement (ProSoundAnalysisArea, SoundAnalysisArea, 0);
Thing_implement (ProSoundEditor, SoundEditor, 0);

autoProSoundEditor ProSoundEditor_create (conststring32 title, Sound sound,
    TextGrid textGrid, FrequencyTrajectories trajectories,
    SmartSpectrogram smartSpectrogram, Spectrogram selectedSpectrogram)
{
    Melder_require (sound && (trajectories || smartSpectrogram || selectedSpectrogram), U"Select Sound and a spectral layer or trajectories.");
    if (trajectories) FrequencyTrajectories_validate (trajectories);
    if (smartSpectrogram) SmartSpectrogram_validate (smartSpectrogram);
    auto me = Thing_new (ProSoundEditor);
    if (!trajectories) { my emptyTrajectories=FrequencyTrajectories_create(sound->xmin,sound->xmax,1);trajectories=my emptyTrajectories.get(); }
    my soundArea() = SoundArea_create (true, nullptr, me.get());
    my soundAnalysisArea() = ProSoundAnalysisArea_create (false, nullptr, me.get()).static_cast_move<structSoundAnalysisArea>();
    my overlayArea()->trajectories = trajectories;
    my overlayArea()->smartSpectrogram = smartSpectrogram;
    my overlayArea()->selectedSpectrogram = selectedSpectrogram;
    my referenceTextGrid = textGrid;
    if (textGrid) {
        my textGridArea() = ProTextGridArea_create (false, nullptr, me.get());
        my textGridArea()->borrowedSoundArea = my soundArea().get();
        my textGridArea()->borrowedSoundAnalysisArea = my soundAnalysisArea().get();
    }
    FunctionEditor_init (me.get(), title, sound);
    auto host = me.get();
    PlotHost_register (host, "SoundEditor",
        [host]() { return PlotHostContext {host->tmin,host->tmax,host->startWindow,host->endWindow,host->startSelection,host->endSelection}; },
        [host]() { host->v_distributeAreas(); FunctionEditor_redraw (host); });
    return me;
}

void structProSoundEditor::v1_dataChanged (Editor sender) {
    ProSoundEditor_Parent::v1_dataChanged (sender);
    if (textGridArea()) textGridArea()->functionChanged (referenceTextGrid);
}

void structProSoundEditor::v_distributeAreas () {
    const double panelBottom = hasLuaPanel() ? 0.30 : 0.0;
    const double lower = textGridArea() ? panelBottom + (1.0 - panelBottom) * 0.20 : panelBottom;
    if (textGridArea()) textGridArea()->setGlobalYRange_fraction (panelBottom, lower);
    if (soundAnalysisArea()->hasContentToShow() || overlayArea()->trajectories) {
        soundAnalysisArea()->setGlobalYRange_fraction (lower, lower + (1.0 - lower) * 0.52);
        soundArea()->setGlobalYRange_fraction (lower + (1.0 - lower) * 0.52, 1.0);
    } else {
        soundAnalysisArea()->setGlobalYRange_fraction (lower, lower);
        soundArea()->setGlobalYRange_fraction (lower, 1.0);
    }
}

void structProSoundEditor::v_draw () {
    FunctionArea_prepareCanvas (soundArea().get());
    if (soundAnalysisArea()->instancePref_pulses_show())
        soundAnalysisArea()->v_draw_analysis_pulses ();
    FunctionArea_drawInside (soundArea().get());
    FunctionArea_prepareCanvas (soundAnalysisArea().get());
    overlayArea()->v_draw_analysis ();
    drawLuaPlots ();
    if (textGridArea()) FunctionArea_drawOne (textGridArea().get());
}

void structProSoundEditor::v_drawLegends () {
    auto overlay = overlayArea();
    FunctionArea_drawLegend (soundArea().get(),
        overlay->hasPulsesToShow() ? FunctionArea_legend_POLES U" %%derived pulses" : nullptr,
        Melder_GREY,
        FunctionArea_legend_WAVEFORM U" ##modifiable sound",
        DataGui_defaultForegroundColour (soundArea().get(), false));
    if (textGridArea()) FunctionArea_drawLegend (textGridArea().get(),
        FunctionArea_legend_TEXTGRID U" %%TextGrid reference", Melder_BLACK);
    const bool hasSelectedPoint = overlay->trajectories && overlay->selectedPoint > 0 &&
        overlay->selectedTrack <= overlay->trajectories->numberOfTracks &&
        overlay->selectedPoint <= overlay->trajectories->tracks.at [overlay->selectedTrack]->points.size;
    conststring32 selectedLabel = hasSelectedPoint
        ? Melder_cat (U" ##", overlay->trajectories->trackNames [overlay->selectedTrack].get(),
            U": ", overlay->trajectories->tracks.at [overlay->selectedTrack]->points.at [overlay->selectedPoint]->value,
            U" Hz")
        : U"";
    conststring32 spectrumLabel = ! overlay->instancePref_spectrogram_show() ? U"" :
        overlay->useSmartSpectrogram && overlay->smartSpectrogram ? U"selected SmartSpectrogram" :
        overlay->selectedSpectrogram ? U"selected Spectrogram" : U"derived spectrogram";
    FunctionArea_drawLegend (overlay,
        Melder_cat (spectrumLabel, emptyTrajectories ? U"" : U"; frequency trajectories", selectedLabel), Melder_BLACK,
        overlay->instancePref_formant_show() ? FunctionArea_legend_SPECKLES U" %%derived formants" : U"", Theme_colour("analysis.formants", Melder_RED),
        overlay->instancePref_intensity_show() ? FunctionArea_legend_LINES U" %%derived intensity" : U"", Theme_colour("analysis.intensity", Melder_GREEN),
        overlay->instancePref_pitch_show() ? FunctionArea_legend_LINES_SPECKLES U" %%derived pitch" : U"", Theme_colour("analysis.pitch", Melder_BLUE));
}

bool structProSoundAnalysisArea::v_draw_analysis_spectrogram () {
    const bool custom = (useSmartSpectrogram && smartSpectrogram) || selectedSpectrogram;
    if (! custom) return false;
    if (! instancePref_spectrogram_show()) return true;
    const double tmin = startWindow(), tmax = endWindow();
    const double fmin = v_spectrogramViewFrom(), fmax = v_spectrogramViewTo();
    if (tmax <= tmin || fmax <= fmin) return true;
    FunctionArea_setViewport (this);
    Graphics_setWindow (graphics(), tmin, tmax, fmin, fmax);
    if (useSmartSpectrogram && smartSpectrogram) {
        SmartSpectrogram s = smartSpectrogram;
        const double low = std::max (fmin, s->firstFrequencyHz);
        const double high = std::min (fmax, s->firstFrequencyHz + (s->numberOfBins - 1) * s->frequencyStepHz);
        const double left = std::max (tmin, s->xmin), right = std::min (tmax, s->xmax);
        if (high <= low || right <= left) return true;
        const double resolution = Graphics_getResolution (graphics());
        const double visibleWidth = std::abs (Graphics_dxWCtoMM (graphics(), right - left)) * resolution / 25.4;
        const double visibleHeight = std::abs (Graphics_dyWCtoMM (graphics(), high - low)) * resolution / 25.4;
        const integer rows = std::min (s->numberOfBins,
            std::min (600_integer, std::max (2_integer, (integer) std::floor (visibleHeight))));
        const integer columns = std::min (1024_integer,
            std::max (2_integer, (integer) std::floor (visibleWidth / 1.5)));
        auto pixels = raw_MAT (rows, columns);
        std::vector<bool> validColumns ((size_t) columns, false);
        std::vector<double> columnPeaks ((size_t) columns, -1e300);
        double observedPeak = -1e300;
        for (integer col = 1, frame = 1; col <= columns; ++ col) {
            const double time = left + (col - 0.5) * (right - left) / columns;
            while (frame < s->numberOfFrames - 1 && s->times [frame + 1] < time) ++ frame;
            integer owner = SmartSpectrogram_frameAt(s,time);
            if (!owner) continue;
            validColumns[(size_t)col-1]=true;
            const bool smooth = smartInterpolateFrames && (!s->numberOfFrameIntervals ||
                (frame<s->numberOfFrames && s->frameStarts[frame+1]<=s->frameEnds[frame]+1e-12));
            const integer next = smooth ? std::min (s->numberOfFrames, frame + 1) : owner;
            if (!smooth) frame=owner;
            const double blend = next == frame ? 0.0 : Melder_clipped (0.0,
                (time - s->times [frame]) / (s->times [next] - s->times [frame]), 1.0);
            for (integer row = 1; row <= rows; ++ row) {
                const double hz = low + (row - 0.5) * (high - low) / rows;
                const double bandLow = low + (row - 1.0) * (high - low) / rows;
                const double bandHigh = low + row * (high - low) / rows;
                const double value = SmartPowerDisplay::bandPeak (
                    s->firstFrequencyHz, s->frequencyStepHz, s->numberOfBins, bandLow, bandHigh,
                    [&](integer bin) { return SmartPowerDisplay::blend (
                        s->valuesDb [bin] [frame], s->valuesDb [bin] [next], blend); }) +
                    instancePref_spectrogram_preemphasis() * std::log2 (hz / 1000.0 + 1e-308);
                pixels [row] [col] = value;
                columnPeaks [(size_t) col - 1] = std::max (columnPeaks [(size_t) col - 1], value);
                observedPeak = std::max (observedPeak, value);
            }
        }
        if(observedPeak == -1e300) return true;
        const double peak = instancePref_spectrogram_autoscaling()
            ? observedPeak : instancePref_spectrogram_maximum();
        const double compression = instancePref_spectrogram_dynamicCompression();
        const double floor = peak - instancePref_spectrogram_dynamicRange();
        for (integer col = 1; col <= columns; ++ col) {
            if(!validColumns[(size_t)col-1]) continue;
            const double adjustment = compression * (peak - columnPeaks [(size_t) col - 1]);
            for (integer row = 1; row <= rows; ++ row) {
                pixels [row] [col] += adjustment;
                if (instancePref_spectrogram_invertColours())
                    pixels [row] [col] = floor + peak - pixels [row] [col];
            }
        }
        // Small cell arrays use Praat's rectangle path on Windows. Large ones
        // fall back to the same bitmap path as Graphics_image, which can leave
        // this editor's overlay white on some Windows setups.
        for (integer row = 1; row <= rows; row += 40) {
            const integer lastRow = std::min (rows, row + 39);
            const double y1 = low + (row - 1.0) * (high - low) / rows;
            const double y2 = low + lastRow * (high - low) / rows;
            for (integer col = 1; col <= columns;) {
                if(!validColumns[(size_t)col-1]){++col;continue;}
                integer lastCol=col;
                while(lastCol<columns&&lastCol<col+59&&validColumns[(size_t)lastCol])++lastCol;
                const double x1 = left + (col - 1.0) * (right - left) / columns;
                const double x2 = left + lastCol * (right - left) / columns;
                auto tile = part_MAT (pixels.get(), row, lastRow, col, lastCol);
                Graphics_cellArray (graphics(), tile.all(), x1, x2, y1, y2, floor, peak);
                col=lastCol+1;
            }
        }
    } else if (selectedSpectrogram) {
        Spectrogram_paintInside (selectedSpectrogram, graphics(), tmin, tmax, fmin, fmax,
            instancePref_spectrogram_maximum(), instancePref_spectrogram_autoscaling(),
            instancePref_spectrogram_dynamicRange(), instancePref_spectrogram_preemphasis(),
            instancePref_spectrogram_dynamicCompression(), instancePref_spectrogram_colourMap(),
            instancePref_spectrogram_invertColours());
    }
    return true;
}

void structProSoundAnalysisArea::v_draw_analysis () {
    ProSoundAnalysisArea_Parent::v_draw_analysis ();
    if (! trajectories) return;
    const double fmin = v_spectrogramViewFrom(), fmax = v_spectrogramViewTo();
    FunctionArea_setViewport (this);
    Graphics_setWindow (graphics(), startWindow(), endWindow(), fmin, fmax);
    const MelderColour colours [] = { Melder_MAGENTA, Melder_CYAN, Melder_RED, Melder_BLUE };
    for (integer track = 1; track <= trajectories->numberOfTracks; ++ track) {
        auto tier = trajectories->tracks.at [track];
        ThemePaint_set(graphics(), ("trajectory.f" + std::to_string(1 + (track - 1) % 10)).c_str(), colours [(track - 1) % 4]);
        Graphics_setLineWidth (graphics(), track == selectedTrack ? 2.0 : 1.3);
        for (integer point = 1; point <= tier->points.size; ++ point) {
            auto current = tier->points.at [point];
            if (point > 1) {
                auto previous = tier->points.at [point - 1];
                Graphics_line (graphics(), previous->number, previous->value, current->number, current->value);
            }
            if (current->number >= startWindow() && current->number <= endWindow() &&
                current->value >= fmin && current->value <= fmax)
                Graphics_fillCircle_mm (graphics(), current->number, current->value, 1.7);
        }
    }
    if (draggingPoint) {
        Graphics_setColour (graphics(), Melder_BLACK);
        Graphics_circle_mm (graphics(), dragTime, dragFrequency, 3.0);
    }
    if (selectedPoint > 0 && selectedTrack <= trajectories->numberOfTracks &&
        selectedPoint <= trajectories->tracks.at [selectedTrack]->points.size) {
        const auto point = trajectories->tracks.at [selectedTrack]->points.at [selectedPoint];
        Graphics_setColour (graphics(), Melder_RED);
        Graphics_setTextAlignment (graphics(), Graphics_LEFT, Graphics_TOP);
        Graphics_text (graphics(), startWindow() + 0.02 * (endWindow() - startWindow()),
            fmax - 0.02 * (fmax - fmin),
            Melder_cat (trajectories->trackNames [selectedTrack].get(), U": ", point->value, U" Hz"));
    }
    Graphics_setLineWidth (graphics(), 1.0);
    Graphics_setColour (graphics(), Melder_BLACK);
}

bool structProSoundAnalysisArea::v_mouse (GuiDrawingArea_MouseEvent event, double x, double y) {
    const double fmin = v_spectrogramViewFrom(), fmax = v_spectrogramViewTo();
    const double hz = fmin + y * (fmax - fmin);
    if (event->isClick() && trajectories) {
        selectedPoint = 0;
        draggingPoint = draggingTrack = 0;
        FunctionArea_setViewport (this);
        Graphics_setWindow (graphics(), startWindow(), endWindow(), fmin, fmax);
        double best = 2.5;
        for (integer track = 1; track <= trajectories->numberOfTracks; ++ track) {
            auto tier = trajectories->tracks.at [track];
            const integer nearest = AnyTier_timeToNearestIndexInTimeWindow (
                tier->asAnyTier(), x, startWindow(), endWindow());
            if (! nearest) continue;
            auto point = tier->points.at [nearest];
            const double distance = Graphics_distanceWCtoMM (graphics(), x, hz, point->number, point->value);
            if (distance < best) {
                best = distance;
                draggingTrack = track; draggingPoint = nearest;
                selectedTrack = track;
                dragTime = point->number; dragFrequency = point->value;
            }
        }
        if (draggingPoint) {
            selectedPoint = draggingPoint;
            const bool parentUpdate = ProSoundAnalysisArea_Parent::v_mouse (event, x, y);
            d_spectrogram_cursor = dragFrequency;
            return parentUpdate || FunctionEditor_UPDATE_NEEDED;
        }
    }
    if (draggingPoint && (event->isDrag() || event->isDrop())) {
        dragTime = Melder_clipped (trajectories->xmin, x, trajectories->xmax);
        dragFrequency = Melder_clipped (0.0, hz, fmax);
        d_spectrogram_cursor = dragFrequency;
        if (event->isDrop()) {
            auto tier = trajectories->tracks.at [draggingTrack];
            auto point = tier->points.at [draggingPoint];
            const bool valid = (draggingPoint == 1 || dragTime > tier->points.at [draggingPoint - 1]->number) &&
                (draggingPoint == tier->points.size || dragTime < tier->points.at [draggingPoint + 1]->number);
            if (valid) {
                point->number = dragTime;
                point->value = dragFrequency;
                Editor_broadcastDataChanged (functionEditor());
            } else {
                d_spectrogram_cursor = point->value;
                Melder_beep ();
            }
            draggingPoint = draggingTrack = 0;
        }
        return FunctionEditor_UPDATE_NEEDED;
    }
    return ProSoundAnalysisArea_Parent::v_mouse (event, x, y);
}

static void menu_useSmart (ProSoundAnalysisArea me, EDITOR_ARGS) {
    my useSmartSpectrogram = true;
    FunctionEditor_redraw (my functionEditor());
}
static void menu_useRegular (ProSoundAnalysisArea me, EDITOR_ARGS) {
    my useSmartSpectrogram = false;
    FunctionEditor_redraw (my functionEditor());
}
static void menu_extractSmartSpectrum(ProSoundAnalysisArea me,EDITOR_ARGS) {
    CONVERT_DATA_TO_ONE
        const double time=0.5*(my startSelection()+my endSelection());
        Melder_require(my smartSpectrogram,U"No SmartSpectrogram is attached.");
        autoSpectrum result=SmartSpectrogram_to_Spectrum_nearest(my smartSpectrogram,time);
    CONVERT_DATA_TO_ONE_END(Melder_cat(U"smart_period_",Melder_double(my smartSpectrogram->times[
        SmartSpectrogram_nearestFrame(my smartSpectrogram,0.5*(my startSelection()+my endSelection()))])))
}
static void menu_smartDisplaySettings (ProSoundAnalysisArea me, EDITOR_ARGS) {
    EDITOR_FORM (U"Selected SmartSpectrogram display", nullptr)
        REAL (viewFrom, U"View from (Hz)", U"0")
        POSITIVE (viewTo, U"View to (Hz)", U"5000")
        POSITIVE (dynamicRange, U"Dynamic range (dB)", U"70")
        BOOLEAN(smooth,U"Interpolate between frames",false)
    EDITOR_OK
        SET_REAL (viewFrom, my instancePref_spectrogram_viewFrom())
        SET_REAL (viewTo, my instancePref_spectrogram_viewTo())
        SET_REAL (dynamicRange, my instancePref_spectrogram_dynamicRange())
        SET_BOOLEAN(smooth,my smartInterpolateFrames)
    EDITOR_DO
        Melder_require (viewFrom >= 0.0 && viewFrom < viewTo && dynamicRange <= 300.0,
            U"Use 0 <= View from < View to and a dynamic range up to 300 dB.");
        my setInstancePref_spectrogram_viewFrom (viewFrom);
        my setInstancePref_spectrogram_viewTo (viewTo);
        my setInstancePref_spectrogram_dynamicRange (dynamicRange);
        my smartInterpolateFrames=smooth;
        FunctionEditor_redraw (my functionEditor());
    EDITOR_END
}
void structProSoundAnalysisArea::v_createMenus () {
    ProSoundAnalysisArea_Parent::v_createMenus ();
    auto menu = Editor_addMenu (functionEditor(), U"Selected layers", 0);
    if (smartSpectrogram) {
        FunctionAreaMenu_addCommand (menu, U"Show selected SmartSpectrogram", 0, menu_useSmart, this);
        FunctionAreaMenu_addCommand (menu, U"SmartSpectrogram display settings...", 0, menu_smartDisplaySettings, this);
        FunctionAreaMenu_addCommand(menu,U"Extract nearest period spectrum",0,menu_extractSmartSpectrum,this);
    }
    if (selectedSpectrogram) FunctionAreaMenu_addCommand (menu, U"Show selected Spectrogram", 0, menu_useRegular, this);
}
