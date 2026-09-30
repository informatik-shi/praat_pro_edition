// Praat Pro Edition. GPL-3.0-or-later.
// SpeechTechn SEG: byte offsets / BYTE_PER_SAMPLE / SAMPLING_FREQ.
#include "SegReader.h"
#include "Strings_.h"
#include <algorithm>
#include <cstdint>
#include <limits>
#include <map>
#include <set>
#include <string>
#include <vector>

namespace {
using Text = std::u32string;
Text lower(Text s) { for (auto &c : s) if (c >= U'A' && c <= U'Z') c += 32; return s; }
std::string trim(std::string s) {
    const auto first = s.find_first_not_of(" \t\r\n");
    if (first == std::string::npos) return {};
    return s.substr(first, s.find_last_not_of(" \t\r\n") - first + 1);
}
int64_t number(const std::string &source) {
    const auto s = trim(source);
    Melder_require(!s.empty(), U"Missing integer.");
    int64_t n = 0;
    for (unsigned char c : s) {
        Melder_require(c >= '0' && c <= '9', U"Expected a non-negative integer.");
        Melder_require(n <= (std::numeric_limits<int64_t>::max() - (c-'0')) / 10,
            U"Integer is too large.");
        n = n * 10 + c - '0';
    }
    return n;
}
#include "SegInput.h"
struct Mark { double time; Text label; };
struct SegTier { Text name; std::vector<Mark> marks; };
SegTier readTier(MelderFile file, const Text &name, SegEncoding encoding) {
    integer lineNumber = 0;
    try {
        autoMelderFile guard = MelderFile_open(file);
        SegInput input(file->filePointer,encoding);
        auto next = [&](std::string &s) { ++lineNumber; return input.next(s); };
        auto line = [&]() -> std::string {
            std::string s;
            Melder_require(next(s), U"Unexpected end of SEG file.");
            return s;
        };
        Melder_require(trim(line()) == "[PARAMETERS]", U"Expected [PARAMETERS]. Check the selected SEG encoding.");
        std::map<std::string,int64_t> header;
        for (;;) {
            auto s = trim(line());
            if (s == "[LABELS]") break;
            if (s.empty()) continue;
            auto eq = s.find('=');
            Melder_require(eq != std::string::npos, U"Expected KEY=VALUE or [LABELS].");
            auto key = trim(s.substr(0,eq));
            Melder_require(key == "SAMPLING_FREQ" || key == "BYTE_PER_SAMPLE" || key == "CODE" ||
                key == "N_CHANNEL" || key == "N_LABEL", U"Unknown SEG parameter.");
            Melder_require(header.emplace(key,number(s.substr(eq+1))).second, U"Duplicate SEG parameter.");
        }
        Melder_require(header.size() == 5, U"Missing SEG parameters.");
        const int64_t rate = header.at("SAMPLING_FREQ"), bytes = header.at("BYTE_PER_SAMPLE");
        Melder_require(rate > 0 && bytes > 0 && header.at("N_CHANNEL") > 0,
            U"SAMPLING_FREQ, BYTE_PER_SAMPLE and N_CHANNEL must be positive.");
        SegTier tier {name, {}};
        int64_t count = 0;
        std::string s;
        while (next(s)) {
            if (trim(s).empty()) continue;
            auto a = s.find(','), b = a == std::string::npos ? a : s.find(',',a+1);
            Melder_require(b != std::string::npos, U"Expected offset,type,label.");
            const int64_t offset = number(s.substr(0,a));
            (void) number(s.substr(a+1,b-a-1)); // colour/type does not define the tier name
            // Match SpeechTechn: first truncate byte offset to a sample index.
            // N_CHANNEL is metadata; the reference reader does not divide by it.
            const double time = double(offset / bytes) / double(rate);
            Melder_require(tier.marks.empty() || time > tier.marks.back().time,
                U"SEG boundaries must be strictly increasing after conversion to samples.");
            tier.marks.push_back({time,Text(Melder_8to32_e(s.substr(b+1).c_str(),kMelder_textInputEncoding::UTF8).get())});
            Melder_require(++count <= header.at("N_LABEL"), U"More labels than N_LABEL.");
        }
        Melder_require(count == header.at("N_LABEL"), U"N_LABEL does not match the number of labels.");
        Melder_require(count >= 2, U"At least two boundaries are needed for an IntervalTier.");
        return tier;
    } catch (MelderError) {
        Melder_throw(U"Cannot read SEG file ", MelderFile_messageName(file), U", line ", lineNumber, U".");
    }
}
}

autoTextGrid TextGrid_readSegFiles(MelderFile anchor, conststring32 levels, SegEncoding encoding) {
    try {
        Melder_require(int(encoding)>=1 && int(encoding)<=7, U"Unknown SEG encoding.");
        const Text filename = MelderFile_name(anchor);
        const auto dot = filename.find_last_of(U'.');
        Melder_require(dot != Text::npos && lower(filename.substr(dot+1,4)) == U"seg_" && dot+5 < filename.size(),
            U"Choose a file whose extension is seg_<tier>, for example recording.seg_B1.");
        Melder_require(MelderFile_readable(anchor), U"The selected SEG file cannot be read.");
        const Text stem = filename.substr(0,dot);
        std::vector<Text> requested;
        std::set<Text> seen;
        Text token;
        bool all = false;
        auto addToken = [&]() {
            if (token.empty()) return;
            Text key = lower(token);
            if (key == U"*" || key == U"all") all = true;
            else {
                if (key.rfind(U"seg_",0) == 0) key.erase(0,4);
                Melder_require(!key.empty() && key.find_first_of(U"./\\*?:") == Text::npos,
                    U"Use tier names such as B1 R1 G1, not file paths.");
                if (seen.insert(key).second) requested.push_back(key);
            }
            token.clear();
        };
        for (const char32_t *c = levels; *c; ++c) {
            if (*c == U' ' || *c == U'\t' || *c == U'\n' || *c == U'\r' || *c == U',' || *c == U';') addToken();
            else token += *c;
        }
        addToken();
        if (requested.empty()) all = true;
        structMelderFolder folder {};
        MelderFile_getParentFolder(anchor,&folder);
        structMelderFile wildcard {};
        MelderFolder_getFile(&folder,U"*",&wildcard);
        auto files = Strings_createAsFileList(wildcard.path);
        std::map<Text,Text> available;
        const Text prefix = stem + U".";
        for (integer i = 1; i <= files->numberOfStrings; ++i) {
            Text name = files->strings[i].get();
            if (name.rfind(prefix,0) != 0) continue;
            const Text ext = name.substr(prefix.size());
            if (ext.size() <= 4 || lower(ext.substr(0,4)) != U"seg_") continue;
            Text key = lower(ext.substr(4));
            Melder_require(available.emplace(key,name).second, U"Ambiguous SEG tier name: ", key.c_str(), U".");
        }
        for (const auto &key : requested)
            Melder_require(available.count(key), U"Requested SEG tier not found: ", key.c_str(), U".");
        if (all) {
            requested.clear();
            for (const auto &entry : available)
                if (entry.first != U"g1" || seen.count(U"g1")) requested.push_back(entry.first);
        }
        Melder_require(!requested.empty(), U"No SEG tiers selected. G1 is excluded unless explicitly listed.");
        std::vector<SegTier> tiers;
        double end = 0;
        for (const auto &key : requested) {
            structMelderFile file {};
            const Text &name = available.at(key);
            MelderFolder_getFile(&folder,name.c_str(),&file);
            tiers.push_back(readTier(&file,name.substr(prefix.size()+4),encoding));
            end = std::max(end,tiers.back().marks.back().time);
        }
        autoTextGrid result = TextGrid_createWithoutTiers(0,end);
        for (const auto &data : tiers) {
            autoIntervalTier tier = IntervalTier_create_raw(0,end);
            Thing_setName(tier.get(),data.name.c_str());
            if (data.marks.front().time > 0) IntervalTier_addInterval_raw(tier.get(),0,data.marks.front().time,U"");
            for (size_t i = 1; i < data.marks.size(); ++i)
                IntervalTier_addInterval_raw(tier.get(),data.marks[i-1].time,data.marks[i].time,data.marks[i-1].label.c_str());
            if (data.marks.back().time < end) IntervalTier_addInterval_raw(tier.get(),data.marks.back().time,end,U"");
            result->tiers->addItem_move(tier.move());
        }
        Thing_setName(result.get(),stem.c_str());
        return result;
    } catch (MelderError) {
        Melder_throw(U"TextGrid not read from SEG files.");
    }
}
