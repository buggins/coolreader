/***************************************************************************
 *   CoolReader engine                                                     *
 *   Copyright (C) 2011,2012 Vadim Lopatin <coolreader.org@gmail.com>      *
 *   Copyright (C) 2020 Aleksey Chernov <valexlin@gmail.com>               *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or         *
 *   modify it under the terms of the GNU General Public License           *
 *   as published by the Free Software Foundation; either version 2        *
 *   of the License, or (at your option) any later version.                *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the Free Software           *
 *   Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,            *
 *   MA 02110-1301, USA.                                                   *
 ***************************************************************************/

// mkpattern.cpp -- convertor of TeX hyphenation files to FBReader format

#include <stdlib.h>
#include <crengine.h>

class Convertor {
    FILE * out;
    int state;
public:
    Convertor(FILE * f) : out(f), state(0) {
        fprintf(out, "<?xml version=\"1.0\" encoding=\"utf8\"?>\n"
            "<!--\n"
            "       hyphenations description for FBReader/CoolReader\n"
            "       from the original file:\n"
            "\n");
    }
    ~Convertor() {
        fprintf(out, "</HyphenationDescription>\n");
        //fclose(out);
    }
    void processLine(lString32 & line) {
        if (line.lastChar()=='\r' || line.lastChar()=='\n')
            line.erase(line.length()-1, 1);
        if (state == 0) {
            //
            if (line.startsWith(lString32("%"))) {
                fprintf(out, "%s\n", LCSTR(line));
                return;
            }
            if (line.startsWith(lString32("\\patterns{"))) {
                start();
                return;
            }
        } else {
            lString32 word;
            for (int i=0; i<=line.length(); i++) {
                lChar32 ch = (i<line.length()) ? line[i] : 0;
                if (ch == '}')
                    break;
                if (ch==' ' || ch=='\t' || ch=='%' || ch==0) {
                    if (!word.empty()) {
                        addPattern(word);
                        word.clear();
                    }
                    if (ch!=' ' && ch!='\t')
                        break;
                } else {
                    word.append(1, ch);
                }
            }
        }
    }
private:
    void addPattern(lString32 pattern) {
        if (pattern[0] == '.')
            pattern[0] = ' ';
        if (pattern[pattern.length()-1] == '.')
            pattern[pattern.length()-1] = ' ';
        fprintf(out, "  <pattern>%s</pattern>\n", LCSTR(pattern));
    }
    void start() {
        fprintf(out, "-->\n<HyphenationDescription>\n");
        state = 1;
    }
};

int main(int argc, char* argv[])
{
    if (argc < 2) {
        printf("Hyphenation pattern convertor\n");
        printf("usage: mkpattern <srclistfile.tex> <dstfile.pattern>\n");
        return -1;
    }
    FILE * src = fopen(argv[1], "rb");
    if (!src) {
        printf("File %s is not found\n", argv[1]);
        return -2;
    }
    FILE * out = fopen(argv[2], "wb");
    if (!out) {
        fclose(src);
        printf("Cannot create file %s\n", argv[2]);
        return -2;
    }
    Convertor cnv(out);
    char buf[4096];
    while (fgets(buf, 4096, src)) {
        lString32 line(buf);
        cnv.processLine(line);
    }
    fclose(src);
    fclose(out);
    return 0;
}
