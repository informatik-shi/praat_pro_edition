// Praat Pro Edition. GPL-3.0-or-later.
#define ooSTRUCT SpectrogramS
oo_DEFINE_CLASS(SpectrogramS,Spectrogram)
    oo_DOUBLE(sourceSamplingFrequency)
    oo_INTEGER(windowSamples)
    oo_DOUBLE(hopSamples)
    oo_INTEGER(fftSamples)
    oo_ENUM(kSound_to_Spectrogram_windowShape,windowShape)
    oo_BOOLEAN(computedFromSound)
    #if oo_READING
        SpectrogramS_validate(this);
    #endif
    #if oo_DECLARING
        void v1_info() override;
    #endif
oo_END_CLASS(SpectrogramS)
#undef ooSTRUCT
