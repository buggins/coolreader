/***************************************************************************
 *   CoolReader engine                                                     *
 *   Copyright (C) 2010,2013 Vadim Lopatin <coolreader.org@gmail.com>      *
 *   Copyright (C) 2019,2020 Konstantin Potapov <pkbo@users.sourceforge.net>
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

#ifndef BOOKFORMATS_H
#define BOOKFORMATS_H

#include "lvstring.h"

/// source document formats
typedef enum {
    doc_format_none,
    doc_format_fb2,
    doc_format_fb3,
    doc_format_txt,
    doc_format_rtf,
    doc_format_epub,
    doc_format_html,
    doc_format_txt_bookmark, // coolreader TXT format bookmark
    doc_format_chm,
    doc_format_doc,
    doc_format_docx,
    doc_format_pdb,
    doc_format_odt,
    doc_format_max = doc_format_odt
    // don't forget update getDocFormatName() when changing this enum
    // Add new types of formats only at the end of this enum to save the correct format number in the history file/database!
} doc_format_t;

lString32 LVDocFormatName(int fmt);
int LVDocFormatFromExtension(lString32 &pathName);
lString8 LVDocFormatCssFileName(int fmt);


#endif // BOOKFORMATS_H
