/***************************************************************************
 *   CoolReader engine                                                     *
 *   Copyright (C) 2013 Vadim Lopatin <coolreader.org@gmail.com>           *
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

#include "../include/bookformats.h"

lString32 LVDocFormatName(int fmt) {
    switch (fmt) {
        case doc_format_fb2: return lString32("FB2");
        case doc_format_fb3: return lString32("FB3");
        case doc_format_txt: return lString32("TXT");
        case doc_format_rtf: return lString32("RTF");
        case doc_format_epub: return lString32("EPUB");
        case doc_format_html: return lString32("HTML");
        case doc_format_txt_bookmark: return lString32("BMK");
        case doc_format_chm: return lString32("CHM");
        case doc_format_doc: return lString32("DOC");
        case doc_format_docx: return lString32("DOCX");
        case doc_format_pdb: return lString32("PDB");
        case doc_format_odt: return lString32("ODT");
        default: return lString32("?");
    }
}

lString8 LVDocFormatCssFileName(int fmt) {
    switch (fmt) {
        case doc_format_fb2: return lString8("fb2.css");
        case doc_format_fb3: return lString8("fb3.css");
        case doc_format_txt: return lString8("txt.css");
        case doc_format_rtf: return lString8("rtf.css");
        case doc_format_epub: return lString8("epub.css");
        case doc_format_html: return lString8("htm.css");
        case doc_format_txt_bookmark: return lString8("txt.css");
        case doc_format_chm: return lString8("chm.css");
        case doc_format_doc: return lString8("doc.css");
        case doc_format_docx: return lString8("docx.css");
        case doc_format_pdb: return lString8("htm.css");
        case doc_format_odt: return lString8("odt.css");
        default: return lString8("txt.css");
    }
}

int LVDocFormatFromExtension(lString32 &pathName) {
    if (pathName.endsWith(".fb2"))
        return doc_format_fb2;
    if (pathName.endsWith(".fb3"))
        return doc_format_fb3;
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
    if (pathName.endsWith(".docx"))
        return doc_format_docx;
    if (pathName.endsWith(".pdb") || pathName.endsWith(".prc") || pathName.endsWith(".mobi") || pathName.endsWith(".azw"))
        return doc_format_pdb;
    if (pathName.endsWith(".odt"))
        return doc_format_odt;
    return doc_format_none;
}

