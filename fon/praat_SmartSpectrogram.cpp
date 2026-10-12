// Praat Pro Edition. GPL-3.0-or-later.
#include "praat.h"
#include "praatM.h"
#include "SmartSpectrogram.h"
#include "SmartSpectrogramEditor.h"

FORM_READ (READ_SmartSpectrogram, U"Read SmartSpectrogram from CSV or TSV", nullptr, true) {
    READ_ONE
        autoSmartSpectrogram result = SmartSpectrogram_read (file);
    READ_ONE_END
}
FORM_SAVE (SAVE_SmartSpectrogram_csv, U"Save SmartSpectrogram as CSV", nullptr, U"csv") {
    SAVE_ONE (SmartSpectrogram, U"save as CSV")
        SmartSpectrogram_write (me, file, false);
    SAVE_ONE_END
}
FORM_SAVE (SAVE_SmartSpectrogram_tsv, U"Save SmartSpectrogram as TSV", nullptr, U"tsv") {
    SAVE_ONE (SmartSpectrogram, U"save as TSV")
        SmartSpectrogram_write (me, file, true);
    SAVE_ONE_END
}
DIRECT (EDIT_SmartSpectrogram) {
    EDITOR_ONE_WITH_ONE (a, SmartSpectrogram, Sound)
        autoSmartSpectrogramEditor editor = SmartSpectrogramEditor_create (ID_AND_FULL_NAME, me, you);
    EDITOR_ONE_WITH_ONE_END
}
DIRECT (ATTACH_SmartSpectrogram_Pitch) {
    MODIFY_FIRST_OF_ONE_AND_ONE (SmartSpectrogram, Pitch)
        my pitch = Data_copy (you);
    MODIFY_FIRST_OF_ONE_AND_ONE_END
}
DIRECT (ATTACH_SmartSpectrogram_Intensity) {
    MODIFY_FIRST_OF_ONE_AND_ONE (SmartSpectrogram, Intensity)
        my intensity = Data_copy (you);
    MODIFY_FIRST_OF_ONE_AND_ONE_END
}
DIRECT (ATTACH_SmartSpectrogram_Trajectories) {
    MODIFY_FIRST_OF_ONE_AND_ONE (SmartSpectrogram, FrequencyTrajectories)
        my trajectories = Data_copy (you);
    MODIFY_FIRST_OF_ONE_AND_ONE_END
}
DIRECT (CLEAR_SmartSpectrogram_overlays) {
    MODIFY_EACH (SmartSpectrogram)
        my pitch.reset(); my intensity.reset(); my trajectories.reset();
    MODIFY_EACH_END
}
FORM (MODIFY_SmartSpectrogram_value, U"Set SmartSpectrogram value", nullptr) {
    NATURAL (frame, U"Frame number", U"1")
    NATURAL (bin, U"Frequency bin number", U"1")
    REAL (db, U"Value (dB)", U"-60")
    OK
DO
    MODIFY_EACH (SmartSpectrogram)
        Melder_require (frame <= my numberOfFrames && bin <= my numberOfBins && std::isfinite (db),
            U"Frame/bin out of range or non-finite dB value.");
        my valuesDb [bin] [frame] = db;
    MODIFY_EACH_END
}
FORM (QUERY_SmartSpectrogram_value, U"Get SmartSpectrogram value", nullptr) {
    REAL (time, U"Time (s)", U"0")
    REAL (frequency, U"Frequency (Hz)", U"1000")
    OK
DO
    QUERY_ONE_FOR_REAL (SmartSpectrogram)
        const double result = SmartSpectrogram_valueAt (me, time, frequency);
    QUERY_ONE_FOR_REAL_END (U" dB")
}
void praat_SmartSpectrogram_init () {
    structSmartSpectrogramArea::f_preferences ();
    Thing_recognizeClassesByName (classSmartSpectrogram);
    praat_addMenuCommand (U"Objects", U"Open", U"Read SmartSpectrogram from CSV/TSV...", nullptr, 0, READ_SmartSpectrogram);
    praat_addAction1 (classSmartSpectrogram, 1, U"View & Edit", nullptr, GuiMenu_ATTRACTIVE, EDIT_SmartSpectrogram);
    praat_addAction2 (classSmartSpectrogram, 1, classSound, 1, U"View & Edit", nullptr, GuiMenu_ATTRACTIVE, EDIT_SmartSpectrogram);
    praat_addAction1 (classSmartSpectrogram, 1, U"Save as CSV file...", nullptr, 0, SAVE_SmartSpectrogram_csv);
    praat_addAction1 (classSmartSpectrogram, 1, U"Save as TSV file...", nullptr, 0, SAVE_SmartSpectrogram_tsv);
    praat_addAction2 (classSmartSpectrogram, 1, classPitch, 1, U"Attach Pitch", nullptr, 0, ATTACH_SmartSpectrogram_Pitch);
    praat_addAction2 (classSmartSpectrogram, 1, classIntensity, 1, U"Attach Intensity", nullptr, 0, ATTACH_SmartSpectrogram_Intensity);
    praat_addAction2 (classSmartSpectrogram, 1, classFrequencyTrajectories, 1, U"Attach FrequencyTrajectories", nullptr, 0, ATTACH_SmartSpectrogram_Trajectories);
    praat_addAction1 (classSmartSpectrogram, 1, U"Clear overlays", nullptr, 0, CLEAR_SmartSpectrogram_overlays);
    praat_addAction1 (classSmartSpectrogram, 1, U"Set value...", nullptr, 0, MODIFY_SmartSpectrogram_value);
    praat_addAction1 (classSmartSpectrogram, 1, U"Get value at time and frequency...", nullptr, 0, QUERY_SmartSpectrogram_value);
}
