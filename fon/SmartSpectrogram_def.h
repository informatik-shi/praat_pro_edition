// Praat Pro Edition. GPL-3.0-or-later.
#define ooSTRUCT SmartSpectrogram
oo_DEFINE_CLASS (SmartSpectrogram, Function)
    oo_INTEGER (numberOfFrames)
    oo_INTEGER (numberOfBins)
    oo_DOUBLE (firstFrequencyHz)
    oo_DOUBLE (frequencyStepHz)
    oo_VEC (times, numberOfFrames)
    oo_MAT (valuesDb, numberOfBins, numberOfFrames)
    oo_OBJECT (Pitch, 1, pitch)
    oo_OBJECT (Intensity, 2, intensity)
    oo_OBJECT (FrequencyTrajectories, 0, trajectories)
    #if oo_DECLARING
        void v1_info () override;
        int v_domainQuantity () const override { return MelderQuantity_TIME_SECONDS; }
        void v_shiftX (double xfrom, double xto) override;
        void v_scaleX (double a, double b, double c, double d) override;
    #endif
oo_END_CLASS (SmartSpectrogram)
#undef ooSTRUCT
