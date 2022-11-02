/***************************************************************************
 *   CoolReader engine                                                     *
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

#include <stdio.h>
#include <string.h>

#include "lvtypes.h"
#include "my_texhyph.h"
#include "lvstreamutils.h"
#include "crlog.h"

int pdb2pattern(const char* srcfilename, const char* dstfilename);

int main(int argc, char* argv[]) {
    if (argc < 3) {
        printf("Hyphenation dictionary dumper\n");
        printf("usage: hyph_dumper <srcfile.pdb> <dstfile.pattern>\n");
        printf("   or\n");
        printf("usage: hyph_dumper <srcfile.pattern> <dstfile.pattern>\n");
        return -1;
    }
    CRLog::setStdoutLogger();
    CRLog::setLogLevel(CRLog::LL_TRACE);
    return pdb2pattern(argv[1], argv[2]);
}

int pdb2pattern(const char *srcfilename, const char *dstfilename) {
    bool res = false;
    lString32 id = Utf8ToUnicode(srcfilename);
    MyTexHyph* hyph = new MyTexHyph(id);
    if (hyph->load(Utf8ToUnicode(srcfilename))) {
        CRLog::info("hyph dictionary \"%s\" loaded successfully.", srcfilename);
        LVStreamRef outStream = LVOpenFileStream(dstfilename, LVOM_WRITE);
        if (!outStream.isNull()) {
            if (hyph->dump(outStream, lString8(srcfilename)))
                res = true;
        } else {
            CRLog::error("Failed to open destination file!", dstfilename);
        }
    } else {
        CRLog::error("hyph dictionary \"%s\" NOT loaded!", srcfilename);
    }
    delete hyph;
    return res;
}
