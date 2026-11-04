#ifndef PRAAT_SMART_FRAME_ACCESS_H
#define PRAAT_SMART_FRAME_ACCESS_H
#include "../SmartSpectrogram.h"
#include "../Spectrum.h"
integer SmartSpectrogram_nearestFrame(SmartSpectrogram me,double time);
bool SmartSpectrogram_hasFrameAt(SmartSpectrogram me,double time);
integer SmartSpectrogram_frameAt(SmartSpectrogram me,double time);
autoSpectrum SmartSpectrogram_to_Spectrum_nearest(SmartSpectrogram me,double time);
#endif
