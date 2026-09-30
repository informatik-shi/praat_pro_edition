// Explicit, strict decoding; independent of Praat's global input preferences.
#include "SegCodepages.h"
class SegInput {
    FILE *file;
    SegEncoding encoding;
    bool first = true, skipLF = false;
    int byte() {
        const int c = fgetc(file);
        Melder_require(!ferror(file), U"Error reading SEG file.");
        return c;
    }
    int requiredByte() {
        const int c = byte();
        Melder_require(c != EOF, U"Truncated character. Check the selected SEG encoding.");
        return c;
    }
    int unit16() {
        int a = requiredByte(), b = requiredByte();
        return encoding == SegEncoding::UTF16LE ? a + 256*b : 256*a + b;
    }
    int point() {
        int a = byte();
        if (a == EOF) return EOF;
        if (encoding == SegEncoding::UTF8) {
            if (a < 128) return a;
            int n, value, minimum;
            if (a >= 0xc2 && a <= 0xdf) { n=1; value=a&31; minimum=0x80; }
            else if (a >= 0xe0 && a <= 0xef) { n=2; value=a&15; minimum=0x800; }
            else if (a >= 0xf0 && a <= 0xf4) { n=3; value=a&7; minimum=0x10000; }
            else { Melder_throw(U"Invalid UTF-8. Select the correct SEG encoding (for example Windows-1251)."); }
            for (int i=0; i<n; ++i) {
                int b=requiredByte();
                Melder_require((b&0xc0)==0x80, U"Invalid UTF-8 continuation byte. Check the selected SEG encoding.");
                value=(value<<6)+(b&63);
            }
            Melder_require(value>=minimum && value<=0x10ffff && !(value>=0xd800 && value<=0xdfff),
                U"Invalid UTF-8 code point. Check the selected SEG encoding.");
            return value;
        }
        if (encoding == SegEncoding::UTF16LE || encoding == SegEncoding::UTF16BE) {
            int b = requiredByte();
            int value = encoding == SegEncoding::UTF16LE ? a+256*b : 256*a+b;
            if (value>=0xd800 && value<=0xdbff) {
                const int low=unit16();
                Melder_require(low>=0xdc00 && low<=0xdfff, U"Invalid UTF-16 surrogate pair.");
                return 0x10000+((value-0xd800)<<10)+(low-0xdc00);
            }
            Melder_require(!(value>=0xdc00 && value<=0xdfff) && value!=0xfffe,
                U"Invalid UTF-16 or wrong byte order. Choose UTF-16LE or UTF-16BE.");
            return value;
        }
        if (a<128 || encoding == SegEncoding::Latin1) return a;
        const char32_t *table = encoding == SegEncoding::Windows1251 ? segWindows1251 :
            encoding == SegEncoding::KOI8R ? segKoi8r : segCp866;
        Melder_require(table[a-128]!=0, U"Undefined byte in the selected SEG encoding.");
        return table[a-128];
    }
public:
    SegInput(FILE *stream, SegEncoding selected): file(stream), encoding(selected) {}
    bool next(std::string &out) {
        Text text;
        for (;;) {
            const int c = point();
            if (c == EOF) {
                if (text.empty()) return false;
                break;
            }
            if (first) {
                first=false;
                if (c==0xfeff) continue;
            }
            if (skipLF) {
                skipLF=false;
                if (c=='\n') continue;
            }
            Melder_require(c!=0, U"NUL character in SEG text. Check the selected encoding.");
            if (c=='\r') { skipLF=true; break; }
            if (c=='\n') break;
            text += char32_t(c);
        }
        auto utf8=Melder_32to8(text.c_str());
        out=utf8.get();
        return true;
    }
};