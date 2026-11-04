// Praat Pro Edition. GPL-3.0-or-later.
#include "praat.h"
#include "praatM.h"
#include "samples/SpectrogramS.h"
#include "ProSoundEditor.h"
#include "SpectrogramEditor.h"
FORM(CONVERT_Sound_SpectrogramS,U"Sound: To SpectrogramS",nullptr){
    NATURAL(windowSamples,U"Analysis length (samples)",U"256")
    NATURAL(hopSamples,U"Time step (samples)",U"64")
    POSITIVE(maximumFrequency,U"Maximum frequency (Hz)",U"5000")
    NATURAL0(fftSamples,U"Transform size (samples; 0 = analysis length)",U"0")
    CHOICE_ENUM(kSound_to_Spectrogram_windowShape,shape,U"Window shape",kSound_to_Spectrogram_windowShape::DEFAULT)
    OK
DO
    CONVERT_EACH_TO_ONE(Sound)
        autoSpectrogramS result=Sound_to_SpectrogramS(me,windowSamples,hopSamples,maximumFrequency,fftSamples,shape);
    CONVERT_EACH_TO_ONE_END(my name.get())
}
DIRECT(CONVERT_SpectrogramS_Spectrogram){
    CONVERT_EACH_TO_ONE(SpectrogramS)
        autoSpectrogram result=SpectrogramS_to_Spectrogram(me);
    CONVERT_EACH_TO_ONE_END(my name.get())
}
FORM(CONVERT_Spectrogram_SpectrogramS,U"Spectrogram: To SpectrogramS",nullptr){
    POSITIVE(fs,U"Source sampling frequency (Hz)",U"22050")
    NATURAL0(window,U"Original analysis length (samples; 0 = unknown)",U"0")
    CHOICE_ENUM(kSound_to_Spectrogram_windowShape,shape,U"Original window shape",kSound_to_Spectrogram_windowShape::DEFAULT)
    COMMENT(U"Copies values and axes without recalculation. Source analysis metadata is not stored in a plain Spectrogram.")
    OK
DO
    CONVERT_EACH_TO_ONE(Spectrogram)
        autoSpectrogramS result=Spectrogram_to_SpectrogramS(me,fs,window,shape);
    CONVERT_EACH_TO_ONE_END(my name.get())
}
DIRECT(EDIT_SpectrogramS){
    EDITOR_ONE(a,SpectrogramS)
        autoSpectrogramEditor editor=SpectrogramEditor_create(ID_AND_FULL_NAME,me);
    EDITOR_ONE_END
}
DIRECT(EDIT_Sound_SpectrogramS){
    EDITOR_ONE_WITH_ONE(a,SpectrogramS,Sound)
        autoProSoundEditor editor=ProSoundEditor_create(ID_AND_FULL_NAME,you,nullptr,nullptr,nullptr,me);
    EDITOR_ONE_WITH_ONE_END
}
DIRECT(QUERY_SpectrogramS_window){QUERY_ONE_FOR_INTEGER(SpectrogramS)
    integer result=my windowSamples;
QUERY_ONE_FOR_INTEGER_END(U" samples (0 = unknown)")}
DIRECT(QUERY_SpectrogramS_ms){QUERY_ONE_FOR_REAL(SpectrogramS)
    double result=my windowSamples?1000.0*my windowSamples/my sourceSamplingFrequency:undefined;
QUERY_ONE_FOR_REAL_END(U" ms")}
DIRECT(QUERY_SpectrogramS_fs){QUERY_ONE_FOR_REAL(SpectrogramS)
    double result=my sourceSamplingFrequency;
QUERY_ONE_FOR_REAL_END(U" Hz")}
DIRECT(QUERY_SpectrogramS_hop){QUERY_ONE_FOR_REAL(SpectrogramS)
    double result=my hopSamples;
QUERY_ONE_FOR_REAL_END(U" samples")}
DIRECT(QUERY_SpectrogramS_fft){QUERY_ONE_FOR_INTEGER(SpectrogramS)
    integer result=my fftSamples;
QUERY_ONE_FOR_INTEGER_END(U" samples (0 = unknown)")}
DIRECT(QUERY_SpectrogramS_method){QUERY_ONE_FOR_STRING(SpectrogramS)
    conststring32 result=my fftSamples==0?U"unknown":(my fftSamples&(my fftSamples-1))==0?U"FFT":U"direct DFT";
QUERY_ONE_FOR_STRING_END}
void praat_SpectrogramS_init(){
    Thing_recognizeClassesByName(classSpectrogramS);
    praat_addAction1(classSound,0,U"To SpectrogramS...",U"To Spectrogram...",0,CONVERT_Sound_SpectrogramS);
    praat_addAction1(classSpectrogram,0,U"To SpectrogramS...",nullptr,0,CONVERT_Spectrogram_SpectrogramS);
    praat_addAction1(classSpectrogramS,0,U"To Spectrogram",nullptr,0,CONVERT_SpectrogramS_Spectrogram);
    praat_addAction1(classSpectrogramS,1,U"View & Edit || View",nullptr,GuiMenu_ATTRACTIVE,EDIT_SpectrogramS);
    praat_addAction2(classSound,1,classSpectrogramS,1,U"View & Edit",nullptr,GuiMenu_ATTRACTIVE,EDIT_Sound_SpectrogramS);
    praat_addAction1(classSpectrogramS,1,U"Get analysis length in samples",nullptr,0,QUERY_SpectrogramS_window);
    praat_addAction1(classSpectrogramS,1,U"Get analysis length in milliseconds",nullptr,0,QUERY_SpectrogramS_ms);
    praat_addAction1(classSpectrogramS,1,U"Get source sampling frequency",nullptr,0,QUERY_SpectrogramS_fs);
    praat_addAction1(classSpectrogramS,1,U"Get time step in samples",nullptr,0,QUERY_SpectrogramS_hop);
    praat_addAction1(classSpectrogramS,1,U"Get transform method",nullptr,0,QUERY_SpectrogramS_method);
    praat_addAction1(classSpectrogramS,1,U"Get transform size in samples || Get FFT size in samples",nullptr,0,QUERY_SpectrogramS_fft);
}
