/*******************************************************

   CoolReader Engine

   rtfimp.cpp:  RTF import implementation

   (c) Vadim Lopatin, 2000-2006
   This source code is distributed under the terms of
   GNU General Public License
   See LICENSE file for details

*******************************************************/
#ifndef RTFIMP_H_INCLUDED
#define RTFIMP_H_INCLUDED

#include "lvxml.h"
#include "crtxtenc.h"
#include <string.h>

#define PARAM_VALUE_NONE 0x7FFFFFFF

enum rtf_control_word_type {
    CWT_CHAR,  /// character entity
    CWT_STYLE, ///
    CWT_IPROP, /// integer property
    CWT_DEST,  /// destination
};

typedef struct  {
    int id;
    const char * name;
    rtf_control_word_type type;
    int index;
    int defvalue;
} rtf_control_word;

enum propIndex {
    pi_destination=-2,
    pi_bracket=-1,
    pi_ch_bold=0,
    pi_ch_italic,
    pi_ch_underline,
    pi_skip_ch_count,
    pi_skip_ansi,
    pi_ansicpg,
    pi_lang,
    pi_deflang,
    pi_align,
    pc_max
};

enum hAlign {
    ha_left = 0,
    ha_center,
    ha_justified,
    ha_right,
    ha_distributed,
    ha_thai,
};

enum rtfDestination {
    dest_default=0,
    dest_footnote,
    dest_header,
    dest_footer,
    dest_pict,
    dest_info,
    dest_fonttbl,
    dest_stylesheet,
    dest_colortbl,
    dest_upr,
    dest_ud,
    dest_max
};


enum rtf_cmd_id {
#define RTF_IPR( name, index, defvalue ) \
    RTF_##name,
#define RTF_CMD( name, type, index ) \
    RTF_##name,
#define RTF_DST( name, index ) \
    RTF_##name,
#define RTF_CHC( name, index ) \
    RTF_##name,
#define RTF_CHR( character, name, index ) \
    RTF_##name,
#include "../include/rtfcmd.h"
};

class LVRtfDestination;

typedef union {
    int i;
    void * p;
    LVRtfDestination * dest;
} propValue;

typedef struct {
    int index;
    propValue value;
} stackedValue;

class LVRtfParser;
class LVRtfValueStack;

class LVRtfDestination
{
protected:
    LVRtfParser & m_parser;
    LVRtfValueStack & m_stack;
    LVXMLParserCallback * m_callback;
public:
    enum rtf_actions {
        RA_PARA,
        RA_PAGE,
        RA_SECTION,
    };
    LVRtfDestination( LVRtfParser & parser );
    virtual void OnAction( int action ) { }
    virtual void OnControlWord( const char * control, int param ) { }
    virtual void OnText( const lChar16 * text, int len,
        lvpos_t fpos, lvsize_t fsize, lUInt32 flags ) { }
    virtual ~LVRtfDestination() { }
};

#define MAX_PROP_STACK_SIZE 4096
class LVRtfValueStack
{
protected:
    propValue props[pc_max];
    stackedValue stack[MAX_PROP_STACK_SIZE];
    LVRtfDestination * dest;
    int sp;
    bool error;
public:
    /// constructor
    LVRtfValueStack()
    : dest(NULL), sp(0), error(false)
    {
        sp = 0;
        memset(props, 0, sizeof(props) );
        props[pi_ansicpg].p = (void*)GetCharsetByte2UnicodeTable( 1251 );
    }
    ~LVRtfValueStack()
    {
        if ( dest )
            delete dest;
    }
    void setDestination( LVRtfDestination * newDest )
    {
        dest = newDest;
    }
    /// returns current destination
    inline LVRtfDestination * getDestination() { return dest; }
    /// converts byte to unicode using current code page
    inline lChar16 byteToUnicode( lUInt8 ch )
    {
        // skip ANSI character counter support
        if ( decInt(pi_skip_ch_count) )
            return 0;
        // skip sequence of ansi characters (\upr{} until \ud{} )
        if ( getInt( pi_skip_ansi )!=0 )
            return 0;
        // TODO: add codepage support
        if ( ch & 0x80 ) {
            const lChar16 * conv_table = (const lChar16 *)props[pi_ansicpg].p;
            return ( conv_table[ch & 0x7F] );
        } else {
            return ( ch );
        }
    }
    /// returns true if any error occured when accessing stack
    inline bool isError()
    {
        return error;
    }
    /// save state on { bracket
    inline void save()
    {
        if ( sp>=MAX_PROP_STACK_SIZE ) {
            error = true;
        } else {
            stack[sp++].index = pi_bracket;
        }
    }
    /// set new destination
    inline void set( LVRtfDestination * newdest )
    {
        if ( sp>=MAX_PROP_STACK_SIZE ) {
            error = true;
        } else {
            CRLog::trace("Changing destination. Level=%d old=%08X new=%08X", sp, (unsigned)dest, (unsigned)newdest);
            stack[sp].index = pi_destination;
            stack[sp++].value.dest = dest;
            dest = newdest;
        }
    }
    /// change integer property
    void set( int index, int value )
    {
        if ( sp>=MAX_PROP_STACK_SIZE ) {
            error = true;
        } else {
            stack[sp].index = index;
            if ( index==pi_ansicpg ) {
                stack[sp++].value.p = props[index].p;
                props[index].p = (void*)GetCharsetByte2UnicodeTable( value );
            } else {
                stack[sp++].value.i = props[index].i;
                props[index].i = value;
                if ( index==pi_lang ) {
                    set( pi_ansicpg, langToCodepage( value ) );
                } else if ( index==pi_deflang ) {
                    set( pi_ansicpg, langToCodepage( value ) );
                }
            }
        }
    }
    /// change pointer property
    void set( int index, void * value )
    {
        if ( sp>=MAX_PROP_STACK_SIZE ) {
            error = true;
        } else {
            stack[sp].index = index;
            stack[sp++].value.p = props[index].p;
            props[index].p = value;
        }
    }
    /// get int property
    inline int getInt( int index )
    {
        return props[index].i;
    }
    /// if int property > 0, decrement its value and return 0, otherwise do nothing and return false
    inline bool decInt( int index )
    {
        if ( props[index].i > 0 ) {
            props[index].i--;
            return true;
        }
        return false;
    }
    /// get pointer property
    void * getPtr( int index )
    {
        return props[index].p;
    }
    /// restore state on } bracket
    bool restore()
    {
        for ( ;; ) {
            if ( sp==0 ) {
                error = true;
                break;
            }
            int i = stack[--sp].index;
            if ( i==pi_bracket )
                break;
            if ( i==pi_destination ) {
                delete dest;
                dest = stack[sp].value.dest;
                CRLog::trace("Restoring destination. Level=%d Value=%08X", sp, (unsigned)dest);
            } else {
                props[i] = stack[sp].value;
            }
        }
        return !error;
    }
};

class LVRtfParser : public LVFileParserBase
{
    friend class LVRtfDestination;
protected:
    LVXMLParserCallback * m_callback;
    LVRtfValueStack m_stack;
    const lChar16 * m_conv_table; // charset conversion table for 8-bit encodings
    lChar16 * txtbuf; /// text buffer
    int txtpos; /// text chars
    int txtfstart; /// text start file offset
    LVRtfValueStack & getStack() { return m_stack; }
    LVXMLParserCallback * getCallback() { return m_callback; }
    void OnBraceOpen();
    void OnBraceClose();
    void OnControlWord( const char * control, int param, bool asterisk );
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


#endif // RTFIMP_H_INCLUDED
