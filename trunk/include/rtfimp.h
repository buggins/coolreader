/*******************************************************

   CoolReader Engine

   rtfimp.cpp:  RTF import implementation

   (c) Vadim Lopatin, 2000-2006
   This source code is distributed under the terms of
   GNU General Public License
   See LICENSE file for details

*******************************************************/

#include "lvxml.h"

#define PARAM_VALUE_NONE 0x7FFFFFFF

enum rtf_control_word_type {
    CWT_CHAR,
    CWT_STYLE,
};

typedef struct  {
    int id;
    const char * name;
    rtf_control_word_type type;
    int index;
} rtf_control_word;

enum rtf_cmd_id {
#define RTF_CMD( name, type, index ) \
    RTF_##name,
#define RTF_CHC( name, index ) \
    RTF_##name,
#define RTF_CHR( character, name, index ) \
    RTF_##name,
#include "../include/rtfcmd.h"
};

class LVRtfParser : public LVFileParserBase
{
protected:
    LVXMLParserCallback * m_callback;
    lChar16 * txtbuf; /// text buffer
    int txtpos; /// text chars
    int txtfstart; /// text start file offset
    void OnBraceOpen();
    void OnBraceClose();
    void OnControlWord( const char * control, int param );
    void OnText( const lChar16 * text, int len,
        lvpos_t fpos, lvsize_t fsize, lUInt32 flags );
    void CommitText();
    void AddChar( lChar16 ch );
    void AddChar8( lUInt8 ch );
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

