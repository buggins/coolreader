
#include "bookformats.h"

lString16 LVDocFormatName(int fmt) {
    switch (fmt) {
    case doc_format_fb2: return lString16("FB2");
    case doc_format_txt: return lString16("TXT");
    case doc_format_rtf: return lString16("RTF");
    case doc_format_epub: return lString16("EPUB");
    case doc_format_html: return lString16("HTML");
    case doc_format_txt_bookmark: return lString16("BMK");
    case doc_format_chm: return lString16("CHM");
    case doc_format_doc: return lString16("DOC");
    case doc_format_pdb: return lString16("PDB");
    default: return lString16("?");
    }
}

lString8 LVDocFormatCssFileName(int fmt) {
    switch (fmt) {
    case doc_format_fb2: return lString8("fb2.css");
    case doc_format_txt: return lString8("txt.css");
    case doc_format_rtf: return lString8("rtf.css");
    case doc_format_epub: return lString8("epub.css");
    case doc_format_html: return lString8("htm.css");
    case doc_format_txt_bookmark: return lString8("txt.css");
    case doc_format_chm: return lString8("chm.css");
    case doc_format_doc: return lString8("doc.css");
    case doc_format_pdb: return lString8("htm.css");
    default: return lString8("txt.css");
    }
}

int LVDocFormatFromExtension(lString16 &pathName) {
    if (pathName.endsWith(".fb2"))
        return doc_format_fb2;
    if (pathName.endsWith(".txt") || pathName.endsWith(".tcr") || pathName.endsWith(".pml"))
        return doc_format_txt;
    if (pathName.endsWith(".rtf"))
        return doc_format_rtf;
    if (pathName.endsWith(".epub"))
        return doc_format_epub;
    if (pathName.endsWith(".htm") || pathName.endsWith(".html") || pathName.endsWith(".shtml") || pathName.endsWith(".xhtml"))
        return doc_format_html;
    if (pathName.endsWith(".txt.bmk"))
        return doc_format_txt_bookmark;
    if (pathName.endsWith(".chm"))
        return doc_format_chm;
    if (pathName.endsWith(".doc"))
        return doc_format_doc;
    if (pathName.endsWith(".pdb") || pathName.endsWith(".prc") || pathName.endsWith(".mobi") || pathName.endsWith(".azw"))
        return doc_format_pdb;
    return doc_format_none;
}

