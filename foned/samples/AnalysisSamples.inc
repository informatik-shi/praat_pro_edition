// Included by SoundAnalysisArea.cpp. Exact physical sample counts, with live millisecond preview.
static void samplesLengthPreview(UiForm form,GuiTextEvent) {
    auto me=static_cast<structSoundAnalysisArea*>(form->command->sender);
    if(!my soundOrLongSound())return;
    const double fs=1.0/my soundOrLongSound()->dx;
    auto sampleText=GuiText_getString(form->field[2]->text);
    auto hopText=GuiText_getString(form->field[3]->text);
    auto transformText=GuiText_getString(form->field[4]->text);
    const double n=Melder_atof(sampleText.get()),hop=Melder_atof(hopText.get());
    double transform=Melder_atof(transformText.get());
    if(transform==0)transform=n;
    if(form->field[5]->label)GuiLabel_setText(form->field[5]->label,Melder_cat(U"Sound sampling frequency: ",Melder_double(fs),U" Hz"));
    if(form->field[6]->label)GuiLabel_setText(form->field[6]->label,
        std::isfinite(n)&&n>=2&&n==std::floor(n)?Melder_cat(U"Analysis duration: ",Melder_fixed(1000*n/fs,6),U" ms (",Melder_double(n),U" samples)"):U"Analysis duration: enter a whole number of at least 2 samples");
    if(form->field[7]->label)GuiLabel_setText(form->field[7]->label,
        std::isfinite(hop)&&hop>=1&&hop==std::floor(hop)?Melder_cat(U"Time step: ",Melder_fixed(1000*hop/fs,6),U" ms"):U"Time step: enter a positive whole sample count");
    if(form->field[8]->label){
        if(std::isfinite(transform)&&transform>=2&&transform<=1048576&&transform>=n&&transform==std::floor(transform)){
            const integer m=(integer)transform;
            GuiLabel_setText(form->field[8]->label,Melder_cat(U"Transform: ",(m&(m-1))==0?U"FFT":U"direct DFT",U" (",m,U" samples); step: ",Melder_fixed(fs/m,6),U" Hz"));
        }else GuiLabel_setText(form->field[8]->label,U"Transform: enter 0 or a whole length at least as large as the analysis window");
    }
}
static void menu_cb_spectrogramSampleSettings(SoundAnalysisArea me,EDITOR_ARGS) {
    EDITOR_FORM(U"Spectrogram analysis in samples",nullptr)
        BOOLEAN(useSamples,U"Use exact sample counts",false)
        NATURAL(windowSamples,U"Analysis length (samples)",U"256")
        NATURAL(hopSamples,U"Time step (samples)",U"64")
        NATURAL0(fftSamples,U"Transform size (samples; 0 = analysis length)",U"0")
        MUTABLE_COMMENT(sampleRateNote,U"")
        MUTABLE_COMMENT(durationNote,U"")
        MUTABLE_COMMENT(hopNote,U"")
        MUTABLE_COMMENT(transformNote,U"")
        COMMENT(U"Power of two: FFT. Other lengths: direct DFT, without automatic padding.")
        COMMENT(U"The count is the full physical window, including for Gaussian.")
        COMMENT(U"Window shape is selected in Advanced spectrogram settings.")
        COMMENT(U"Applies to computed Sound analysis; imported spectral layers are unchanged.")
    EDITOR_OK
        Melder_require(my soundOrLongSound(),U"Open a Sound or LongSound to set analysis length in samples.");
        SET_BOOLEAN(useSamples,my instancePref_spectrogram_useSamples())
        SET_INTEGER(windowSamples,my instancePref_spectrogram_windowSamples())
        SET_INTEGER(hopSamples,my instancePref_spectrogram_hopSamples())
        SET_INTEGER(fftSamples,my instancePref_spectrogram_fftSamples())
        if(cmd->d_uiform->field[2]->text){
            GuiText_setChangedCallback(cmd->d_uiform->field[2]->text,samplesLengthPreview,cmd->d_uiform.get());
            GuiText_setChangedCallback(cmd->d_uiform->field[3]->text,samplesLengthPreview,cmd->d_uiform.get());
            GuiText_setChangedCallback(cmd->d_uiform->field[4]->text,samplesLengthPreview,cmd->d_uiform.get());
            samplesLengthPreview(cmd->d_uiform.get(),nullptr);
        }
    EDITOR_DO
        if(useSamples){
            Melder_require(windowSamples>=2&&windowSamples<=1048576&&windowSamples<=my soundOrLongSound()->nx,U"Analysis length must be 2 to 1048576 samples and fit inside the Sound.");
            Melder_require(hopSamples<=2147483647,U"Time step is too large.");
            Melder_require(fftSamples==0||(fftSamples>=windowSamples&&fftSamples<=1048576),U"Transform size must be 0 (analysis length), or at least as large as the analysis window (maximum 1048576).");
        }
        my setInstancePref_spectrogram_useSamples(useSamples);
        my setInstancePref_spectrogram_windowSamples(windowSamples);
        my setInstancePref_spectrogram_hopSamples(hopSamples);
        my setInstancePref_spectrogram_fftSamples(fftSamples);
        if(useSamples)my setInstancePref_spectrogram_windowLength(windowSamples*my soundOrLongSound()->dx/
            (my instancePref_spectrogram_windowShape()==kSound_to_Spectrogram_windowShape::GAUSSIAN?2.0:1.0));
        my d_spectrogramList.reset();
        FunctionEditor_redraw(my functionEditor());
    EDITOR_END
}
