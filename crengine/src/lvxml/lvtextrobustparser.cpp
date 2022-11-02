/***************************************************************************
 *   CoolReader engine                                                     *
 *   Copyright (C) 2016 Hzj_jie <hzj_jie@hotmail.com>                      *
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

#include "lvtextrobustparser.h"

/// constructor
LVTextRobustParser::LVTextRobustParser( LVStreamRef stream, LVXMLParserCallback * callback, bool isPreFormatted )
    : LVTextParser(stream, callback, isPreFormatted)
{
}

/// descructor
LVTextRobustParser::~LVTextRobustParser()
{
}

/// returns true if format is recognized by parser
bool LVTextRobustParser::CheckFormat()
{
    m_lang_name = lString32( "en" );
    SetCharset( lString32( "utf-8" ).c_str() );
    return true;
}
