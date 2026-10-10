// Praat Pro Edition. GPL-3.0-or-later.
#include "SmartSpectrogram.h"
#include <algorithm>
#include <cmath>
#include <string>
#include <vector>

#include "oo_DESTROY.h"
#include "SmartSpectrogram_def.h"
#include "oo_COPY.h"
#include "SmartSpectrogram_def.h"
#include "oo_EQUAL.h"
#include "SmartSpectrogram_def.h"
#include "oo_CAN_WRITE_AS_ENCODING.h"
#include "SmartSpectrogram_def.h"
#include "oo_WRITE_TEXT.h"
#include "SmartSpectrogram_def.h"
#include "oo_READ_TEXT.h"
#include "SmartSpectrogram_def.h"
#include "oo_WRITE_BINARY.h"
#include "SmartSpectrogram_def.h"
#include "oo_READ_BINARY.h"
#include "SmartSpectrogram_def.h"
#include "oo_DESCRIPTION.h"
#include "SmartSpectrogram_def.h"

Thing_implement (SmartSpectrogram, Function, 0);
static constexpr integer maximumValues = 20000000;  // keep import/export usable on Windows 7 x86

void structSmartSpectrogram::v1_info () {
    SmartSpectrogram_Parent::v1_info ();
    MelderInfo_writeLine (U"Frames: ", numberOfFrames, U"; frequency bins: ", numberOfBins);
    MelderInfo_writeLine (U"Frequency axis: ", firstFrequencyHz, U" Hz + bin × ", frequencyStepHz, U" Hz");
    MelderInfo_writeLine (U"Values: dB; frame times may be irregular.");
}
void structSmartSpectrogram::v_shiftX (double from, double to) {
    SmartSpectrogram_Parent::v_shiftX (from, to);
    for (integer i = 1; i <= numberOfFrames; ++ i) NUMshift (& times [i], from, to);
    if (pitch) pitch->v_shiftX (from, to);
    if (intensity) intensity->v_shiftX (from, to);
    if (trajectories) trajectories->v_shiftX (from, to);
}
void structSmartSpectrogram::v_scaleX (double a, double b, double c, double d) {
    SmartSpectrogram_Parent::v_scaleX (a, b, c, d);
    for (integer i = 1; i <= numberOfFrames; ++ i) NUMscale (& times [i], a, b, c, d);
    if (pitch) pitch->v_scaleX (a, b, c, d);
    if (intensity) intensity->v_scaleX (a, b, c, d);
    if (trajectories) trajectories->v_scaleX (a, b, c, d);
}
autoSmartSpectrogram SmartSpectrogram_create (constVEC times, integer bins, double firstHz, double stepHz) {
    Melder_require (times.size >= 1 && times.size <= 100000, U"Use 1 to 100000 frames.");
    Melder_require (bins >= 2 && bins <= 100000 && bins <= maximumValues / times.size,
        U"Use 2 to 100000 bins and at most 20 million values.");
    Melder_require (std::isfinite (firstHz) && firstHz >= 0 && std::isfinite (stepHz) && stepHz > 0,
        U"Frequency start and step must be finite; start >= 0 and step > 0.");
    Melder_require (std::isfinite (firstHz + (bins - 1) * stepHz), U"Frequency range overflows.");
    for (integer i = 1; i <= times.size; ++ i)
        Melder_require (std::isfinite (times [i]) && (i == 1 || times [i] > times [i - 1]),
            U"Frame times must be finite and strictly increasing.");
    autoSmartSpectrogram me = Thing_new (SmartSpectrogram);
    my numberOfFrames = times.size;
    my numberOfBins = bins;
    my firstFrequencyHz = firstHz;
    my frequencyStepHz = stepHz;
    my xmin = times [1] - (times.size == 1 ? 0.5 : 0.0);
    my xmax = times [times.size] + (times.size == 1 ? 0.5 : 0.0);
    my times = copy_VEC (times);
    my valuesDb = zero_MAT (bins, times.size);
    return me;
}
void SmartSpectrogram_validate (SmartSpectrogram me) {
    Melder_require (my numberOfFrames >= 1 && my numberOfBins >= 2 &&
        my numberOfFrames <= 100000 && my numberOfBins <= maximumValues / my numberOfFrames &&
        my times.size == my numberOfFrames && my valuesDb.nrow == my numberOfBins &&
        my valuesDb.ncol == my numberOfFrames, U"Invalid SmartSpectrogram dimensions.");
    Melder_require (std::isfinite (my firstFrequencyHz) && my firstFrequencyHz >= 0 &&
        std::isfinite (my frequencyStepHz) && my frequencyStepHz > 0,
        U"Invalid frequency axis.");
    for (integer i = 1; i <= my numberOfFrames; ++ i)
        Melder_require (std::isfinite (my times [i]) &&
            (i == 1 || my times [i] > my times [i - 1]), U"Frame times are not increasing.");
    for (integer row = 1; row <= my numberOfBins; ++ row)
        for (integer col = 1; col <= my numberOfFrames; ++ col)
            Melder_require (std::isfinite (my valuesDb [row] [col]), U"Spectrum values must be finite dB numbers.");
}
double SmartSpectrogram_valueAt (SmartSpectrogram me, double time, double hz) {
    if (! std::isfinite (time) || ! std::isfinite (hz) ||
        hz < my firstFrequencyHz || hz > my firstFrequencyHz + (my numberOfBins - 1) * my frequencyStepHz)
        return undefined;
    const double rowPosition = (hz - my firstFrequencyHz) / my frequencyStepHz + 1.0;
    const integer row1 = Melder_clipped (1_integer, (integer) std::floor (rowPosition), my numberOfBins);
    const integer row2 = std::min (my numberOfBins, row1 + 1);
    const double fy = rowPosition - row1;
    integer col1 = 1, col2 = 1;
    double fx = 0;
    if (time <= my times [1]) {
        col1 = col2 = 1;
    } else if (time >= my times [my numberOfFrames]) {
        col1 = col2 = my numberOfFrames;
    } else {
        integer left = 1, right = my numberOfFrames;
        while (right - left > 1) {
            integer mid = (left + right) / 2;
            if (my times [mid] <= time) left = mid; else right = mid;
        }
        col1 = left; col2 = right;
        fx = (time - my times [left]) / (my times [right] - my times [left]);
    }
    const double low = my valuesDb [row1] [col1] * (1 - fx) + my valuesDb [row1] [col2] * fx;
    const double high = my valuesDb [row2] [col1] * (1 - fx) + my valuesDb [row2] [col2] * fx;
    return low * (1 - fy) + high * fy;
}

static std::vector<std::u32string> split (const std::u32string &line, char32 separator) {
    std::vector<std::u32string> fields;
    size_t begin = 0;
    for (;;) {
        size_t end = line.find (separator, begin);
        fields.push_back (line.substr (begin, end == std::u32string::npos ? end : end - begin));
        if (end == std::u32string::npos) break;
        begin = end + 1;
    }
    return fields;
}
static double number (const std::u32string &field, integer lineNumber) {
    Melder_require (! field.empty() && Melder_isStringNumeric (field.c_str()),
        U"SmartSpectrogram line ", lineNumber, U": expected a number with a decimal point.");
    const double value = Melder_atof (field.c_str());
    Melder_require (std::isfinite (value), U"SmartSpectrogram line ", lineNumber, U": value must be finite.");
    return value;
}
autoSmartSpectrogram SmartSpectrogram_read (MelderFile file) {
    auto text = MelderFile_readText (file);
    const std::u32string source = text.get();
    bool hasStart = false, hasStep = false, hasValues = false, hasHeader = false;
    double firstHz = 0, stepHz = 0;
    char32 separator = U',';
    std::vector<double> frameTimes;
    std::vector<std::vector<double>> rows;
    integer lineNumber = 0;
    for (size_t offset = 0; offset < source.size();) {
        size_t end = source.find (U'\n', offset);
        if (end == std::u32string::npos) end = source.size();
        std::u32string line = source.substr (offset, end - offset);
        offset = end + (end < source.size());
        ++ lineNumber;
        if (! line.empty() && line.back() == U'\r') line.pop_back();
        if (line.empty()) continue;
        if (line[0] == U'#') {
            Melder_require (! hasHeader, U"Metadata must precede the time row (line ", lineNumber, U").");
            if (line == U"# SmartSpectrogram 1") continue;
            const std::u32string startKey = U"# frequency_start_hz=";
            const std::u32string stepKey = U"# frequency_step_hz=";
            const std::u32string valuesKey = U"# values=";
            if (line.rfind (startKey, 0) == 0) {
                Melder_require (! hasStart, U"Duplicate frequency_start_hz.");
                firstHz = number (line.substr (startKey.size()), lineNumber); hasStart = true;
            } else if (line.rfind (stepKey, 0) == 0) {
                Melder_require (! hasStep, U"Duplicate frequency_step_hz.");
                stepHz = number (line.substr (stepKey.size()), lineNumber); hasStep = true;
            } else if (line.rfind (valuesKey, 0) == 0) {
                Melder_require (! hasValues && line.substr (valuesKey.size()) == U"db", U"Only # values=db is supported.");
                hasValues = true;
            } else Melder_throw (U"Unknown metadata on line ", lineNumber, U".");
            continue;
        }
        if (! hasHeader) {
            Melder_require (hasStart && hasStep && hasValues, U"Missing SmartSpectrogram frequency or value metadata.");
            separator = line.find (U'\t') != std::u32string::npos ? U'\t' : U',';
            auto fields = split (line, separator);
            Melder_require (fields.size() >= 2 && fields[0] == U"time_s", U"Expected time_s followed by frame times.");
            for (size_t i = 1; i < fields.size(); ++ i) frameTimes.push_back (number (fields[i], lineNumber));
            Melder_require (frameTimes.size() <= 100000, U"Too many frames.");
            for (size_t i = 1; i < frameTimes.size(); ++ i)
                Melder_require (frameTimes[i] > frameTimes[i - 1], U"Times must be strictly increasing.");
            hasHeader = true;
        } else {
            auto fields = split (line, separator);
            Melder_require (fields.size() == frameTimes.size(), U"SmartSpectrogram line ", lineNumber,
                U": expected ", frameTimes.size(), U" dB values.");
            std::vector<double> row;
            for (const auto &field : fields) row.push_back (number (field, lineNumber));
            rows.push_back (std::move (row));
            Melder_require (rows.size() <= 100000 && rows.size() <= maximumValues / frameTimes.size(), U"Too many frequency bins.");
        }
    }
    Melder_require (hasHeader && rows.size() >= 2, U"SmartSpectrogram needs a time row and at least two frequency bins.");
    auto frameVector = raw_VEC ((integer) frameTimes.size());
    for (integer i = 1; i <= frameVector.size; ++ i) frameVector [i] = frameTimes [i - 1];
    auto me = SmartSpectrogram_create (frameVector.get(), (integer) rows.size(), firstHz, stepHz);
    for (integer row = 1; row <= my numberOfBins; ++ row)
        for (integer col = 1; col <= my numberOfFrames; ++ col)
            my valuesDb [row] [col] = rows [row - 1] [col - 1];
    return me;
}
void SmartSpectrogram_write (SmartSpectrogram me, MelderFile file, bool tsv) {
    SmartSpectrogram_validate (me);
    const char32 sep = tsv ? U'\t' : U',';
    std::u32string output = U"# SmartSpectrogram 1\n# frequency_start_hz=";
    output += Melder_double (my firstFrequencyHz);
    output += U"\n# frequency_step_hz=";
    output += Melder_double (my frequencyStepHz);
    output += U"\n# values=db\ntime_s";
    for (integer col = 1; col <= my numberOfFrames; ++ col) {
        output += sep; output += Melder_double (my times [col]);
    }
    output += U'\n';
    for (integer row = 1; row <= my numberOfBins; ++ row) {
        for (integer col = 1; col <= my numberOfFrames; ++ col) {
            if (col > 1) output += sep;
            output += Melder_double (my valuesDb [row] [col]);
        }
        output += U'\n';
    }
    MelderFile_writeText_e (file, output.c_str(), kMelder_textOutputEncoding::UTF8);
}
