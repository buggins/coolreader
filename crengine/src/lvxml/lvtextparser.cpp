/***************************************************************************
 *   CoolReader engine                                                     *
 *   Copyright (C) 2007-2010 Vadim Lopatin <coolreader.org@gmail.com>      *
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

#include "lvtextparser.h"
#include "lvxmlparsercallback.h"
#include "lvtextlinequeue.h"
#include "crlog.h"

/// constructor
LVTextParser::LVTextParser( LVStreamRef stream, LVXMLParserCallback * callback, bool isPreFormatted )
    : LVTextFileBase(stream)
    , m_callback(callback)
    , m_isPreFormatted( isPreFormatted )
{
    m_firstPageTextCounter = 300;
}

/// descructor
LVTextParser::~LVTextParser()
{
}

/// returns true if format is recognized by parser
bool LVTextParser::CheckFormat()
{
    Reset();
    // encoding test
    if ( !AutodetectEncoding() )
        return false;
    #define TEXT_PARSER_DETECT_SIZE 16384
    Reset();
    lChar32 * chbuf = new lChar32[TEXT_PARSER_DETECT_SIZE];
    FillBuffer( TEXT_PARSER_DETECT_SIZE );
    int charsDecoded = ReadTextBytes( 0, m_buf_len, chbuf, TEXT_PARSER_DETECT_SIZE-1, 0 );
    bool res = false;
    if ( charsDecoded > 16 ) {
        int illegal_char_count = 0;
        int crlf_count = 0;
        int space_count = 0;
        for ( int i=0; i<charsDecoded; i++ ) {
            if ( chbuf[i]<=32 ) {
                switch( chbuf[i] ) {
                case ' ':
                case '\t':
                    space_count++;
                    break;
                case 10:
                case 13:
                    crlf_count++;
                    break;
                case 12:
                //case 9:
                case 8:
                case 7:
                case 30:
                case 0x14:
                case 0x15:
                    break;
                default:
                    illegal_char_count++;
                }
            }
        }
        if ( illegal_char_count==0 && (space_count>=charsDecoded/16 || crlf_count>0) )
            res = true;
        if ( illegal_char_count>0 )
            CRLog::error("illegal characters detected: count=%d", illegal_char_count );
    }
    delete[] chbuf;
    Reset();
    return res;
}

/// parses input stream
bool LVTextParser::Parse()
{
    LVTextLineQueue queue( this, 2000 );
    queue.ReadLines( 2000 );
    if ( !m_isPreFormatted )
        queue.detectFormatFlags();
    // make fb2 document structure
    m_callback->OnTagOpen( NULL, U"?xml" );
    m_callback->OnAttribute( NULL, U"version", U"1.0" );
    m_callback->OnAttribute( NULL, U"encoding", GetEncodingName().c_str() );
    m_callback->OnEncoding( GetEncodingName().c_str(), GetCharsetTable( ) );
    m_callback->OnTagBody();
    m_callback->OnTagClose( NULL, U"?xml" );
    m_callback->OnTagOpenNoAttr( NULL, U"FictionBook" );
      // DESCRIPTION
      m_callback->OnTagOpenNoAttr( NULL, U"description" );
        m_callback->OnTagOpenNoAttr( NULL, U"title-info" );
          queue.DetectBookDescription( m_callback );
        m_callback->OnTagClose( NULL, U"title-info" );
      m_callback->OnTagClose( NULL, U"description" );
      // BODY
      m_callback->OnTagOpenNoAttr( NULL, U"body" );
        //m_callback->OnTagOpen( NULL, U"section" );
          // process text
          queue.DoTextImport( m_callback );
        //m_callback->OnTagClose( NULL, U"section" );
      m_callback->OnTagClose( NULL, U"body" );
    m_callback->OnTagClose( NULL, U"FictionBook" );
    return true;
}
