// Praat Pro Edition. GPL-3.0-or-later.
#ifndef PRAAT_SEG_READER_H
#define PRAAT_SEG_READER_H
#include "TextGrid.h"
enum class SegEncoding { UTF8=1, Windows1251, UTF16LE, UTF16BE, KOI8R, CP866, Latin1 };
autoTextGrid TextGrid_readSegFiles (MelderFile anchor, conststring32 levels, SegEncoding encoding = SegEncoding::UTF8);
#endif
