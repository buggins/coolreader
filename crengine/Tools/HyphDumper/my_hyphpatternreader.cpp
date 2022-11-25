/***************************************************************************
 *   CoolReader engine                                                     *
 *   Copyright (C) 2009-2012,2014 Vadim Lopatin <coolreader.org@gmail.com> *
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

// Copied from coolreader-3.2.49 (crengine/src/hyphman.cpp)

#include "my_hyphpatternreader.h"

MyHyphPatternReader::MyHyphPatternReader(lString32Collection &result) : insidePatternTag(false), data(result)
{
    result.clear();
}

ldomNode *MyHyphPatternReader::OnTagOpen(const lChar32 *nsname, const lChar32 *tagname)
{
    CR_UNUSED(nsname);
    if (!lStr_cmp(tagname, "pattern")) {
        insidePatternTag = true;
    }
    return NULL;
}

void MyHyphPatternReader::OnTagClose(const lChar32 *nsname, const lChar32 *tagname, bool self_closing_tag)
{
    CR_UNUSED2(nsname, tagname);
    insidePatternTag = false;
}

void MyHyphPatternReader::OnText(const lChar32 *text, int len, lUInt32 flags)
{
    CR_UNUSED(flags);
    if ( insidePatternTag )
        data.add( lString32(text, len) );
}
