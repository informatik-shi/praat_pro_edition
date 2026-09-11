// Praat Custom. GPL-3.0-or-later.
#include "praat.h"
#include "praatM.h"
#include "FrequencyTrajectories.h"
#include "FrequencyTrajectoriesEditor.h"
#include "Formant.h"
#include <cmath>
FORM_READ(READ_FrequencyTrajectories,U"Read frequency trajectories from CSV or TSV",nullptr,true){
    READ_ONE
        autoFrequencyTrajectories result=FrequencyTrajectories_read(file);
    READ_ONE_END
}
FORM_SAVE(SAVE_FrequencyTrajectories_csv,U"Save trajectories as CSV",nullptr,U"csv"){
    SAVE_ONE(FrequencyTrajectories,U"save trajectories as CSV")
        FrequencyTrajectories_write(me,file,false);
    SAVE_ONE_END
}
FORM_SAVE(SAVE_FrequencyTrajectories_tsv,U"Save trajectories as TSV",nullptr,U"tsv"){
    SAVE_ONE(FrequencyTrajectories,U"save trajectories as TSV")
        FrequencyTrajectories_write(me,file,true);
    SAVE_ONE_END
}
FORM(NEW_FrequencyTrajectories,U"Create FrequencyTrajectories",nullptr){
    WORD(name,U"Name",U"tracks")
    REAL(start,U"Start time (s)",U"0")
    REAL(end,U"End time (s)",U"1")
    NATURAL(count,U"Number of trajectories",U"3")
    OK
DO
    CREATE_ONE
        autoFrequencyTrajectories result=FrequencyTrajectories_create(start,end,count);
    CREATE_ONE_END(name)
}
DIRECT(EDIT_FrequencyTrajectories){
    EDITOR_ONE_WITH_ONE(a,FrequencyTrajectories,Sound)
        autoFrequencyTrajectoriesEditor editor=FrequencyTrajectoriesEditor_create(ID_AND_FULL_NAME,me,you);
    EDITOR_ONE_WITH_ONE_END
}
FORM(MODIFY_FrequencyTrajectories_add,U"Add trajectory point",nullptr){
    NATURAL(track,U"Trajectory number",U"1")
    REAL(time,U"Time (s)",U"0.1")
    REAL(hz,U"Frequency (Hz)",U"500")
    OK
DO
    MODIFY_EACH(FrequencyTrajectories)
        FrequencyTrajectories_addPoint(me,track,time,hz);
    MODIFY_EACH_END
}
FORM(QUERY_FrequencyTrajectories_value,U"Get trajectory value at time",nullptr){
    NATURAL(track,U"Trajectory number",U"1")
    REAL(time,U"Time (s)",U"0.1")
    OK
DO
    QUERY_ONE_FOR_REAL(FrequencyTrajectories)
        Melder_require(track<=my tracks.size,U"Trajectory index out of range.");
        double result=RealTier_getValueAtTime(my tracks.at[track],time);
    QUERY_ONE_FOR_REAL_END(U" Hz")
}
DIRECT(CONVERT_Formant_to_FrequencyTrajectories){
    CONVERT_EACH_TO_ONE(Formant)
        integer count=0;for(integer i=1;i<=my nx;++i)count=std::max(count,my frames[i].numberOfFormants);
        autoFrequencyTrajectories result=FrequencyTrajectories_create(my xmin,my xmax,count);
        for(integer i=1;i<=my nx;++i){double time=my x1+(i-1)*my dx;
            for(integer j=1;j<=my frames[i].numberOfFormants;++j){double hz=my frames[i].formant[j].frequency;
                if(std::isfinite(hz)&&hz>=0)FrequencyTrajectories_addPoint(result.get(),j,time,hz);}}
    CONVERT_EACH_TO_ONE_END(U"tracks")
}
void praat_FrequencyTrajectories_init(){
    Thing_recognizeClassesByName(classFrequencyTrajectories);
    praat_addMenuCommand(U"Objects",U"Open",U"Read frequency trajectories from CSV/TSV...",nullptr,0,READ_FrequencyTrajectories);
    praat_addMenuCommand(U"Objects",U"New",U"Create FrequencyTrajectories...",nullptr,0,NEW_FrequencyTrajectories);
    praat_addAction1(classFrequencyTrajectories,1,U"View & Edit",nullptr,GuiMenu_ATTRACTIVE,EDIT_FrequencyTrajectories);
    praat_addAction2(classFrequencyTrajectories,1,classSound,1,U"View & Edit",nullptr,GuiMenu_ATTRACTIVE,EDIT_FrequencyTrajectories);
    praat_addAction1(classFrequencyTrajectories,1,U"Save as CSV file...",nullptr,0,SAVE_FrequencyTrajectories_csv);
    praat_addAction1(classFrequencyTrajectories,1,U"Save as TSV file...",nullptr,0,SAVE_FrequencyTrajectories_tsv);
    praat_addAction1(classFrequencyTrajectories,0,U"Add point...",nullptr,0,MODIFY_FrequencyTrajectories_add);
    praat_addAction1(classFrequencyTrajectories,1,U"Get value at time...",nullptr,0,QUERY_FrequencyTrajectories_value);
    praat_addAction1(classFormant,0,U"To FrequencyTrajectories",nullptr,0,CONVERT_Formant_to_FrequencyTrajectories);
}
