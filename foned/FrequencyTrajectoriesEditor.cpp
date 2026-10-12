// Praat Custom. GPL-3.0-or-later.
#include "FrequencyTrajectoriesEditor.h"
#include "Sound_and_Spectrogram.h"
#include "EditorM.h"
#include <algorithm>
#include <cmath>
Thing_implement(FrequencyTrajectoriesArea,RealTierArea,0);
Thing_implement(FrequencyTrajectoriesEditor,FunctionEditor,0);

void structFrequencyTrajectoriesEditor::v1_dataChanged(Editor sender) {
    auto object=static_cast<FrequencyTrajectories>(data());
    FrequencyTrajectories_validate(object);
    auto area=trajectoriesArea().get();area->trajectories=object;
    if(soundArea()){soundArea()->functionChanged(nullptr);area->backgroundSound=soundArea()->sound();}
    area->backgroundSmart=backgroundSmartCopy.get();
    area->selectedTrack=Melder_clipped(1_integer,area->selectedTrack,object->numberOfTracks);
    area->functionChanged(object->tracks.at[area->selectedTrack]);
    // Parent redraws immediately: rebind tiers (including after Undo) before it paints.
    FrequencyTrajectoriesEditor_Parent::v1_dataChanged(sender);
}
void structFrequencyTrajectoriesEditor::v_drawLegends() {
    auto area=trajectoriesArea().get();
    FunctionArea_drawLegend(area,Melder_cat(U"##",area->selectedTrack,U": ",area->trajectories->trackNames[area->selectedTrack].get(),U" — drag points; Trajectory menu selects track"),Melder_RED);
    if(soundArea())FunctionArea_drawLegend(soundArea().get(),U"Sound (reference copy); Play selection to listen",Melder_BLACK);
}
autoFrequencyTrajectoriesEditor FrequencyTrajectoriesEditor_create(conststring32 title,FrequencyTrajectories tracks,Sound sound,SmartSpectrogram smart) {
    FrequencyTrajectories_validate(tracks);
    if(sound)Melder_require(tracks->xmin>=sound->xmin&&tracks->xmax<=sound->xmax,U"Trajectory time domain must lie within the selected Sound; times are absolute seconds, no automatic offset is applied.");
    if(smart){
        SmartSpectrogram_validate(smart);
        Melder_require(smart->xmax>=tracks->xmin&&smart->xmin<=tracks->xmax,U"SmartSpectrogram and FrequencyTrajectories time domains must overlap.");
    }
    auto me=Thing_new(FrequencyTrajectoriesEditor);
    my trajectoriesArea()=FrequencyTrajectoriesArea_create(true,nullptr,me.get());
    if(smart){my backgroundSmartCopy=Data_copy(smart);my trajectoriesArea()->backgroundSmart=my backgroundSmartCopy.get();}
    if(sound)my soundArea()=SoundArea_create(false,sound,me.get());
    FunctionEditor_init(me.get(),title,tracks);
    return me;
}
static void paintSmartBackground(FrequencyTrajectoriesArea me) {
    SmartSpectrogram s=my backgroundSmart;
    const double left=std::max(my startWindow(),s->xmin),right=std::min(my endWindow(),s->xmax);
    const double fmin=s->firstFrequencyHz;
    const double fmax=std::min(my ceiling,fmin+(s->numberOfBins-1)*s->frequencyStepHz);
    if(right<=left||fmax<=fmin)return;
    const integer columns=1000,rows=std::min(s->numberOfBins,600_integer);
    auto image=raw_MAT(rows,columns);
    double peak=-1e300;
    for(integer col=1,frame=1;col<=columns;++col){
        const double time=left+(col-0.5)*(right-left)/columns;
        while(frame<s->numberOfFrames-1&&s->times[frame+1]<time)++frame;
        const integer next=std::min(s->numberOfFrames,frame+1);
        const double blend=next==frame?0.0:Melder_clipped(0.0,(time-s->times[frame])/(s->times[next]-s->times[frame]),1.0);
        for(integer row=1;row<=rows;++row){
            const double binPosition=1.0+(row-1.0)*(fmax-fmin)/(s->frequencyStepHz*(rows-1.0));
            const integer bin1=Melder_clipped(1_integer,(integer)std::floor(binPosition),s->numberOfBins);
            const integer bin2=std::min(s->numberOfBins,bin1+1);
            const double frequencyBlend=binPosition-bin1;
            const double low=s->valuesDb[bin1][frame]*(1-blend)+s->valuesDb[bin1][next]*blend;
            const double high=s->valuesDb[bin2][frame]*(1-blend)+s->valuesDb[bin2][next]*blend;
            const double value=low*(1-frequencyBlend)+high*frequencyBlend;
            image[row][col]=value;
            peak=std::max(peak,value);
        }
    }
    Graphics_image(my graphics(),image.all(),left,right,fmin,fmax,peak-my dynamicRange,peak);
}
void structFrequencyTrajectoriesArea::v_drawInside() {
    if(!backgroundSmart&&backgroundSound&&(cachedStart!=startWindow()||cachedEnd!=endWindow())) {
        cachedStart=startWindow();cachedEnd=endWindow();spectrogram.reset();
        try {
            // Like SoundAnalysisArea: progress callbacks must not re-enter painting.
            autoMelderProgressOff progress;
            auto part=Sound_extractPart(backgroundSound,std::max(backgroundSound->xmin,startWindow()-windowLength),std::min(backgroundSound->xmax,endWindow()+windowLength),kSound_windowShape::RECTANGULAR,1.0,true);
            spectrogram=Sound_to_Spectrogram_e(part.get(),windowLength,std::min(ceiling,0.5/backgroundSound->dx),std::max(0.002,(endWindow()-startWindow())/1500.0),std::max(10.0,ceiling/600.0),kSound_to_Spectrogram_windowShape::GAUSSIAN,8.0,8.0);
        }catch(MelderError){Melder_clearError();}
    }
    FunctionArea_setViewport(this);
    Graphics_setWindow(graphics(),startWindow(),endWindow(),ymin,ymax);
    if(backgroundSmart)paintSmartBackground(this);
    else if(spectrogram)Spectrogram_paintInside(spectrogram.get(),graphics(),startWindow(),endWindow(),0,ceiling,0,true,dynamicRange,6.0,0,kSpectrogram_colourMap::GREY,false);
    // Non-selected tracks use distinct colours; the active tier uses Praat's editable-point styling.
    const MelderColour colours[]={Melder_BLUE,Melder_GREEN,Melder_MAGENTA,Melder_CYAN};
    for(integer i=1;i<=trajectories->tracks.size;++i)if(i!=selectedTrack){
        auto tier=trajectories->tracks.at[i];Graphics_setColour(graphics(),colours[(i-1)%4]);Graphics_setLineWidth(graphics(),2.0);
        integer first=std::max(1_integer,AnyTier_timeToLowIndex(tier->asAnyTier(),startWindow()));
        integer last=std::min(tier->points.size,AnyTier_timeToHighIndex(tier->asAnyTier(),endWindow()));
        for(integer p=first;p<=last;++p){auto a=tier->points.at[p];if(a->number>=startWindow()&&a->number<=endWindow())Graphics_fillCircle_mm(graphics(),a->number,a->value,2.0);
            if(p<last){auto b=tier->points.at[p+1];Graphics_line(graphics(),a->number,a->value,b->number,b->value);}}
    }
    FrequencyTrajectoriesArea_Parent::v_drawInside();
    if(backgroundSound&&!backgroundSmart&&!spectrogram){Graphics_setColour(graphics(),Melder_RED);Graphics_setTextAlignment(graphics(),Graphics_LEFT,Graphics_TOP);Graphics_text(graphics(),startWindow(),ceiling,U"Spectrogram unavailable at this window length / interval");}
}
bool structFrequencyTrajectoriesArea::v_mouse(GuiDrawingArea_MouseEvent event,double x,double y) {
    if(event->isClick()){
        viewRealTierAsWorldByWorld();double best=1.5;integer hit=selectedTrack;
        for(integer i=1;i<=trajectories->tracks.size;++i){auto tier=trajectories->tracks.at[i];integer p=AnyTier_timeToNearestIndexInTimeWindow(tier->asAnyTier(),x,startWindow(),endWindow());
            if(p){auto point=tier->points.at[p];double distance=Graphics_distanceWCtoMM(graphics(),x,y*ceiling,point->number,point->value);if(distance<best){best=distance;hit=i;}}}
        if(hit!=selectedTrack){selectedTrack=hit;functionChanged(trajectories->tracks.at[hit]);}
    }
    return FrequencyTrajectoriesArea_Parent::v_mouse(event,x,y);
}
static void selectTrack(FrequencyTrajectoriesArea me,integer track) {
    Melder_require(track>=1&&track<=my trajectories->numberOfTracks,U"Trajectory index out of range.");
    my selectedTrack=track;my functionChanged(my trajectories->tracks.at[track]);FunctionEditor_redraw(my functionEditor());
}
static void menu_select(FrequencyTrajectoriesArea me,EDITOR_ARGS) {
    EDITOR_FORM(U"Select trajectory",nullptr)
        NATURAL(track,U"Trajectory number",U"1")
    EDITOR_OK
        SET_INTEGER(track,my selectedTrack)
    EDITOR_DO
        selectTrack(me,track);
    EDITOR_END
}
static void menu_next(FrequencyTrajectoriesArea me,EDITOR_ARGS){selectTrack(me,my selectedTrack%my trajectories->numberOfTracks+1);}
static void menu_add(FrequencyTrajectoriesArea me,EDITOR_ARGS){
    FunctionArea_save(me,U"Add trajectory point");
    FrequencyTrajectories_addPoint(my trajectories,my selectedTrack,0.5*(my startSelection()+my endSelection()),my ycursor);
    FunctionArea_broadcastDataChanged(me);
}
static void menu_remove(FrequencyTrajectoriesArea me,EDITOR_ARGS){FunctionArea_save(me,U"Remove trajectory point(s)");RealTierArea_removePoints(me);FunctionArea_broadcastDataChanged(me);}
static void menu_addAt(FrequencyTrajectoriesArea me,EDITOR_ARGS){
    EDITOR_FORM(U"Add trajectory point",nullptr)
        REAL(time,U"Time (s)",U"0.1")
        REAL(frequency,U"Frequency (Hz)",U"500")
    EDITOR_OK
        SET_REAL(time,0.5*(my startSelection()+my endSelection()))
        SET_REAL(frequency,my ycursor)
    EDITOR_DO
        FunctionArea_save(me,U"Add trajectory point");FrequencyTrajectories_addPoint(my trajectories,my selectedTrack,time,frequency);FunctionArea_broadcastDataChanged(me);
    EDITOR_END
}
static void menu_settings(FrequencyTrajectoriesArea me,EDITOR_ARGS){
    EDITOR_FORM(U"Trajectory spectrogram settings",nullptr)
        POSITIVE(maximumFrequency,U"Maximum frequency (Hz)",U"5500")
        POSITIVE(analysisWindow,U"Window length (s)",U"0.005")
        POSITIVE(range,U"Dynamic range (dB)",U"50")
    EDITOR_OK
        SET_REAL(maximumFrequency,my ceiling)
        SET_REAL(analysisWindow,my windowLength)
        SET_REAL(range,my dynamicRange)
    EDITOR_DO
        Melder_require(maximumFrequency<=100000&&analysisWindow<=1.0&&range<=200,U"Use frequency <= 100000 Hz, window <= 1 s and range <= 200 dB.");
        my ceiling=maximumFrequency;my windowLength=analysisWindow;my dynamicRange=range;my cachedStart=undefined;my v_updateScaling();FunctionEditor_redraw(my functionEditor());
    EDITOR_END
}
void structFrequencyTrajectoriesArea::v_createMenus(){
    auto menu=Editor_addMenu(functionEditor(),U"Trajectory",0);
    FunctionAreaMenu_addCommand(menu,U"Select trajectory...",0,menu_select,this);
    FunctionAreaMenu_addCommand(menu,U"Next trajectory",'N',menu_next,this);
    FunctionAreaMenu_addCommand(menu,U"Spectrogram settings...",0,menu_settings,this);
    FunctionAreaMenu_addCommand(menu,U"Add point at cursor",'T',menu_add,this);
    FunctionAreaMenu_addCommand(menu,U"Add point at...",0,menu_addAt,this);
    FunctionAreaMenu_addCommand(menu,U"Remove point(s)",GuiMenu_COMMAND_EXTRA|'T',menu_remove,this);
}
