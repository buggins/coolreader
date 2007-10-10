/*******************************************************

   CoolReader Engine

   rtfimp.cpp:  RTF import implementation

   (c) Vadim Lopatin, 2000-2006
   This source code is distributed under the terms of
   GNU General Public License
   See LICENSE file for details

*******************************************************/
#include "../include/rtfimp.h"

//==================================================
// Text file parser

/// constructor
LVRtfParser::LVRtfParser( LVStreamRef stream, LVXMLParserCallback * callback )
    : LVFileParserBase(stream)
    , m_callback(callback)
{
}

/// descructor
LVRtfParser::~LVRtfParser()
{
}

/// returns true if format is recognized by parser
bool LVRtfParser::CheckFormat()
{
    bool res = false;
    Reset();
    FillBuffer( 50 );
    res = (m_buf[0]=='{' && m_buf[1]=='\\' && m_buf[2]=='r'
         && m_buf[3]=='t' && m_buf[4]=='f' );
    Reset();
    return res;
}

/// parses input stream
bool LVRtfParser::Parse()
{
    return false;
}

/// resets parsing, moves to beginning of stream
void LVRtfParser::Reset()
{
    LVFileParserBase::Reset();
}

/// sets charset by name
void LVRtfParser::SetCharset( const lChar16 * name )
{
    //TODO
}

/// sets 8-bit charset conversion table (128 items, for codes 128..255)
void LVRtfParser::SetCharsetTable( const lChar16 * table )
{
    //TODO
}

/// returns 8-bit charset conversion table (128 items, for codes 128..255)
lChar16 * LVRtfParser::GetCharsetTable( )
{
    return NULL;
}

void LVRtfParser::OnBraceOpen()
{
}

void LVRtfParser::OnBraceClose()
{
}

void LVRtfParser::OnControlWord( const char * control, int param )
{
}

void LVRtfParser::OnText( const lChar16 * text, int len,
        lvpos_t fpos, lvsize_t fsize, lUInt32 flags )
{
}
