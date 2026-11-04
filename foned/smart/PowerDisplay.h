// Display sampling only; the original SmartSpectrogram dB data stay unchanged.
// GPL-3.0-or-later.
#ifndef PRAAT_SMART_POWER_DISPLAY_H
#define PRAAT_SMART_POWER_DISPLAY_H
#include <algorithm>
#include <cmath>
#include <cstdint>
namespace SmartPowerDisplay {
inline double blend(double a,double b,double weight) {
    if(weight<=0.0)return a;
    if(weight>=1.0)return b;
    const double maximum=std::max(a,b);
    // Stable linear interpolation of power, expressed back in dB.
    return maximum+10.0*std::log10((1.0-weight)*std::pow(10.0,(a-maximum)/10.0)+
        weight*std::pow(10.0,(b-maximum)/10.0));
}
template<class Read> double bandPeak(double first,double step,std::int64_t bins,
    double low,double high,Read read) {
    auto sample=[&](double hz) {
        const double position=std::max(0.0,std::min(double(bins-1),(hz-first)/step));
        const auto left=(std::int64_t)std::floor(position);
        return blend(read(left+1),read(std::min(bins,left+2)),position-left);
    };
    double peak=std::max(sample(low),sample(high));
    // Retain narrow harmonics falling between display row centres.
    const auto firstBin=std::max<std::int64_t>(0,(std::int64_t)std::ceil((low-first)/step));
    const auto lastBin=std::min<std::int64_t>(bins-1,(std::int64_t)std::floor((high-first)/step));
    for(auto bin=firstBin;bin<=lastBin;++bin)peak=std::max(peak,read(bin+1));
    return peak;
}
}
#endif
