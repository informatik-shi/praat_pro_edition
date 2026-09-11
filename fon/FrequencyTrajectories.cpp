// Praat Custom. GPL-3.0-or-later.
#include "FrequencyTrajectories.h"
#include <vector>
#include <string>
#include <cmath>
#include <algorithm>
#include <set>
#include "oo_DESTROY.h"
#include "FrequencyTrajectories_def.h"
#include "oo_COPY.h"
#include "FrequencyTrajectories_def.h"
#include "oo_EQUAL.h"
#include "FrequencyTrajectories_def.h"
#include "oo_CAN_WRITE_AS_ENCODING.h"
#include "FrequencyTrajectories_def.h"
#include "oo_WRITE_TEXT.h"
#include "FrequencyTrajectories_def.h"
#include "oo_READ_TEXT.h"
#include "FrequencyTrajectories_def.h"
#include "oo_WRITE_BINARY.h"
#include "FrequencyTrajectories_def.h"
#include "oo_READ_BINARY.h"
#include "FrequencyTrajectories_def.h"
#include "oo_DESCRIPTION.h"
#include "FrequencyTrajectories_def.h"

Thing_implement (FrequencyTrajectories, Function, 0);
void structFrequencyTrajectories::v_shiftX(double a,double b) {
    FrequencyTrajectories_Parent::v_shiftX(a,b);
    for(integer i=1;i<=tracks.size;++i)tracks.at[i]->v_shiftX(a,b);
}
void structFrequencyTrajectories::v_scaleX(double a,double b,double c,double d) {
    FrequencyTrajectories_Parent::v_scaleX(a,b,c,d);
    for(integer i=1;i<=tracks.size;++i)tracks.at[i]->v_scaleX(a,b,c,d);
}
autoFrequencyTrajectories FrequencyTrajectories_create(double start,double end,integer count) {
    Melder_require(std::isfinite(start)&&std::isfinite(end)&&end>start,U"End time must exceed start time.");
    Melder_require(count>=1&&count<=256,U"Use 1 to 256 trajectories.");
    autoFrequencyTrajectories me=Thing_new(FrequencyTrajectories);
    my xmin=start;my xmax=end;my numberOfTracks=count;my trackNames=autoSTRVEC(count);
    for(integer i=1;i<=count;++i){my tracks.addItem_move(RealTier_create(start,end));my trackNames[i]=Melder_dup(Melder_cat(U"F",i));}
    return me;
}
void FrequencyTrajectories_validate(FrequencyTrajectories me) {
    Melder_require(my numberOfTracks>=1&&my numberOfTracks<=256&&my tracks.size==my numberOfTracks,U"Invalid trajectory count.");
    Melder_require(std::isfinite(my xmin)&&std::isfinite(my xmax)&&my xmax>my xmin,U"Invalid time domain.");
    std::set<std::u32string> names;
    for(integer i=1;i<=my numberOfTracks;++i) {
        Melder_require(my trackNames[i]&&my trackNames[i][0]&&names.insert(my trackNames[i].get()).second,U"Trajectory names must be nonempty and unique.");
        double previous=-INFINITY;
        for(integer p=1;p<=my tracks.at[i]->points.size;++p){auto point=my tracks.at[i]->points.at[p];
            Melder_require(std::isfinite(point->number)&&point->number>=my xmin&&point->number<=my xmax&&point->number>previous&&std::isfinite(point->value)&&point->value>=0,U"Invalid point in trajectory ",i,U".");previous=point->number;
        }
    }
}
void FrequencyTrajectories_addPoint(FrequencyTrajectories me,integer track,double time,double hz) {
    Melder_require(track>=1&&track<=my tracks.size,U"Trajectory index out of range.");
    Melder_require(std::isfinite(time)&&time>=my xmin&&time<=my xmax,U"Point time is outside the object time domain.");
    Melder_require(std::isfinite(hz)&&hz>=0,U"Frequency must be finite and nonnegative.");
    auto tier=my tracks.at[track];
    integer nearest=AnyTier_timeToNearestIndex(tier->asAnyTier(),time);
    Melder_require(!nearest||tier->points.at[nearest]->number!=time,U"Duplicate time in this trajectory.");
    RealTier_addPoint(tier,time,hz);
}
// CSV/TSV records, including quoted names and doubled quotes. Newlines in names are rejected.
static std::vector<std::u32string> fields(const std::u32string &line,char32 delimiter) {
    std::vector<std::u32string> result;size_t p=0;
    for(;;){std::u32string field;
        if(p<line.size()&&line[p]==U'"'){
            ++p;bool closed=false;
            while(p<line.size()){char32 c=line[p++];if(c==U'"'){if(p<line.size()&&line[p]==U'"'){++p;field+=c;}else{closed=true;break;}}else field+=c;}
            Melder_require(closed,U"Unclosed CSV quote.");
            Melder_require(p==line.size()||line[p]==delimiter,U"Unexpected text after quoted field.");
        }else{while(p<line.size()&&line[p]!=delimiter){Melder_require(line[p]!=U'"',U"Quote inside unquoted field.");field+=line[p++];}}
        result.push_back(field);if(p==line.size())break;++p;
    }return result;
}
autoFrequencyTrajectories FrequencyTrajectories_read(MelderFile file) {
    auto text=MelderFile_readText(file);std::u32string source=text.get();
    struct Row{double time,hz;size_t track;};std::vector<Row> rows;std::vector<std::u32string> names;
    bool header=false;char32 delimiter=U',';size_t offset=0;integer lineNumber=0;
    double start=INFINITY,end=-INFINITY;
    while(offset<source.size()){
        size_t next=source.find(U'\n',offset);if(next==std::u32string::npos)next=source.size();
        auto line=source.substr(offset,next-offset);offset=next+1;++lineNumber;
        if(!line.empty()&&line.back()==U'\r')line.pop_back();
        if(line.empty())continue;
        try{
            if(!header){delimiter=line.find(U'\t')!=std::u32string::npos?U'\t':U',';auto f=fields(line,delimiter);
                Melder_require(f.size()==3&&(f[0]==U"time_s"||f[0]==U"time")&&f[1]==U"track"&&f[2]==U"frequency_hz",U"Expected header: time_s,track,frequency_hz (or tab-separated).");header=true;continue;}
            auto f=fields(line,delimiter);Melder_require(f.size()==3,U"Expected exactly three fields.");
            Melder_require(Melder_isStringNumeric(f[0].c_str())&&Melder_isStringNumeric(f[2].c_str()),U"Invalid numeric value; use decimal dots.");
            double time=Melder_atof(f[0].c_str()),hz=Melder_atof(f[2].c_str());
            Melder_require(std::isfinite(time)&&std::isfinite(hz)&&hz>=0,U"Time/frequency must be finite; frequency cannot be negative.");
            Melder_require(!f[1].empty()&&f[1].size()<=256,U"Track name must contain 1–256 characters.");
            auto it=std::find(names.begin(),names.end(),f[1]);size_t index=it-names.begin();
            if(it==names.end()){Melder_require(names.size()<256,U"Too many tracks.");names.push_back(f[1]);}
            rows.push_back({time,hz,index});start=std::min(start,time);end=std::max(end,time);
        }catch(MelderError){Melder_throw(U"Trajectory file, line ",lineNumber,U".");}
    }
    Melder_require(!rows.empty(),U"Trajectory file contains no points.");
    if(end==start){start-=0.5;end+=0.5;}
    auto me=FrequencyTrajectories_create(start,end,(integer)names.size());
    for(size_t i=0;i<names.size();++i)my trackNames[i+1]=Melder_dup(names[i].c_str());
    std::sort(rows.begin(),rows.end(),[](const Row&a,const Row&b){return a.time<b.time;});
    for(auto &row:rows)FrequencyTrajectories_addPoint(me.get(),row.track+1,row.time,row.hz);
    return me;
}
void FrequencyTrajectories_write(FrequencyTrajectories me,MelderFile file,bool tsv) {
    FrequencyTrajectories_validate(me);
    char32 delimiter=tsv?U'\t':U',';std::u32string output=U"time_s";output+=delimiter;output+=U"track";output+=delimiter;output+=U"frequency_hz\n";
    for(integer i=1;i<=my numberOfTracks;++i){std::u32string name=U"\"";
        for(const char32 *p=my trackNames[i].get();*p;++p){Melder_require(*p!=U'\r'&&*p!=U'\n',U"Track name contains a newline.");name+=*p;if(*p==U'"')name+=*p;}name+=U'"';
        for(integer j=1;j<=my tracks.at[i]->points.size;++j){auto point=my tracks.at[i]->points.at[j];output+=Melder_double(point->number);output+=delimiter;output+=name;output+=delimiter;output+=Melder_double(point->value);output+=U'\n';}
    }
    MelderFile_writeText_e(file,output.c_str(),kMelder_textOutputEncoding::UTF8);
}
