/*******************************************************

   CoolReader Engine

   rtfimp.cpp:  RTF import implementation

   (c) Vadim Lopatin, 2000-2006
   This source code is distributed under the terms of
   GNU General Public License
   See LICENSE file for details

*******************************************************/
#include "../include/rtfimp.h"
#include <strings.h>

//==================================================
// RTF file parser


#undef RTF_CMD
#undef RTF_CHR
#undef RTF_CHC
#define RTF_CMD( name, type, index ) \
    { RTF_##name, #name, type, index },
#define RTF_CHC( name, index ) \
    { RTF_##name, #name, CWT_CHAR, index },
#define RTF_CHR( character, name, index ) \
    { RTF_##name, character, CWT_CHAR, index },
static const rtf_control_word rtf_words[] = {
#include "../include/rtfcmd.h"
};
static const int rtf_words_count = sizeof(rtf_words) / sizeof(rtf_control_word);

static const rtf_control_word * findControlWord( const char * name )
{
    int a = 0;
    int b = rtf_words_count;
    int c;
    for ( ;; ) {
        c = ( a + b ) / 2;
        int res = strcmp( name, rtf_words[c].name );
        if ( !res )
            return &rtf_words[c];
        if ( a + 1 >=b )
            return NULL;
        if ( res>0 )
            a = c + 1;
        else
            b = c;
    }
}

/// constructor
LVRtfParser::LVRtfParser( LVStreamRef stream, LVXMLParserCallback * callback )
    : LVFileParserBase(stream)
    , m_callback(callback)
    , txtbuf(NULL)
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


void LVRtfParser::CommitText()
{
    if ( txtpos==0 )
        return;
    txtbuf[txtpos] = 0;
    OnText( txtbuf, txtpos, txtfstart, (m_buf_fpos + m_buf_pos) - txtfstart, 0 );
    txtpos = 0;
}

#define MAX_TXT_SIZE 65535

void LVRtfParser::AddChar8( lUInt8 ch )
{
    // TODO: add codepage support
    AddChar( ch );
}

// m_buf_pos points to first byte of char
void LVRtfParser::AddChar( lChar16 ch )
{
    if ( txtpos >= MAX_TXT_SIZE )
        CommitText();
    if ( txtpos==0 )
        txtfstart = m_buf_fpos + m_buf_pos;
    txtbuf[txtpos++] = ch;
}

#define MIN_BUF_DATA_SIZE 32768

static int charToHex( lUInt8 ch )
{
    if ( ch>='0' && ch<='9' )
        return ch-'0';
    if ( ch>='a' && ch<='f' )
        return ch-'a'+10;
    if ( ch>='A' && ch<='F' )
        return ch-'A'+10;
    return -1;
}

/// parses input stream
bool LVRtfParser::Parse()
{
    bool errorFlag = false;
    m_callback->OnStart(this);
    txtbuf = new lChar16[ MAX_TXT_SIZE+1 ];
    txtpos = 0;
    txtfstart = 0;
    char cwname[33];
    while ( !Eof() && !errorFlag && !m_stopped ) {
        // load next portion of data if necessary
        if ( m_buf_len - m_buf_pos < MIN_BUF_DATA_SIZE ) {
            if ( !FillBuffer( MIN_BUF_DATA_SIZE*2 ) ) {
                errorFlag = true;
                break;
            }
        }
        int len = (int)m_buf_len - (int)m_buf_pos;
        // check end of file
        if ( len <=0 )
            break; //EOF
        const lUInt8 * p = m_buf + m_buf_pos;
        char ch = *p++;
        if ( ch=='{' ) {
            OnBraceOpen();
            m_buf_pos++;
            continue;
        } else if ( ch=='}' ) {
            OnBraceClose();
            m_buf_pos++;
            continue;
        } else if ( ch=='\\' && *p!='\'' ) {
            // control
            char ch2 = *p;
            if ( (ch2>='A' && ch2<='Z') || (ch2>='a' && ch2<='z') ) {
                // control word
                int cwi = 0;
                do {
                    cwname[cwi++] = ch2;
                    ch2 = *++p;
                } while ( ( (ch2>='A' && ch2<='Z') || (ch2>='a' && ch2<='z') ) && cwi<32 );
                cwname[cwi] = 0;
                int param = PARAM_VALUE_NONE;
                if ( ch2==' ' ) {
                    p++;
                } else {
                    if ( ch2 == '-' ) {
                        p++;
                        param = 0;
                        while ( ch2>='0' && ch2<='9' ) {
                            param = param * 10 + (ch2-'0');
                            ch2 = *++p;
                        }
                        param = -param;
                    } else if ( ch2>='0' && ch2<='9' ) {
                        param = 0;
                        while ( ch2>='0' && ch2<='9' ) {
                            param = param * 10 + (ch2-'0');
                            ch2 = *++p;
                        }
                    }
                }
                OnControlWord( cwname, param );
            } else {
                // control char
                cwname[0] = ch2;
                p++;
                OnControlWord( cwname, PARAM_VALUE_NONE );
            }
            m_buf_pos += p - (m_buf + m_buf_pos);
        } else {
            lChar16 txtch = 0;
            if ( ch=='\\' ) {
                p++;
                int digit1 = charToHex( p[0] );
                int digit2 = charToHex( p[1] );
                if ( digit1>=0 && digit2>=0 ) {
                    AddChar8( (lChar8)((digit1 << 4) | digit2) );
                } else {
                    AddChar('\\');
                    AddChar('\'');
                    AddChar8(p[0]);
                    AddChar8(p[1]);
                    p+=2;
                }
            } else {
                AddChar8( *p++ );
            }
            //=======================================================
            //=======================================================
            m_buf_pos += p - (m_buf + m_buf_pos);
        }
    }
    m_callback->OnStop();
    delete txtbuf;
    txtbuf = NULL;
    return !errorFlag;
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
    CommitText();
}

void LVRtfParser::OnBraceClose()
{
    CommitText();
}

void LVRtfParser::OnControlWord( const char * control, int param )
{
    CommitText();
}

void LVRtfParser::OnText( const lChar16 * text, int len,
        lvpos_t fpos, lvsize_t fsize, lUInt32 flags )
{
    m_callback->OnText( text, len, fpos, fsize, flags );
}
