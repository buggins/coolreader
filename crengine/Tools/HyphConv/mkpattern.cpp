// mkpattern.cpp -- convertor of TeX hyphenation files to FBReader format
// (c) Vadim Lopatin, 2011

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
    void processLine(lString16 & line) {
        if (line.lastChar()=='\r' || line.lastChar()=='\n')
            line.erase(line.length()-1, 1);
        if (state == 0) {
            //
            if (line.startsWith(lString16("%"))) {
                fprintf(out, "%s\n", LCSTR(line));
                return;
            }
            if (line.startsWith(lString16("\\patterns{"))) {
                start();
                return;
            }
        } else {
            lString16 word;
            for (int i=0; i<=line.length(); i++) {
                lChar16 ch = (i<line.length()) ? line[i] : 0;
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
    void addPattern(lString16 pattern) {
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
    if (!src) {
        printf("Cannot create file %s\n", argv[2]);
        return -2;
    }
    Convertor cnv(out);
    char buf[4096];
    while (fgets(buf, 4096, src)) {
        lString16 line(buf);
        cnv.processLine(line);
    }
    fclose(src);
    fclose(out);
    return 0;
}
