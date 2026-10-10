// Praat Pro Edition. GPL-3.0-or-later.
#ifndef _SmartSpectrogram_h_
#define _SmartSpectrogram_h_
#include "Pitch.h"
#include "Intensity.h"
#include "FrequencyTrajectories.h"
#include "SmartSpectrogram_def.h"

autoSmartSpectrogram SmartSpectrogram_create (constVEC times, integer numberOfBins,
    double firstFrequencyHz, double frequencyStepHz);
autoSmartSpectrogram SmartSpectrogram_read (MelderFile file);
void SmartSpectrogram_write (SmartSpectrogram me, MelderFile file, bool tsv);
void SmartSpectrogram_validate (SmartSpectrogram me);
double SmartSpectrogram_valueAt (SmartSpectrogram me, double time, double frequencyHz);
#endif
