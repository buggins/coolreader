/*******************************************************

   CoolReader Engine

   lvtextrobustparser.cpp

   (c) Vadim Lopatin, 2000-2006
   This source code is distributed under the terms of
   GNU General Public License
   See LICENSE file for details

*******************************************************/

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
