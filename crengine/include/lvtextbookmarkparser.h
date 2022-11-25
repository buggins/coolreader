/***************************************************************************
 *   CoolReader engine                                                     *
 *   Copyright (C) 2009 Vadim Lopatin <coolreader.org@gmail.com>           *
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

#ifndef __LVTEXTBOOKMARKPARSER_H_INCLUDED__
#define __LVTEXTBOOKMARKPARSER_H_INCLUDED__

#include "lvtextparser.h"

/// parser of CoolReader's text format bookmarks
class LVTextBookmarkParser : public LVTextParser
{
public:
    /// constructor
    LVTextBookmarkParser( LVStreamRef stream, LVXMLParserCallback * callback );
    /// descructor
    virtual ~LVTextBookmarkParser();
    /// returns true if format is recognized by parser
    virtual bool CheckFormat();
    /// parses input stream
    virtual bool Parse();
};

#endif  // __LVTEXTBOOKMARKPARSER_H_INCLUDED__
