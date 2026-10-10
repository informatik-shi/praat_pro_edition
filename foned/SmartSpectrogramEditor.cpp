// Praat Pro Edition. GPL-3.0-or-later.
#include "SmartSpectrogramEditor.h"
#include "EditorM.h"
#include <algorithm>
#include <cmath>

Thing_implement (SmartSpectrogramArea, FunctionArea, 0);
Thing_implement (SmartSpectrogramEditor, FunctionEditor, 0);

autoSmartSpectrogramEditor SmartSpectrogramEditor_create (conststring32 title, SmartSpectrogram spectrum, Sound sound) {
    SmartSpectrogram_validate (spectrum);
    if (sound) Melder_require (spectrum->xmin >= sound->xmin && spectrum->xmax <= sound->xmax,
        U"SmartSpectrogram time domain must lie within the selected Sound; no time offset is applied.");
    auto me = Thing_new (SmartSpectrogramEditor);
    my spectrumArea() = SmartSpectrogramArea_create (true, nullptr, me.get());
    if (sound) my soundArea() = SoundArea_create (false, sound, me.get());
    FunctionEditor_init (me.get(), title, spectrum);
    return me;
}
void structSmartSpectrogramEditor::v1_dataChanged (Editor sender) {
    auto spectrum = static_cast<SmartSpectrogram> (data());
    SmartSpectrogram_validate (spectrum);
    spectrumArea()->functionChanged (spectrum);
    if (soundArea()) soundArea()->functionChanged (nullptr);
    SmartSpectrogramEditor_Parent::v1_dataChanged (sender);
}
void structSmartSpectrogramEditor::v_drawLegends () {
    auto area = spectrumArea().get();
    FunctionArea_drawLegend (area, Melder_cat (U"SmartSpectrogram (dB); frame ", area->selectedFrame,
        U", bin ", area->selectedBin, U" — Spectrogram menu edits values"), Melder_BLACK);
    if (soundArea()) FunctionArea_drawLegend (soundArea().get(), U"Sound (reference); Play selection to listen", Melder_BLACK);
}
void structSmartSpectrogramArea::v_drawInside () {
    SmartSpectrogram s = spectrum();
    const double fmin = s->firstFrequencyHz;
    const double fmax = fmin + (s->numberOfBins - 1) * s->frequencyStepHz;
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
            const double binPosition = 1.0 + (row - 1.0) * (s->numberOfBins - 1.0) / (rows - 1.0);
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
        const double frequency = s->firstFrequencyHz + y * (s->numberOfBins - 1) * s->frequencyStepHz;
        selectedBin = Melder_clipped (1_integer,
            (integer) std::llround ((frequency - s->firstFrequencyHz) / s->frequencyStepHz) + 1, s->numberOfBins);
        FunctionEditor_redraw (functionEditor());
    }
    return FunctionEditor_defaultMouseInWideDataView (functionEditor(), event, x) || true;
}
static void menu_settings (SmartSpectrogramArea me, EDITOR_ARGS) {
    EDITOR_FORM (U"SmartSpectrogram display", nullptr)
        POSITIVE (range, U"Dynamic range (dB)", U"60")
        BOOLEAN (smooth, U"Interpolate between frames", true)
    EDITOR_OK
        SET_REAL (range, my dynamicRangeDb)
        SET_BOOLEAN (smooth, my interpolateFrames)
    EDITOR_DO
        Melder_require (range <= 300, U"Dynamic range must be <= 300 dB.");
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
void structSmartSpectrogramArea::v_createMenus () {
    auto menu = Editor_addMenu (functionEditor(), U"SmartSpectrogram", 0);
    FunctionAreaMenu_addCommand (menu, U"Display settings...", 0, menu_settings, this);
    FunctionAreaMenu_addCommand (menu, U"Set selected value...", 0, menu_setValue, this);
}
