// Praat Pro Edition. GPL-3.0-or-later.
#include "SpectrogramS.h"
#include "NUM2.h"
#include <algorithm>
#include <cmath>
#include "oo_DESTROY.h"
#include "SpectrogramS_def.h"
#include "oo_COPY.h"
#include "SpectrogramS_def.h"
#include "oo_EQUAL.h"
#include "SpectrogramS_def.h"
#include "oo_CAN_WRITE_AS_ENCODING.h"
#include "SpectrogramS_def.h"
#include "oo_WRITE_TEXT.h"
#include "SpectrogramS_def.h"
#include "oo_READ_TEXT.h"
#include "SpectrogramS_def.h"
#include "oo_WRITE_BINARY.h"
#include "SpectrogramS_def.h"
#include "oo_READ_BINARY.h"
#include "SpectrogramS_def.h"
#include "oo_DESCRIPTION.h"
#include "SpectrogramS_def.h"
// Matrix's current binary/text representation is version 2.
Thing_implement(SpectrogramS,Spectrogram,2);
void structSpectrogramS::v1_info(){
    SpectrogramS_Parent::v1_info();
    MelderInfo_writeLine(U"Source sampling frequency: ",sourceSamplingFrequency,U" Hz");
    MelderInfo_writeLine(U"Analysis window: ",windowSamples,U" samples (",windowSamples?1000.0*windowSamples/sourceSamplingFrequency:undefined,U" ms)");
    MelderInfo_writeLine(U"Time step: ",hopSamples,U" samples (",1000.0*dx,U" ms)");
    MelderInfo_writeLine(U"Transform size: ",fftSamples,U" samples (0 = unknown after conversion)");
    MelderInfo_writeLine(U"Transform method: ",fftSamples==0?U"unknown (copied values)":(fftSamples&(fftSamples-1))==0?U"FFT":U"direct DFT");
    MelderInfo_writeLine(U"Window shape: ",kSound_to_Spectrogram_windowShape_getText(windowShape));
    MelderInfo_writeLine(U"Computed from Sound: ",computedFromSound?U"yes":U"no; values copied without recalculation");
}
void SpectrogramS_validate(constSpectrogramS me){
    Melder_require(std::isfinite(my sourceSamplingFrequency)&&my sourceSamplingFrequency>0,U"Invalid source sampling frequency.");
    Melder_require(my windowSamples==0||(my windowSamples>=2&&my windowSamples<=1048576),U"Use 2 to 1048576 window samples (or 0 for unknown metadata).");
    Melder_require(std::isfinite(my hopSamples)&&my hopSamples>0&&std::isfinite(my dx)&&my dx>0,U"Invalid frame time step.");
    Melder_require(my fftSamples==0||(my fftSamples>=2&&my fftSamples>=my windowSamples&&my fftSamples<=1048576),U"Transform size must be at least the window length, at least 2, and at most 1048576.");
    Melder_require(std::isfinite(my ymax)&&my ymax>0&&my ymax<=my sourceSamplingFrequency/2+my sourceSamplingFrequency*1e-12,U"Spectrogram frequency range exceeds the source Nyquist frequency.");
    Melder_require((int)my windowShape>=0&&(int)my windowShape<=5,U"Invalid window shape.");
    Melder_require(!my computedFromSound||(my windowSamples>=2&&my fftSamples>=my windowSamples&&my hopSamples==std::floor(my hopSamples)),U"Invalid sample analysis metadata.");
    Melder_require(std::abs(my dx*my sourceSamplingFrequency-my hopSamples)<=1e-9*my hopSamples,U"Frame time grid does not match sample metadata.");
    Melder_require(my nx>=1&&my ny>=1&&my ny<=20000000/my nx,U"Use at most 20 million SpectrogramS values.");
}
autoSpectrogramS Spectrogram_to_SpectrogramS(constSpectrogram me,double fs,integer window,kSound_to_Spectrogram_windowShape shape){
    Melder_require(std::isfinite(fs)&&fs>0,U"Sampling frequency must be finite and positive.");
    Melder_require(my ymax<=fs/2+fs*1e-12,U"Source sampling frequency must be at least twice the spectrogram maximum frequency.");
    Melder_require(my nx>=1&&my ny>=1&&my ny<=20000000/my nx,U"Use at most 20 million SpectrogramS values.");
    auto result=Thing_new(SpectrogramS);
    Matrix_init(result.get(),my xmin,my xmax,my nx,my dx,my x1,my ymin,my ymax,my ny,my dy,my y1);
    result->sourceSamplingFrequency=fs;result->windowSamples=window;result->hopSamples=my dx*fs;
    result->fftSamples=0;result->windowShape=shape;result->computedFromSound=false;
    SpectrogramS_validate(result.get());result->z.all()<<=my z.all();return result;
}
autoSpectrogram SpectrogramS_to_Spectrogram(constSpectrogramS me){
    SpectrogramS_validate(me);auto result=Spectrogram_create(my xmin,my xmax,my nx,my dx,my x1,my ymin,my ymax,my ny,my dy,my y1);
    result->z.all()<<=my z.all();return result;
}
autoSpectrogramS Sound_to_SpectrogramS(constSound sound,integer n,integer hop,double fmax,integer fft,
    kSound_to_Spectrogram_windowShape shape,integer sourceSampleOffset){
    Melder_require(n>=2&&n<=1048576,U"Use 2 to 1048576 analysis samples.");
    Melder_require(hop>=1&&hop<=2147483647,U"Time step must be a positive sample count up to 2147483647.");
    Melder_require(n<=sound->nx,U"The analysis window exceeds the number of Sound samples.");
    const double fs=1.0/sound->dx;
    Melder_require(std::isfinite(fmax)&&fmax>0&&fmax<=fs/2+fs*1e-12,U"Maximum frequency must be positive and not exceed Nyquist (",fs/2,U" Hz).");
    if(fft==0)fft=n;
    Melder_require(fft>=n&&fft<=1048576,U"Transform size must be at least the window length, at least 2, and at most 1048576.");
    Melder_require(sourceSampleOffset>=0,U"Source sample offset must not be negative.");
    const integer first=(hop-sourceSampleOffset%hop)%hop; // keep the original Sound's hop grid when zooming
    Melder_require(first+n<=sound->nx,U"No complete analysis window on this sample grid.");
    const integer frames=1+(sound->nx-first-n)/hop;
    const double df=fs/fft;
    const integer bins=std::min(fft/2+1,1+(integer)std::floor(fmax/df+1e-12));
    Melder_require(bins<=20000000/frames,U"SpectrogramS exceeds 20 million values. Increase the time step or reduce FFT size/frequency range.");
    auto result=Thing_new(SpectrogramS);
    Matrix_init(result.get(),sound->xmin,sound->xmax,frames,hop/fs,sound->x1+(first+0.5*(n-1))/fs,0,fmax,bins,df,0);
    result->sourceSamplingFrequency=fs;result->windowSamples=n;result->hopSamples=hop;result->fftSamples=fft;
    result->windowShape=shape;result->computedFromSound=true;
    auto window=raw_VEC(n);longdouble sum=0;
    for(integer j=1;j<=n;++j){
        const double u=(j-0.5)/n,v=2*u-1;double w=1;
        switch(shape){
            case kSound_to_Spectrogram_windowShape::SQUARE:break;
            case kSound_to_Spectrogram_windowShape::HAMMING:w=.54-.46*std::cos(2*NUMpi*u);break;
            case kSound_to_Spectrogram_windowShape::BARTLETT:w=1-std::abs(v);break;
            case kSound_to_Spectrogram_windowShape::WELCH:w=1-v*v;break;
            case kSound_to_Spectrogram_windowShape::HANNING:w=.5-.5*std::cos(2*NUMpi*u);break;
            case kSound_to_Spectrogram_windowShape::GAUSSIAN:w=(std::exp(-12*v*v)-std::exp(-12.0))/(1-std::exp(-12.0));break;
            default:Melder_throw(U"Unknown analysis window shape.");
        }
        window[j]=w;sum+=w*w;
    }
    Melder_require(sum>0,U"The window has zero energy.");
    const bool useFFT=(fft&(fft-1))==0;
    auto data=zero_VEC(fft);
    autoNUMFourierTable table;
    if(useFFT)table=NUMFourierTable_create(fft);
    autoMelderProgress progress(U"Sound to SpectrogramS...");
    for(integer frame=1;frame<=frames;++frame){
        if(frame==1||frame%32==0||frame==frames)Melder_progress(double(useFFT?frame:frame-1)/frames,U"SpectrogramS: frame ",frame,U" / ",frames);
        const integer start=1+first+(frame-1)*hop;
        for(integer channel=1;channel<=sound->ny;++channel){
            for(integer j=1;j<=n;++j)data[j]=sound->z[channel][start+j-1]*window[j];
            for(integer j=n+1;j<=fft;++j)data[j]=0;
            if(useFFT)NUMfft_forward(table.get(),data.get());
            for(integer bin=1;bin<=bins;++bin){
                double power;
                if(useFFT){
                    power=bin==1?data[1]*data[1]:bin==fft/2+1?data[fft]*data[fft]:data[2*bin-2]*data[2*bin-2]+data[2*bin-1]*data[2*bin-1];
                }else{
                    // X[k] = sum(j=0..N-1) x[j]*w[j] * exp(-i*2*pi*k*j/M).
                    // Explicit M>N requests zero padding; automatic M=N never pads.
                    const integer k=bin-1;
                    longdouble real=0,imaginary=0;
                    for(integer j=0;j<n;++j){
                        // Reduce the phase to one turn before trigonometry; k*j fits integer64.
                        const double angle=2*NUMpi*double((k*j)%fft)/fft;
                        real+=data[j+1]*std::cos(angle);
                        imaginary-=data[j+1]*std::sin(angle);
                    }
                    power=double(real*real+imaginary*imaginary);
                    if(bin%16==0||bin==bins)Melder_progress(
                        (frame-1+(channel-1+double(bin)/bins)/sound->ny)/frames,
                        U"Direct DFT: frame ",frame,U" / ",frames,U", bin ",bin,U" / ",bins);
                }
                // Same FFT-power/window-energy convention as Praat Spectrogram; average channel powers.
                result->z[bin][frame]+=power/(double(sum)*sound->ny);
            }
        }
    }
    SpectrogramS_validate(result.get());return result;
}
