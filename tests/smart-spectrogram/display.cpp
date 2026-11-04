#include "../../foned/smart/PowerDisplay.h"
#include <cassert>
#include <iostream>
#include <vector>
int main() {
    using namespace SmartPowerDisplay;
    assert(blend(20,-160,0)==20&&blend(20,-160,1)==-160);
    assert(blend(20,20,.5)==20);
    // A peak fading towards silence loses ~3 dB at half power, not 90 dB.
    assert(std::abs(blend(20,-160,.5)-(20-10*std::log10(2)))<1e-10);
    assert(std::isfinite(blend(0,-30000,.5)));
    const std::vector<double> spectrum{-160,-160,80,-160,-160};
    auto read=[&](std::int64_t bin){return spectrum.at((size_t)bin-1);};
    assert(bandPeak(0,20,5,20,60,read)==80);
    assert(bandPeak(0,20,5,0,10,read)==-160);
    assert(bandPeak(0,20,5,40,40,read)==80);
    assert(bandPeak(0,20,5,0,80,read)==80);
    // Flat spectra do not become stronger when display rows are merged.
    assert(bandPeak(0,20,100,0,1900,[](std::int64_t){return 12.0;})==12);
    // Exact original values remain exact at temporal/frequency sample centres.
    for(std::int64_t bin=1;bin<=5;++bin)
        assert(bandPeak(0,20,5,(bin-1)*20,(bin-1)*20,read)==read(bin));
    std::cout<<"SMART DISPLAY TESTS: PASS\n";
}
