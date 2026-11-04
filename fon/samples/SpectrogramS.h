// Praat Pro Edition. GPL-3.0-or-later.
#ifndef PRAAT_SPECTROGRAM_S_H
#define PRAAT_SPECTROGRAM_S_H
#include "../Spectrogram.h"
#include "../Sound_and_Spectrogram.h"
#include "SpectrogramS_def.h"
autoSpectrogramS Sound_to_SpectrogramS(constSound sound,integer windowSamples,integer hopSamples,
    double maximumFrequency,integer fftSamples,kSound_to_Spectrogram_windowShape shape,integer sourceSampleOffset=0);
autoSpectrogram SpectrogramS_to_Spectrogram(constSpectrogramS me);
autoSpectrogramS Spectrogram_to_SpectrogramS(constSpectrogram me,double samplingFrequency,integer windowSamples,
    kSound_to_Spectrogram_windowShape shape);
void SpectrogramS_validate(constSpectrogramS me);
#endif
