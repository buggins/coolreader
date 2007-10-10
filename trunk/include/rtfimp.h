/*******************************************************

   CoolReader Engine

   rtfimp.cpp:  RTF import implementation

   (c) Vadim Lopatin, 2000-2006
   This source code is distributed under the terms of
   GNU General Public License
   See LICENSE file for details

*******************************************************/

#include "lvxml.h"

class LVRtfParser : public LVFileParserBase
{
protected:
    LVXMLParserCallback * m_callback;
    void OnBraceOpen();
    void OnBraceClose();
    void OnControlWord();
    void OnText( const lChar16 * text, int len,
        lvpos_t fpos, lvsize_t fsize, lUInt32 flags );
public:
    /// constructor
    LVRtfParser( LVStreamRef stream, LVXMLParserCallback * callback );
    /// returns true if format is recognized by parser
    virtual bool CheckFormat();
    /// parses input stream
    virtual bool Parse();
    /// resets parsing, moves to beginning of stream
    virtual void Reset();
    /// sets charset by name
    virtual void SetCharset( const lChar16 * name );
    /// sets 8-bit charset conversion table (128 items, for codes 128..255)
    virtual void SetCharsetTable( const lChar16 * table );
    /// returns 8-bit charset conversion table (128 items, for codes 128..255)
    virtual lChar16 * GetCharsetTable( );
    /// virtual destructor
    virtual ~LVRtfParser();
};

