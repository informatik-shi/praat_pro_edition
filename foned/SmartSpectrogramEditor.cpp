// Praat Pro Edition. GPL-3.0-or-later.
#include "SmartSpectrogramEditor.h"
#include "EditorM.h"
#include <algorithm>
#include <cmath>

Thing_implement (SmartSpectrogramArea, SoundAnalysisArea, 0);
Thing_implement (SmartSpectrogramEditor, FunctionEditor, 0);

#include "Prefs_define.h"
#include "SmartSpectrogramArea_prefs.h"
#include "Prefs_install.h"
#include "SmartSpectrogramArea_prefs.h"
#include "Prefs_copyToInstance.h"
#include "SmartSpectrogramArea_prefs.h"

double structSmartSpectrogramArea::displayedMaximumFrequencyHz () const {
    SmartSpectrogram s = spectrum();
    const double fullMaximum = s->firstFrequencyHz + (s->numberOfBins - 1) * s->frequencyStepHz;
    const double requestedMaximum = instancePref_maximumFrequencyHz();
    return std::isfinite (requestedMaximum) && requestedMaximum > s->firstFrequencyHz
        ? std::min (requestedMaximum, fullMaximum) : fullMaximum;
}

autoSmartSpectrogramEditor SmartSpectrogramEditor_create (conststring32 title, SmartSpectrogram spectrum, Sound sound) {
    SmartSpectrogram_validate (spectrum);
    if (sound) Melder_require (spectrum->xmin >= sound->xmin && spectrum->xmax <= sound->xmax,
        U"SmartSpectrogram time domain must lie within the selected Sound; no time offset is applied.");
    auto me = Thing_new (SmartSpectrogramEditor);
    my spectrumArea() = SmartSpectrogramArea_create (true, nullptr, me.get());
    my spectrumArea()->smartSpectrum = spectrum;
    my spectrumArea()->d_spectrogram_cursor = undefined;
    if (sound) my soundArea() = SoundArea_create (false, sound, me.get());
    FunctionEditor_init (me.get(), title, spectrum);
    return me;
}
void structSmartSpectrogramEditor::v1_dataChanged (Editor sender) {
    auto spectrum = static_cast<SmartSpectrogram> (data());
    SmartSpectrogram_validate (spectrum);
    spectrumArea()->smartSpectrum = spectrum;
    if (soundArea()) {
        soundArea()->functionChanged (nullptr);
        spectrumArea()->functionChanged (soundArea()->sound());
    } else {
        spectrumArea()->functionChanged (nullptr);
    }
    SmartSpectrogramEditor_Parent::v1_dataChanged (sender);
}
void structSmartSpectrogramEditor::v_draw () {
    if (soundArea()) {
        FunctionArea_prepareCanvas (soundArea().get());
        if (spectrumArea()->instancePref_pulses_show())
            spectrumArea()->v_draw_analysis_pulses ();
        FunctionArea_drawInside (soundArea().get());
    }
    FunctionArea_prepareCanvas (spectrumArea().get());
    spectrumArea()->v_draw_analysis ();
}
void structSmartSpectrogramEditor::v_drawLegends () {
    auto area = spectrumArea().get();
    FunctionArea_drawLegend (area,
        Melder_cat (U"SmartSpectrogram (dB); frame ", area->selectedFrame, U", bin ", area->selectedBin), Melder_BLACK,
        soundArea() && area->instancePref_formant_show() ? FunctionArea_legend_SPECKLES U" %%formants" : U"", Melder_RED,
        soundArea() && area->instancePref_intensity_show() ? FunctionArea_legend_LINES U" %%intensity" : U"", Melder_GREEN,
        soundArea() && area->instancePref_pitch_show() ? FunctionArea_legend_LINES_SPECKLES U" %%pitch" : U"", Melder_BLUE);
    if (soundArea()) FunctionArea_drawLegend (soundArea().get(),
        area->instancePref_pulses_show() ? FunctionArea_legend_POLES U" %%pulses" : U"", Melder_GREY,
        FunctionArea_legend_WAVEFORM U" ##Sound", Melder_BLACK);
}
void structSmartSpectrogramArea::v_draw_analysis () {
    if (sound() && endWindow() - startWindow() <= instancePref_longestAnalysis()) {
        SmartSpectrogramArea_Parent::v_draw_analysis ();
        return;
    }
    v_draw_analysis_spectrogram ();
    const double fmin = v_spectrogramViewFrom(), fmax = v_spectrogramViewTo();
    Graphics_setWindow (graphics(), startWindow(), endWindow(), fmin, fmax);
    Graphics_setColour (graphics(), DataGuiColour_NONEDITABLE);
    Graphics_setTextAlignment (graphics(), Graphics_RIGHT, Graphics_HALF);
    Graphics_text (graphics(), startWindow(), fmin, Melder_graphicalHalf (fmin), U" Hz");
    Graphics_text (graphics(), startWindow(), fmax, Melder_graphicalHalf (fmax), U" Hz");
    if (std::isfinite (d_spectrogram_cursor) && d_spectrogram_cursor > fmin && d_spectrogram_cursor < fmax) {
        Graphics_setLineType (graphics(), Graphics_DOTTED);
        Graphics_setColour (graphics(), DataGuiColour_NONEDITABLE_SELECTED);
        Graphics_text (graphics(), startWindow(), d_spectrogram_cursor,
            Melder_graphicalHalf (d_spectrogram_cursor), U" Hz");
        Graphics_line (graphics(), startWindow(), d_spectrogram_cursor, endWindow(), d_spectrogram_cursor);
        Graphics_setLineType (graphics(), Graphics_DRAWN);
    }
    Graphics_setColour (graphics(), Melder_BLACK);
}
bool structSmartSpectrogramArea::v_draw_analysis_spectrogram () {
    if (sound() && ! instancePref_spectrogram_show())
        return true;
    SmartSpectrogram s = spectrum();
    const double fmin = s->firstFrequencyHz;
    const double fmax = displayedMaximumFrequencyHz();
    const double left = startWindow(), right = endWindow();
    FunctionArea_setViewport (this);
    Graphics_setWindow (graphics(), left, right, fmin, fmax);
    const integer columns = 1000;
    const integer rows = std::min (s->numberOfBins, 600_integer);
    auto image = raw_MAT (rows, columns);
    double peak = -1e300;
    for (integer col = 1, frame = 1; col <= columns; ++ col) {
        const double time = left + (col - 0.5) * (right - left) / columns;
        while (frame < s->numberOfFrames - 1 && s->times [frame + 1] < time) ++ frame;
        const integer next = std::min (s->numberOfFrames, frame + 1);
        const double blend = ! interpolateFrames || next == frame ? 0.0 :
            Melder_clipped (0.0, (time - s->times [frame]) / (s->times [next] - s->times [frame]), 1.0);
        const integer nearest = blend < 0.5 ? frame : next;
        for (integer row = 1; row <= rows; ++ row) {
            const double binPosition = 1.0 + (row - 1.0) * (fmax - fmin) / (s->frequencyStepHz * (rows - 1.0));
            const integer bin1 = Melder_clipped (1_integer, (integer) std::floor (binPosition), s->numberOfBins);
            const integer bin2 = std::min (s->numberOfBins, bin1 + 1);
            const double fy = binPosition - bin1;
            double value;
            if (interpolateFrames) {
                const double low = s->valuesDb [bin1] [frame] * (1 - blend) + s->valuesDb [bin1] [next] * blend;
                const double high = s->valuesDb [bin2] [frame] * (1 - blend) + s->valuesDb [bin2] [next] * blend;
                value = low * (1 - fy) + high * fy;
            } else {
                value = s->valuesDb [bin1] [nearest] * (1 - fy) + s->valuesDb [bin2] [nearest] * fy;
            }
            image [row] [col] = value;
            peak = std::max (peak, value);
        }
    }
    Graphics_image (graphics(), image.all(), left, right, fmin, fmax, peak - dynamicRangeDb, peak);

    if (s->pitch) {
        Graphics_setColour (graphics(), Melder_CYAN);
        Graphics_setLineWidth (graphics(), 2.0);
        double oldTime = undefined, oldHz = undefined;
        for (integer i = 1; i <= s->pitch->nx; ++ i) {
            const double time = s->pitch->x1 + (i - 1) * s->pitch->dx;
            if (time < left || time > right) continue;
            const double hz = Pitch_getValueAtTime (s->pitch.get(), time, kPitch_unit::HERTZ, false);
            if (std::isfinite (hz) && hz >= fmin && hz <= fmax) {
                if (std::isfinite (oldHz) && time - oldTime <= 1.5 * s->pitch->dx)
                    Graphics_line (graphics(), oldTime, oldHz, time, hz);
                oldTime = time; oldHz = hz;
            } else oldHz = undefined;
        }
    }
    if (s->trajectories) {
        const MelderColour colours[] = { Melder_RED, Melder_BLUE, Melder_GREEN, Melder_MAGENTA };
        for (integer track = 1; track <= s->trajectories->numberOfTracks; ++ track) {
            Graphics_setColour (graphics(), colours [(track - 1) % 4]);
            Graphics_setLineWidth (graphics(), 2.0);
            auto tier = s->trajectories->tracks.at [track];
            for (integer i = 1; i <= tier->points.size; ++ i) {
                auto point = tier->points.at [i];
                if (point->number >= left && point->number <= right && point->value >= fmin && point->value <= fmax)
                    Graphics_fillCircle_mm (graphics(), point->number, point->value, 1.5);
                if (i > 1) {
                    auto previous = tier->points.at [i - 1];
                    Graphics_line (graphics(), previous->number, previous->value, point->number, point->value);
                }
            }
        }
    }
    if (s->intensity) {
        // Intensity is a time/dB curve, not a frequency. Give it a labelled strip
        // at the top of the spectrogram and keep the frequency axis unchanged.
        double low = 1e300, high = -1e300;
        for (integer i = 1; i <= s->intensity->nx; ++ i) {
            const double time = s->intensity->x1 + (i - 1) * s->intensity->dx;
            if (time < left || time > right) continue;
            const double db = s->intensity->z [1] [i];
            if (std::isfinite (db)) { low = std::min (low, db); high = std::max (high, db); }
        }
        if (low <= high) {
            if (high == low) { low -= 1; high += 1; }
            Graphics_setColour (graphics(), Melder_RED);
            Graphics_setLineWidth (graphics(), 2.0);
            double oldTime = undefined, oldY = undefined;
            for (integer i = 1; i <= s->intensity->nx; ++ i) {
                const double time = s->intensity->x1 + (i - 1) * s->intensity->dx;
                if (time < left || time > right) continue;
                const double db = s->intensity->z [1] [i];
                if (! std::isfinite (db)) continue;
                const double y = fmin + (fmax - fmin) * (0.78 + 0.20 * (db - low) / (high - low));
                if (std::isfinite (oldY)) Graphics_line (graphics(), oldTime, oldY, time, y);
                oldTime = time; oldY = y;
            }
            Graphics_setTextAlignment (graphics(), Graphics_LEFT, Graphics_TOP);
            Graphics_text (graphics(), left, fmax, U"Intensity (dB): ", Melder_fixed (low, 1), U"–", Melder_fixed (high, 1));
        }
    }
    Graphics_setLineWidth (graphics(), 1.0);
    Graphics_setColour (graphics(), Melder_BLACK);
    return true;
}
bool structSmartSpectrogramArea::v_mouse (GuiDrawingArea_MouseEvent event, double x, double y) {
    if (event->isClick()) {
        SmartSpectrogram s = spectrum();
        integer left = 1, right = s->numberOfFrames;
        while (right - left > 1) {
            integer mid = (left + right) / 2;
            if (s->times [mid] <= x) left = mid; else right = mid;
        }
        selectedFrame = right != left && std::abs (s->times [right] - x) < std::abs (s->times [left] - x) ? right : left;
        const double frequency = v_spectrogramViewFrom() + y * (v_spectrogramViewTo() - v_spectrogramViewFrom());
        selectedBin = Melder_clipped (1_integer,
            (integer) std::llround ((frequency - s->firstFrequencyHz) / s->frequencyStepHz) + 1, s->numberOfBins);
    }
    const bool handled = SmartSpectrogramArea_Parent::v_mouse (event, x, y);
    if (event->isClick()) FunctionEditor_redraw (functionEditor());
    return handled || true;
}
static void menu_settings (SmartSpectrogramArea me, EDITOR_ARGS) {
    EDITOR_FORM (U"SmartSpectrogram display", nullptr)
        REAL (maximumFrequency, U"Maximum frequency (Hz; 0 = full range)", U"0")
        POSITIVE (range, U"Dynamic range (dB)", U"60")
        BOOLEAN (smooth, U"Interpolate between frames", true)
    EDITOR_OK
        SET_REAL (maximumFrequency, my instancePref_maximumFrequencyHz())
        SET_REAL (range, my dynamicRangeDb)
        SET_BOOLEAN (smooth, my interpolateFrames)
    EDITOR_DO
        Melder_require (std::isfinite (maximumFrequency) && maximumFrequency >= 0.0 &&
            (maximumFrequency == 0.0 || maximumFrequency > my spectrum()->firstFrequencyHz),
            U"Maximum frequency must be 0 for the full range, or higher than the first frequency (",
            my spectrum()->firstFrequencyHz, U" Hz).");
        Melder_require (range <= 300, U"Dynamic range must be <= 300 dB.");
        my setInstancePref_maximumFrequencyHz (maximumFrequency);
        my dynamicRangeDb = range;
        my interpolateFrames = smooth;
        FunctionEditor_redraw (my functionEditor());
    EDITOR_END
}
static void menu_setValue (SmartSpectrogramArea me, EDITOR_ARGS) {
    EDITOR_FORM (U"Set SmartSpectrogram value", nullptr)
        REAL (db, U"Value (dB)", U"-60")
    EDITOR_OK
        SET_REAL (db, my spectrum()->valuesDb [my selectedBin] [my selectedFrame])
    EDITOR_DO
        Melder_require (std::isfinite (db), U"dB value must be finite.");
        FunctionArea_save (me, U"Set spectrum value");
        my spectrum()->valuesDb [my selectedBin] [my selectedFrame] = db;
        FunctionArea_broadcastDataChanged (me);
    EDITOR_END
}
static void menu_showSpectrogram (SmartSpectrogramArea me, EDITOR_ARGS) {
    my setInstancePref_spectrogram_show (! my instancePref_spectrogram_show());
    GuiMenuItem_check (my spectrogramToggle, my instancePref_spectrogram_show());
    FunctionEditor_redraw (my functionEditor());
}
static void QUERY_EDITOR_FOR_REAL__smartFrequency (SmartSpectrogramArea me, EDITOR_ARGS) {
    QUERY_EDITOR_FOR_REAL
        const double result = my d_spectrogram_cursor;
    QUERY_EDITOR_FOR_REAL_END (U" Hz")
}
static void QUERY_DATA_FOR_REAL__smartValueAtCursor (SmartSpectrogramArea me, EDITOR_ARGS) {
    QUERY_DATA_FOR_REAL
        Melder_require (std::isfinite (my d_spectrogram_cursor), U"Click in the SmartSpectrogram first.");
        const double time = 0.5 * (my startSelection() + my endSelection());
        const double result = SmartSpectrogram_valueAt (my spectrum(), time, my d_spectrogram_cursor);
    QUERY_DATA_FOR_REAL_END (U" dB")
}
static void menu_moveFrequencyCursor (SmartSpectrogramArea me, EDITOR_ARGS) {
    EDITOR_FORM (U"Move frequency cursor to", nullptr)
        REAL (frequency, U"Frequency (Hz)", U"0")
    EDITOR_OK
        SET_REAL (frequency, std::isfinite (my d_spectrogram_cursor) ? my d_spectrogram_cursor : my v_spectrogramViewFrom())
    EDITOR_DO
        Melder_require (std::isfinite (frequency) && frequency >= my v_spectrogramViewFrom() &&
            frequency <= my v_spectrogramViewTo(), U"Frequency must lie inside the displayed range.");
        my d_spectrogram_cursor = frequency;
        my selectedBin = Melder_clipped (1_integer,
            (integer) std::llround ((frequency - my spectrum()->firstFrequencyHz) / my spectrum()->frequencyStepHz) + 1,
            my spectrum()->numberOfBins);
        FunctionEditor_redraw (my functionEditor());
    EDITOR_END
}
void structSmartSpectrogramArea::v_createMenus () {
    const bool hasSound = !! static_cast<SmartSpectrogramEditor> (functionEditor())->soundArea();
    if (hasSound)
        SmartSpectrogramArea_Parent::v_createMenus ();
    auto menu = Editor_addMenu (functionEditor(), U"SmartSpectrogram", 0);
    if (hasSound)
        spectrogramToggle = FunctionAreaMenu_addCommand (menu, U"Show spectrogram",
            GuiMenu_CHECKBUTTON | (instancePref_spectrogram_show() ? GuiMenu_TOGGLE_ON : 0),
            menu_showSpectrogram, this);
    FunctionAreaMenu_addCommand (menu, U"Display settings...", 0, menu_settings, this);
    FunctionAreaMenu_addCommand (menu, U"Get frequency at frequency cursor", 0, QUERY_EDITOR_FOR_REAL__smartFrequency, this);
    FunctionAreaMenu_addCommand (menu, U"Get value at cursor cross", 0, QUERY_DATA_FOR_REAL__smartValueAtCursor, this);
    FunctionAreaMenu_addCommand (menu, U"Move frequency cursor to...", 0, menu_moveFrequencyCursor, this);
    FunctionAreaMenu_addCommand (menu, U"Set selected value...", 0, menu_setValue, this);
}
