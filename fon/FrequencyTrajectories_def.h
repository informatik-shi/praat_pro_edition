// Praat Custom. GPL-3.0-or-later.
#define ooSTRUCT FrequencyTrajectories
oo_DEFINE_CLASS (FrequencyTrajectories, Function)
    oo_INTEGER (numberOfTracks)
    oo_STRING_VECTOR (trackNames, numberOfTracks)
    oo_COLLECTION_OF (OrderedOf, tracks, RealTier, 0)
    #if oo_DECLARING
        void v_shiftX (double xfrom, double xto) override;
        void v_scaleX (double a, double b, double c, double d) override;
    #endif
oo_END_CLASS (FrequencyTrajectories)
#undef ooSTRUCT
