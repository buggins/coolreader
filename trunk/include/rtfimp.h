/*******************************************************

   CoolReader Engine

   rtfimp.cpp:  RTF import implementation

   (c) Vadim Lopatin, 2000-2006
   This source code is distributed under the terms of
   GNU General Public License
   See LICENSE file for details

*******************************************************/

#include "lvxml.h"
#include <strings.h>

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

enum {
    pi_destination=-2,
    pi_bracket=-1,
    pi_ch_bold=0,
    pi_ch_italic,
    pi_ch_underline,
    pc_max
} propIndex;


enum rtf_cmd_id {
#define RTF_IPR( name, index, defvalue ) \
    RTF_##name,
#define RTF_CMD( name, type, index ) \
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
    LVRtfDestination( LVRtfParser & parser );
    virtual void OnControlWord( const char * control, int param ) = 0;
    virtual void OnText( const lChar16 * text, int len,
        lvpos_t fpos, lvsize_t fsize, lUInt32 flags ) = 0;
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
    : sp(0), error(false)
    {
        sp = 0;
        memset(props, 0, sizeof(props) );
    }
    /// returns current destination
    inline LVRtfDestination * getDestination() { return dest; }
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
        save();
        if ( sp>=MAX_PROP_STACK_SIZE ) {
            error = true;
        } else {
            stack[sp].index = pi_destination;
            stack[sp++].value.dest = dest;
            dest = newdest;
        }
    }
    /// change integer property
    inline void set( int index, int value )
    {
        if ( sp>=MAX_PROP_STACK_SIZE ) {
            error = true;
        } else {
            stack[sp].index = index;
            stack[sp++].value.i = props[index].i;
            props[index].i = value;
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
    int getInt( int index )
    {
        return props[index].i;
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
    LVRtfValueStack m_stack;
    LVXMLParserCallback * m_callback;
    lChar16 * txtbuf; /// text buffer
    int txtpos; /// text chars
    int txtfstart; /// text start file offset
    LVRtfValueStack & getStack() { return m_stack; }
    LVXMLParserCallback * getCallback() { return m_callback; }
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


