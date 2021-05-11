
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
